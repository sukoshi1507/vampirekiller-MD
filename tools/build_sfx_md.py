#!/usr/bin/env python3
"""Generate the whip sound effect as a per-frame PSG stream for sgdk/src/psg_player.c.

Source: castlevania-md (Mega Drive port of Castlevania NES, copyleft), SFX_0A, the NES
sound $0A that src/main.c plays on the whip's 11th frame (whip_tick). Its data is the
NES APU register script `sfx_0a` in src/music.c: pairs (register, value), 0xFE n = wait
n-1 frames, 0xFF = end of the frame's writes, no loop.

The APU script is replayed here exactly as src/sound.c does it (player_step, apu_clock,
sound_out for the noise channel, and psg_set's write-skipping), then the resulting PSG
writes are packed per 60 Hz frame. Stream format matches build_music_md.py: per frame, a
count byte then that many PSG bytes; 0xFF ends the stream.

Only the noise channel is modelled: sfx_0a writes only $400C-$400F. The script checks
that, and fails if the source ever writes another channel.

Usage: build_sfx_md.py [--source-dir DIR] [--output DIR]
Output: sgdk/src/generated/sfx_md.{c,h}. Never hand edit the generated files.
"""
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT.parent / 'castlevania-md'
SFX_NAME = 'sfx_0a'
SFX_ID_DEFINE = 'SFX_0A'

# sound.c: NES volume 0-15 -> PSG attenuation, and the noise period table.
ATTENUATION = [15, 12, 9, 7, 6, 5, 4, 3, 3, 2, 2, 1, 1, 1, 0, 0]
NOISE_PERIOD = [4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068]
LENGTH_TABLE = [10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
                12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30]
NOISE_CONTROL = 0xE7  # white noise, rate from tone 2 (sound.c writes it when the noise is acquired)
END = 0xFF


def read_source(source_dir):
    music = (source_dir / 'src/music.c').read_text(encoding='utf-8')
    header = (source_dir / 'inc/music.h').read_text(encoding='utf-8')

    m = re.search(r'static const u8 %s\[(\d+)\] = \{(.*?)\};' % SFX_NAME, music, re.S)
    if not m:
        raise SystemExit('array %s not found in %s' % (SFX_NAME, source_dir / 'src/music.c'))
    data = bytes(int(x) for x in re.findall(r'\d+', m.group(2)))
    if len(data) != int(m.group(1)):
        raise SystemExit('size mismatch for %s' % SFX_NAME)

    # The sounds[] table index must equal the SFX id (SFX_0A = 24).
    table = re.search(r'const Sound sounds\[SOUNDS\] = \{(.*?)\n\};', music, re.S).group(1)
    entries = re.findall(r'\{\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', table)
    index = [i for i, e in enumerate(entries) if e[0] == SFX_NAME]
    sfx_id = int(re.search(r'#define %s\s+(\d+)' % SFX_ID_DEFINE, header).group(1))
    if index != [sfx_id]:
        raise SystemExit('%s is at sounds[%s], expected %d' % (SFX_NAME, index, sfx_id))
    name, size, loop = entries[sfx_id]
    if int(size) != len(data) or int(loop) != 0xFFFF:
        raise SystemExit('unexpected sounds[] entry for %s: %s' % (SFX_NAME, entries[sfx_id]))
    return data


class Apu:
    """The parts of sound.c's Apu used by the noise channel (sfx_0a never touches the others)."""

    def __init__(self):
        self.r = [0] * 0x18
        self.len = [0] * 4
        self.env_div = [0] * 4
        self.env_level = [0] * 4
        self.env_start = [False] * 4
        self.owned = 0

    def write(self, reg, v):
        if reg >= 0x10 and reg != 0x15:
            raise SystemExit('register $%02X is outside the noise channel' % reg)
        self.r[reg] = v
        if reg < 0x10:
            self.owned |= 1 << (reg >> 2)
        if reg == 0x15:
            for ch in range(4):
                if not v & (1 << ch):
                    self.len[ch] = 0
        elif reg < 0x10 and (reg & 3) == 3:
            ch = reg >> 2
            if ch != 2:
                self.env_start[ch] = True
            if self.r[0x15] & (1 << ch):
                self.len[ch] = LENGTH_TABLE[v >> 3]

    def envelope_clock(self, ch):
        control = self.r[ch * 4]
        if self.env_start[ch]:
            self.env_start[ch] = False
            self.env_level[ch] = 15
            self.env_div[ch] = control & 15
        elif self.env_div[ch]:
            self.env_div[ch] -= 1
        else:
            self.env_div[ch] = control & 15
            if self.env_level[ch]:
                self.env_level[ch] -= 1
            elif control & 0x20:
                self.env_level[ch] = 15

    def clock(self):
        # apu_clock: 4 quarter clocks (envelopes; the triangle counter is not used here), 2 half clocks (length)
        for _ in range(4):
            self.envelope_clock(0)
            self.envelope_clock(1)
            self.envelope_clock(3)
        for _ in range(2):
            for ch in range(4):
                halt = (self.r[8] & 0x80) if ch == 2 else (self.r[ch * 4] & 0x20)
                if self.len[ch] and not halt:
                    self.len[ch] -= 1

    def volume(self, ch):
        if not self.len[ch]:
            return 0
        return self.r[ch * 4] & 15 if self.r[ch * 4] & 0x10 else self.env_level[ch]


