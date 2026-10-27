#!/usr/bin/env python3
"""Turn an SN76489 VGM capture into a per-frame PSG stream for sgdk/src/psg_player.c.

Input: research/audio/psg_sn76489_candidate.vgm (see tools/convert_psg_vgm.py).
Output: sgdk/src/generated/music_md.{c,h}. Stream format, one entry per frame:
    count, then count raw bytes written to the PSG port.
The entry with count 0xFF ends the stream. PSG latch bytes can take any value,
so no in-band marker is used for frame boundaries. The status is candidate.
The XGM2 driver route was abandoned: its packed blob never silenced a channel.
"""
import argparse
import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
END = 0xFF


def frames_from_vgm(data):
    """Return one list of PSG bytes per 60 Hz frame, from SN76489 VGM commands."""
    offset = struct.unpack_from('<I', data, 0x34)[0]
    i = 0x40 if offset == 0 else 0x34 + offset
    frames = []
    current = []
    while i < len(data):
        cmd = data[i]
        if cmd == 0x50:
            current.append(data[i + 1])
            i += 2
        elif cmd == 0x61:
            # VGM waits are in 44.1 kHz samples; one 60 Hz frame is 735 samples.
            samples = struct.unpack_from('<H', data, i + 1)[0]
            frames.append(current)
            current = []
            for _ in range(max(0, round(samples / 735) - 1)):
                frames.append([])
            i += 3
        elif cmd in (0x62, 0x63):
            frames.append(current)
            current = []
            i += 1
        elif cmd == 0x66:
            break
        else:
            raise ValueError(f'unsupported VGM command 0x{cmd:02X} at 0x{i:X}')
    frames.append(current)
    return frames


def generate(source, output):
    data = source.read_bytes()
    if data[:4] != b'Vgm ':
        raise ValueError(f'{source} is not a VGM file')
    stream = bytearray()
    frames = frames_from_vgm(data)
    for writes in frames:
        if len(writes) > 254:
            raise ValueError('more than 254 PSG writes in one frame')
        stream.append(len(writes))
        stream.extend(writes)
    stream.append(END)
    output.mkdir(parents=True, exist_ok=True)
    lines = ['/* Generated from research/audio SN76489 candidate. Never hand edit. */',
             '#include "music_md.h"',
             f'const u8 vk_music_test[{len(stream)}] = {{']
    for i in range(0, len(stream), 16):
        lines.append('    ' + ', '.join('0x%02X' % b for b in stream[i:i + 16]) + ',')
    lines.append('};')
    (output / 'music_md.c').write_text('\n'.join(lines) + '\n')
    (output / 'music_md.h').write_text(
        '/* Generated PSG stream. Never hand edit. */\n'
        '#ifndef VK_MUSIC_MD_H\n'
        '#define VK_MUSIC_MD_H\n'
        '#include <genesis.h>\n'
        f'extern const u8 vk_music_test[{len(stream)}];\n'
        '#endif\n')
    print(f'{len(frames)} frames, {len(stream)} bytes')
    return hashlib.sha256(bytes(stream)).hexdigest()


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('source', type=Path, nargs='?',
                   default=ROOT / 'research/audio/psg_sn76489_candidate.vgm')
    p.add_argument('--output', type=Path, default=ROOT / 'sgdk/src/generated')
    a = p.parse_args()
    print('Generated music_md from', a.source, 'sha256', generate(a.source, a.output))
