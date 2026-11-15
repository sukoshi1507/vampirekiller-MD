#!/usr/bin/env python3
"""Copy the 16-bit Simon frames of Castlevania-MD into assets/gfx16/simon (once).

usage: import_simon16.py <castlevania-md checkout>

Castlevania-MD (same author) holds Simon in the 16-bit art (tiles redrawn by Fiuke Bnuy, given to the
author) as lists of 8x16 sprites in src/level.c (tools/level_to_md.py there). Each frame is drawn here
on a 96 x 44 transparent picture, Simon's left edge at x = 32 and the NES sprite origin at y = 4, so
that tools/build_simon16.py needs nothing but these pictures. obj_0 is the brazier (flame and stand), used
by tools/build_gfx16.py for the courtyard.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FRAMES = ['obj_0', 'pose_stand', 'pose_walk1', 'pose_walk2', 'pose_crouch', 'pose_stair_up_a', 'pose_stair_up_b',
          'pose_stair_down_b'] + [f'{p}_{k}' for p in ('whip', 'whipc0', 'throw', 'cwhip', 'cwhipc0') for k in range(3)]
W, H, OX, OY = 96, 44, 32, 4


def main():
    from PIL import Image
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src = (Path(sys.argv[1]) / 'src/level.c').read_text()

    def array(name):
        m = re.search(r'const u\d+ %s\[\d+\] = \{(.*?)\};' % name, src, re.S)
        return [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]
    tiles, palette = array('level_tiles'), array('level_palette')
    rgb = [((w >> 1 & 7) * 36, (w >> 5 & 7) * 36, (w >> 9 & 7) * 36) for w in palette]
    out = ROOT / 'assets/gfx16/simon'
    out.mkdir(parents=True, exist_ok=True)
    for name in FRAMES:
        m = re.search(r'static const SimonSprite %s\[\d+\] = \{(.*?)\};' % name, src, re.S)
        im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        px = im.load()
        for entry in re.findall(r'\{([^{}]+)\}', m.group(1)):
            dx, dy, attr, tile = (int(v, 0) for v in entry.split(','))
            pal, hflip, vflip = (attr >> 13) & 3, attr & 0x800, attr & 0x1000
            for part in (0, 1):                       # 8x16 sprite: two tiles
                for y in range(8):
                    row = tiles[(tile + (1 - part if vflip else part)) * 8 + (7 - y if vflip else y)]
                    for x in range(8):
                        v = (row >> (28 - 4 * x)) & 15
                        X, Y = OX + dx + (7 - x if hflip else x), OY + dy + part * 8 + y
                        if v and 0 <= X < W and 0 <= Y < H:
                            px[X, Y] = rgb[pal * 16 + v] + (255,)
        im.save(out / f'{name}.png')
    print(len(FRAMES), 'frames ->', out)


if __name__ == '__main__':
    main()