class Player:
    def __init__(self, data):
        self.data = data
        self.p = 0
        self.wait = 0
        self.on = True
        self.apu = Apu()
        self.apu.r[0x15] = 0x0F  # player_start: the NES game enables the 4 channels once

    def step(self):
        """player_step: returns False once the sound is over (no clock on that frame)."""
        if not self.on:
            return False
        if self.wait:
            self.wait -= 1
            self.apu.clock()
            return True
        while True:
            if self.p >= len(self.data):
                self.on = False
                return False
            b = self.data[self.p]
            self.p += 1
            if b == 0xFF:
                break
            if b == 0xFE:
                self.wait = self.data[self.p] - 1
                if self.wait < 0:
                    raise SystemExit('wait of 0 frames is not handled')
                self.p += 1
                break
            self.apu.write(b, self.data[self.p])
            self.p += 1
        self.apu.clock()
        return True


class PsgState:
    """sound.c's psg_last cache and the PSG byte encoding of psg_set."""

    def __init__(self):
        self.last = [0xFFFF] * 4

    def set(self, out, ch, period, att):
        v = period << 4 | att
        if v == self.last[ch]:
            return
        if ch < 3 and period != self.last[ch] >> 4:
            out.append(0x80 | ch << 5 | (period & 15))
            out.append((period >> 4) & 0x3F)
        out.append(0x90 | ch << 5 | att)
        self.last[ch] = v


def simulate(data):
    """One list of PSG bytes per 60 Hz frame, following sound_frame and sound_out."""
    player = Player(data)
    psg = PsgState()
    frames = []
    fm_sfx_mask = 0
    while True:
        out = []
        on = player.step()
        mask = player.apu.owned if player.on else 0
        acquired = mask & ~fm_sfx_mask
        for ch in range(4):
            if acquired & (1 << ch):
                psg.last[ch] = 0xFFFF
        if acquired & 8:
            out.append(NOISE_CONTROL)
            psg.last[2] = 0xFFFF
        fm_sfx_mask = mask
        # sound_out, noise channel only. Channel 3 is owned by the effect while it plays;
        # afterwards the (empty) music APU is silent.
        apu = player.apu if player.on and player.apu.owned & 8 else Apu()
        n = NOISE_PERIOD[apu.r[14] & 15] >> 4
        psg.set(out, 2, n if n else 1, 15)
        att = min(ATTENUATION[apu.volume(3)] + 2, 15)
        psg.set(out, 3, 0, att)
        frames.append(out)
        if not on:
            break
    return frames


def build_stream(frames):
    stream = bytearray()
    for writes in frames:
        if len(writes) > 0xFE:
            raise SystemExit('frame with too many PSG bytes')
        stream.append(len(writes))
        stream.extend(writes)
    stream.append(END)
    return stream


def generate(source_dir, output):
    data = read_source(source_dir)
    frames = simulate(data)
    stream = build_stream(frames)
    output.mkdir(parents=True, exist_ok=True)
    lines = ['/* Generated from castlevania-md SFX_0A (NES sound $0A, the whip). Never hand edit.',
             ' * Source: castlevania-md src/music.c, sfx_0a; replayed as src/sound.c does.',
             ' * Generator: tools/build_sfx_md.py. Copyleft: see research/audio/whip-sfx-castlevania.md. */',
             '#include "sfx_md.h"',
             f'const u8 vk_sfx_whip[{len(stream)}] = {{']
    for i in range(0, len(stream), 16):
        lines.append('    ' + ', '.join('0x%02X' % b for b in stream[i:i + 16]) + ',')
    lines.append('};')
    (output / 'sfx_md.c').write_text('\n'.join(lines) + '\n')
    (output / 'sfx_md.h').write_text(
        '/* Generated PSG stream for the whip (castlevania-md SFX_0A). Never hand edit. */\n'
        '#ifndef VK_SFX_MD_H\n'
        '#define VK_SFX_MD_H\n'
        '#include <genesis.h>\n'
        f'extern const u8 vk_sfx_whip[{len(stream)}];\n'
        '#endif\n')
    return len(frames), len(stream)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--source-dir', type=Path, default=DEFAULT_SOURCE)
    p.add_argument('--output', type=Path, default=ROOT / 'sgdk/src/generated')
    a = p.parse_args()
    frames, size = generate(a.source_dir, a.output)
    print('Generated sfx_md: %d frames, %d bytes' % (frames, size))
