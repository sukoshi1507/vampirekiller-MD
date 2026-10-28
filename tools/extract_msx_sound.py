#!/usr/bin/env python3
"""Extract every Vampire Killer (MSX2) sound by running the ROM's own sound driver.

No openMSX: a Z80 core (kosarev/z80) runs the ROM with the Konami mapper, the way
tools/castle_room_objects.py does. The BIOS entries the driver uses are trapped:
GIWRTE (0093, AY register in A, value in E) is logged, GIRDPSG (0096) reads back
the logged registers. Driver entry points (research/audio-driver.md):
    5076  sound init (C097 = BC mixer, then 5085: stop all, C096 = 0)
    509F  request: A = 01..1C effect, 80..8F music, 00 stop, FB..FF specials
          (FB/FC timed item on channel A, FD/FE pause, FF music fade)
    8964  per-frame update (bank 14 at 8000, bank 15 at A000), called by the
          VBLANK handler 4028 once per frame
A sound is requested once, then 8964 runs once per frame, and the AY writes of
each frame are kept. Effects end when the driver clears C096 (89D7). Music ends
when C0A7 (one bit per music channel, cleared by command FF at 8C80) reaches 0,
or loops: the driver state C010..C0BF is a pure function of the previous state,
so the first repeated state gives the exact loop point.

Output (JSON, one entry per sound): frames = list of [[reg, value], ...] per
frame, loop = frame index the stream jumps back to (null when it ends), plus
note_c = frames where a music note starts on channel C (8AF1 with C095 = 2),
which is when the driver unmutes the channel after an effect (8B00), and
lock = frames after which C0A8 is set (music command E6, cleared by EB): the
request routine refuses every effect while it is set (516B).

Usage: extract_msx_sound.py ROM [--out research/audio/msx_sound_frames.json]
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))

from z80 import Z80Machine  # noqa: E402
from probe import load_rom  # noqa: E402

GIWRTE = 0x0093
GIRDPSG = 0x0096
RETURN = 0x0100
SOUND_INIT = 0x5076
SOUND_REQUEST = 0x509F
SOUND_UPDATE = 0x8964
NOTE_START = 0x8AF1          # a note (byte < C0) was just read for channel C095
SFX_TABLE = 0x8D8D           # bank 14: 2-byte pointer per effect 00..1D
MUSIC_TABLE = 0x8DC9         # bank 14: 3 pointers (channels A, B, C) per music 80..
STATE = (0xC010, 0xC0C0)     # driver variables, enough to define its future
MAX_FRAMES = 30000

EFFECTS = list(range(0x01, 0x1D))
MUSIC = list(range(0x80, 0x8F))
SPECIALS = [0xFB, 0xFD]      # FB: timed item loop on channel A (8C95); FD: pause jingle (9463)


class Driver:
    """The ROM sound driver on a bare Z80: CPU, Konami mapper, two BIOS traps."""

    def __init__(self, data):
        self.data = data
        self.machine = m = Z80Machine()
        for bank in range(4):
            m.set_memory_block(0x4000 + bank * 8192, data[bank * 8192:(bank + 1) * 8192])

        def mapper_write(address, value):
            window = (address - 0x4000) // 8192
            if window not in (1, 2, 3) or value > 15:
                raise ValueError(f'unexpected mapper write {address:04X}={value}')
            m.set_memory_block(0x4000 + window * 8192, data[value * 8192:(value + 1) * 8192])

        m.set_write_callback(mapper_write)
        for window in (0x6000, 0x8000, 0xA000):
            m.mark_addrs(window, 1, m.WRITE_MARK)
        for address in (GIWRTE, GIRDPSG, RETURN, NOTE_START):
            m.set_breakpoint(address)
        self.regs = [0] * 16
        self.writes = []
        self.note_c = False

    def map_sound_banks(self):
        """What 4028 does before CALL 8964: bank 14 at 8000, bank 15 at A000."""
        m = self.machine
        m.set_memory_block(0x8000, self.data[14 * 8192:15 * 8192])
        m.set_memory_block(0xA000, self.data[15 * 8192:16 * 8192])

    def _pop(self):
        m = self.machine
        address = m.memory[m.sp] | m.memory[m.sp + 1] << 8
        m.sp = (m.sp + 2) & 0xFFFF
        return address

    def call(self, address, a=0, limit=2000000):
        m = self.machine
        m.sp = 0xF100
        m.memory[0xF100:0xF102] = bytes((RETURN & 0xFF, RETURN >> 8))
        m.pc = address
        m.a = a
        m.ticks_to_stop = limit
        while True:
            m.run()
            pc = m.pc
            if pc == RETURN:
                return
            if pc == GIWRTE:
                reg, value = m.a & 0x0F, m.e
                self.regs[reg] = value
                self.writes.append((reg, value))
                m.pc = self._pop()
            elif pc == GIRDPSG:
                m.a = self.regs[m.a & 0x0F]
                m.pc = self._pop()
            elif pc == NOTE_START:
                if m.memory[0xC095] == 2:
                    self.note_c = True
                m.step_over_breakpoint()
            elif not m.ticks_to_stop:
                raise RuntimeError(f'cycle limit in routine {address:04X}')
            # Any other stop is the core's end-of-frame event: no interrupt is
            # raised (on_handle_active_int is never called), so just go on.

    def frame(self):
        """One VBLANK: returns the AY writes of this frame and the note-C flag."""
        self.writes = []
        self.note_c = False
        self.map_sound_banks()
        self.call(SOUND_UPDATE)
        return self.writes, self.note_c

    def ram(self, address):
        return self.machine.memory[address]

    def state(self):
        return bytes(self.machine.memory[STATE[0]:STATE[1]])


def run_sound(data, sound, max_frames=MAX_FRAMES):
    """Request one sound from silence and return its frames, loop point and markers."""
    drv = Driver(data)
    drv.call(SOUND_INIT)
    # One idle frame: 5085 points every channel at 89CE, which writes volume 0
    # once and parks the channel; the game has run many frames before a request.
    drv.frame()
    drv.writes = []
    drv.call(SOUND_REQUEST, a=sound)
    request_writes = drv.writes
    frames, note_c, lock, seen = [], [], [], {}
    loop = None
    is_music = sound >= 0x80
    ended = {
        # Effect: 89D7 clears C096 when the effect stream ends.
        'effect': lambda: drv.ram(0xC096) == 0,
        # Music: 8C80 clears one C0A7 bit per channel that reaches command FF.
        'music': lambda: drv.ram(0xC0A7) == 0,
        # FD (pause): the C080 channel ends at 89E9, which points C01A at 89F1.
        'pause': lambda: drv.ram(0xC01A) | drv.ram(0xC01B) << 8 == 0x89F1,
        # FB (timed item) loops on channel A until FC: only a loop ends it.
        'special': lambda: False,
    }
    kind = 'music' if is_music else 'effect'
    if sound == 0xFD:
        kind = 'pause'
    elif sound == 0xFB:
        kind = 'special'
    for index in range(max_frames):
        writes, note = drv.frame()
        if index == 0:
            writes = request_writes + writes
        frames.append([list(w) for w in writes])
        if note:
            note_c.append(index)
        if drv.ram(0xC0A8):
            lock.append(index)
        if ended[kind]():
            break
        if kind in ('effect', 'pause'):
            continue
        key = drv.state()
        if key in seen:
            # The state after frame index equals the state after frame seen[key]:
            # frame index + 1 replays frame seen[key] + 1: frames 0..index are
            # kept and the stream goes on at seen[key] + 1.
            loop = seen[key] + 1
            break
        seen[key] = index
    else:
        raise RuntimeError(f'sound {sound:02X}: no end and no loop in {max_frames} frames')
    return {'id': sound, 'kind': kind,
            'frames': frames, 'loop': loop, 'note_c': note_c, 'lock': lock}


def table_pointers(data):
    def word(cpu):
        o = 14 * 8192 + cpu - 0x8000
        return data[o] | data[o + 1] << 8
    effects = {f'{i:02X}': f'{word(SFX_TABLE + 2 * i):04X}' for i in EFFECTS}
    music = {f'{s:02X}': [f'{word(MUSIC_TABLE + 6 * (s - 0x80) + 2 * c):04X}' for c in range(3)]
             for s in MUSIC}
    return effects, music


def extract(data):
    sounds = [run_sound(data, s) for s in EFFECTS + MUSIC + SPECIALS]
    effects, music = table_pointers(data)
    return {
        'source': 'Vampire Killer (MSX2) ROM, sound driver run by tools/extract_msx_sound.py',
        'rom_sha256': hashlib.sha256(data).hexdigest(),
        'frame_rate': 60,
        'effect_pointers': effects,
        'music_pointers': music,
        'sounds': sounds,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('rom', type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / 'research/audio/msx_sound_frames.json')
    args = parser.parse_args()
    result = extract(load_rom(args.rom))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, separators=(',', ':')) + '\n')
    for s in result['sounds']:
        writes = sum(len(f) for f in s['frames'])
        loop = '-' if s['loop'] is None else s['loop']
        print(f"{s['id']:02X} {s['kind']:6s} frames={len(s['frames']):5d} loop={loop} writes={writes}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
