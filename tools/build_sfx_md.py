#!/usr/bin/env python3
"""Generate the whip sound effect as a per-frame PSG stream for sgdk/src/psg_player.c.

The effect is synthesized, not extracted: the MSX2 capture cannot isolate the whip
(research/audio/whip-sfx-diff.md). It is a white-noise burst on the noise channel
whose volume fades over eight 60 Hz frames. Stream format matches build_music_md.py:
per frame, a count byte then that many PSG bytes; 0xFF ends the stream.
Output: sgdk/src/generated/sfx_md.{c,h}. Placeholder effect, not the original sound.
"""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
END = 0xFF
NOISE_CONTROL = 0xE6  # white noise, shift rate 2 (clock / 256)
ATTENUATION = [0, 2, 4, 6, 8, 10, 12, 15]  # 15 is silent: the burst ends there


def frames():
    out = []
    for n, att in enumerate(ATTENUATION):
        writes = [0xF0 | att]  # noise channel volume latch
        if n == 0:
            writes.insert(0, NOISE_CONTROL)
        out.append(writes)
    return out


def generate(output):
    stream = bytearray()
    for writes in frames():
        stream.append(len(writes))
        stream.extend(writes)
    stream.append(END)
    output.mkdir(parents=True, exist_ok=True)
    lines = ['/* Generated placeholder whip effect (synthesized). Never hand edit. */',
             '#include "sfx_md.h"',
             f'const u8 vk_sfx_whip[{len(stream)}] = {{']
    for i in range(0, len(stream), 16):
        lines.append('    ' + ', '.join('0x%02X' % b for b in stream[i:i + 16]) + ',')
    lines.append('};')
    (output / 'sfx_md.c').write_text('\n'.join(lines) + '\n')
    (output / 'sfx_md.h').write_text(
        '/* Generated PSG stream for the whip. Never hand edit. */\n'
        '#ifndef VK_SFX_MD_H\n'
        '#define VK_SFX_MD_H\n'
        '#include <genesis.h>\n'
        f'extern const u8 vk_sfx_whip[{len(stream)}];\n'
        '#endif\n')
    return len(stream)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--output', type=Path, default=ROOT / 'sgdk/src/generated')
    a = p.parse_args()
    print('Generated sfx_md,', generate(a.output), 'bytes')
