#!/usr/bin/env python3
"""Generate the enemy/candle hit sound effect as a per-frame PSG stream for sgdk/src/psg_player.c.

Source: castlevania-md (Mega Drive port of Castlevania NES, no licence published), SFX_14, the NES sound $14
that src/main.c plays on a hit ("a hit", whip_strikes and the enemy hit paths). It is the
fallback chosen for MSX effect 0C (candle/torch hit, research/hit-sound.md): the MSX effect
table could not be decoded (its driver format is unidentified), so the NES sound is used.

The NES data is the APU register script `sfx_14` in src/music.c, the same format as SFX_0A
(see tools/build_sfx_md.py). The script is replayed with the functions of build_sfx_md.py,
which follow src/sound.c, then packed per 60 Hz frame in the stream format of
psg_player.c: a count byte then that many PSG bytes, 0xFF ends the stream.

Only the noise channel is modelled: sfx_14 writes $400C, $400E and $400F only. The replay
fails if the source ever writes another channel.

Usage: build_sfx_hit_md.py [--source-dir DIR] [--output DIR]
Output: sgdk/src/generated/sfx_hit_md.{c,h}, symbol vk_sfx_hit. Never hand edit the generated files.
"""
import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_sfx_md as sfx  # noqa: E402  (same directory: replay model and stream packing)

DEFAULT_SOURCE = ROOT.parent / 'castlevania-md'
SFX_NAME = 'sfx_14'
SFX_ID_DEFINE = 'SFX_14'
SYMBOL = 'vk_sfx_hit'
GUARD = 'VK_SFX_HIT_MD_H'


def generate(source_dir, output):
    sfx.SFX_NAME = SFX_NAME
    sfx.SFX_ID_DEFINE = SFX_ID_DEFINE
    data = sfx.read_source(source_dir)
    frames = sfx.simulate(data)
    stream = sfx.build_stream(frames)
    output.mkdir(parents=True, exist_ok=True)
    lines = ['/* Generated from castlevania-md SFX_14 (NES sound $14, a hit). Never hand edit.',
             ' * Source: castlevania-md src/music.c, sfx_14; replayed as src/sound.c does.',
             ' * Generator: tools/build_sfx_hit_md.py (model: tools/build_sfx_md.py).',
             ' * Used for MSX effect 0C (candle/torch hit): fallback, see research/hit-sound.md. */',
             '#include "sfx_hit_md.h"',
             f'const u8 {SYMBOL}[{len(stream)}] = {{']
    for i in range(0, len(stream), 16):
        lines.append('    ' + ', '.join('0x%02X' % b for b in stream[i:i + 16]) + ',')
    lines.append('};')
    (output / 'sfx_hit_md.c').write_text('\n'.join(lines) + '\n')
    (output / 'sfx_hit_md.h').write_text(
        '/* Generated PSG stream for a hit (castlevania-md SFX_14). Never hand edit. */\n'
        f'#ifndef {GUARD}\n'
        f'#define {GUARD}\n'
        '#include <genesis.h>\n'
        f'extern const u8 {SYMBOL}[{len(stream)}];\n'
        '#endif\n')
    return len(frames), len(stream)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--source-dir', type=Path, default=DEFAULT_SOURCE)
    p.add_argument('--output', type=Path, default=ROOT / 'sgdk/src/generated')
    a = p.parse_args()
    frames, size = generate(a.source_dir, a.output)
    print('Generated sfx_hit_md: %d frames, %d bytes' % (frames, size))
