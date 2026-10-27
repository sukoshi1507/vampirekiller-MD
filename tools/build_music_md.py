#!/usr/bin/env python3
"""Embed the candidate XGM2 music of the MSX2 PSG capture as a C array for SGDK.

Input: research/audio/psg_sn76489_candidate.xgc, the packed XGC2 blob made from the
SN76489 candidate VGM with SGDK's xgm2tool.jar. The XGM2 (unpacked) form was silent
in the Mega Drive driver, so only the packed blob is embedded.
Output: sgdk/src/generated/music_md.{c,h}. The status is candidate: the track has
not been listened to, and its trigger mapping is not yet established.
"""
import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def generate(source, output):
    data = source.read_bytes()
    if not data:
        raise ValueError(f'{source} is empty')
    output.mkdir(parents=True, exist_ok=True)
    lines = ['/* Generated from research/audio XGC2 candidate. Never hand edit. */',
             '#include "music_md.h"',
             f'const u8 vk_music_test[{len(data)}] = {{']
    for i in range(0, len(data), 16):
        lines.append('    ' + ', '.join('0x%02X' % b for b in data[i:i + 16]) + ',')
    lines.append('};')
    (output / 'music_md.c').write_text('\n'.join(lines) + '\n')
    (output / 'music_md.h').write_text(
        '/* Generated XGC2 candidate music. Never hand edit. */\n'
        '#ifndef VK_MUSIC_MD_H\n'
        '#define VK_MUSIC_MD_H\n'
        '#include <genesis.h>\n'
        f'extern const u8 vk_music_test[{len(data)}];\n'
        '#endif\n')
    return hashlib.sha256(data).hexdigest()


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('source', type=Path, nargs='?',
                   default=ROOT / 'research/audio/psg_sn76489_candidate.xgc')
    p.add_argument('--output', type=Path, default=ROOT / 'sgdk/src/generated')
    a = p.parse_args()
    print('Generated music_md from', a.source, 'sha256', generate(a.source, a.output))
