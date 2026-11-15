#!/usr/bin/env python3
"""Simon in the 16-bit art (research/gfx16.md): one 16-bit picture for every MSX picture of Simon.

The port draws Simon from tile arrays decoded from the MSX ROM (24 x 32 poses, 32 x 16 death,
64 x 32 attack and crouch compositions with the lash). This tool writes arrays of the SAME layout
(sgdk/src/generated/simon16_md.c/.h) from the 16-bit frames of assets/gfx16/simon
(tools/import_simon16.py), so that the drawing code only picks another array:

- each MSX picture is read back from its generated array for its geometry (bounding box);
- the 16-bit frame of the same moment is put at the same place: feet on the same row, body on the
  same columns (24 x 32 poses: boxes centred; compositions: body on columns 16..31, mirrored for
  the left-facing ones);
- the lash of the stretched whip is resampled to end on the MSX lash's last column (the hit box is
  the MSX one: the picture must not promise more or less reach);
- the thrown-weapon frames crouched (no NES equivalent) are the crouched whip frames without lash;
- the shield (not in the NES game) is drawn here: a steel bar held in front of Simon;
- the death picture is the walking frame laid down face first.
One palette of 13 colours for all of it (PAL3 entries 1..13; 14 and 15 stay the port's).

Usage: python3 tools/build_simon16.py [--preview FILE.png]
"""
import argparse
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))

from build_gfx16 import pack_palette, distance, rgb3  # noqa: E402

GEN = ROOT / 'sgdk/src/generated'
SRC = ROOT / 'assets/gfx16/simon'
OX, OY = 32, 4                   # Simon's left edge and the sprite origin in the source pictures
COLOURS = 13


def msx_words(path, name):
    text = (GEN / path).read_text()
    m = re.search(r'%s\[[^=]*=\s*\{(.*?)\};' % name, text, re.S)
    return [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]+)', m.group(1))]


def msx_canvas(words, first_tile, tw, th):
    """Sprite tiles are column-major. Returns rows of colour indices."""
    rows = [[0] * (tw * 8) for _ in range(th * 8)]
    for t in range(tw * th):
        cx, cy = t // th, t % th
        for y in range(8):
            row = words[(first_tile + t) * 8 + y]
            for x in range(8):
                rows[cy * 8 + y][cx * 8 + x] = (row >> (28 - 4 * x)) & 15
    return rows


def msx_wide(words, frame):
    a, b = msx_canvas(words, frame * 32, 4, 4), msx_canvas(words, frame * 32 + 16, 4, 4)
    return [ra + rb for ra, rb in zip(a, b)]


# Hit pose variants shown to the user (~/Documents/VampireKiller-MD_maquette_simon16_recul.png).
RECOIL = {'A': ('pose_walk2', 'shear', 0.25), 'B': ('pose_walk2', 'shear', 0.4), 'C': ('pose_stand', 'shear', 0.3),
          'D': ('pose_walk2', 'rotate', 20), 'E': ('pose_crouch', 'rotate', 25)}
RECOIL_CHOICE = 'D'


def box(rows):
    pts = [(x, y) for y, r in enumerate(rows) for x, v in enumerate(r) if v]
    return min(p[0] for p in pts), min(p[1] for p in pts), max(p[0] for p in pts) + 1, max(p[1] for p in pts) + 1


class Art:
    def __init__(self):
        from PIL import Image
        self.Image = Image
        self.cache = {}

    def frame(self, name):
        if name not in self.cache:
            self.cache[name] = self.Image.open(SRC / f'{name}.png').convert('RGBA')
        return self.cache[name]

    def new(self, w, h):
        return self.Image.new('RGBA', (w, h), (0, 0, 0, 0))

    def bottom(self, name):
        return self.frame(name).getbbox()[3]

    def centred(self, name, msx_rows, mirror):
        """24 x 32 pose (or any canvas): boxes centred, feet on the MSX picture's last row."""
        w, h = len(msx_rows[0]), len(msx_rows)
        src = self.frame(name)
        body = src.crop(src.getbbox())
        if mirror:
            body = body.transpose(self.Image.FLIP_LEFT_RIGHT)
        b = box(msx_rows)
        x = (b[0] + b[2] - body.size[0] + 1) // 2
        x = max(0, min(w - body.size[0], x))
        out = self.new(w, h)
        out.paste(body, (x, b[3] - body.size[1]), body)
        return out

    def wide(self, name, msx_rows, feet_of, tip=False, cut=None):
        """64 x 32 right-facing composition: Simon's left edge on column 16, the feet of the pose
        `feet_of` on the MSX picture's last row. tip: resample the lash to the MSX last column.
        cut: drop everything from that column on (no lash)."""
        b = box(msx_rows)
        src = self.frame(name)
        out = self.new(64, 32)
        out.paste(src, (16 - OX, b[3] - self.bottom(feet_of)), src)
        if cut is not None:
            px = out.load()
            for y in range(32):
                for x in range(cut, 64):
                    px[x, y] = (0, 0, 0, 0)
        if tip:
            full = self.new(96, 32)
            full.paste(src, (16 - OX, b[3] - self.bottom(feet_of)), src)
            end = full.getbbox()[2]
            start = 16 + 24                     # the lash starts past the stretched arm
            px, fx = out.load(), full.load()
            for x in range(start, 64):
                sx = start + (x - start) * (end - start) // (b[2] - start) if x < b[2] else None
                for y in range(32):
                    px[x, y] = fx[sx, y] if sx is not None and sx < 96 else (0, 0, 0, 0)
        return out

    def recoil(self, kind):
        """The hit pose (absent from the NES game, drawn here): a walking frame leaned back by a
        shear (top moved away from the facing side), or turned. Kept as frame 'recoil'."""
        name, how, k = RECOIL[kind]
        src = self.frame(name)
        body = src.crop(src.getbbox())
        if how == 'shear':
            w, h = body.size
            ext = int(k * h) + 1
            lean = self.new(w + ext, h)
            for y in range(h):
                lean.paste(body.crop((0, y, w, y + 1)), (ext - int(round((h - 1 - y) * k)), y))
        else:
            lean = body.rotate(k, resample=self.Image.NEAREST, expand=True)
        lean = lean.crop(lean.getbbox())
        out = self.new(96, 44)
        out.paste(lean, (OX, 4 + 32 - lean.size[1]), lean)
        self.cache['recoil'] = out
        return 'recoil'

    def mirror(self, im):
        return im.transpose(self.Image.FLIP_LEFT_RIGHT)

    def shield(self, im, low):
        """A steel bar in front of Simon (he faces right): 5 px wide, 16 high (15 crouched)."""
        o, l, m, d = (0, 0, 0, 255), (216, 216, 252), (144, 144, 180), (72, 108, 108)
        b = im.getbbox()
        h = 15 if low else 16
        x0 = min(im.size[0] - 5, b[2] - 3)
        y0 = b[3] - 17 if low else b[1] + 7
        out = im.copy()
        px = out.load()
        for y in range(h):
            row = [None, o, o, o, None] if y in (0, h - 1) else ([o, m, d, d, o] if y == h // 2 else [o, l, m, d, o])
            for x, c in enumerate(row):
                if c:
                    px[x0 + x, y0 + y] = c
        return out


def build(art):
    """name -> list of RGBA canvases, in the order of the MSX arrays."""
    pose_src = ['pose_stand', 'pose_walk1', 'pose_walk2']
    player_w = msx_words('player_assets.c', 'vk_player_tiles')
    stair_w = msx_words('player_assets.c', 'vk_player_stair_tiles')
    death_w = msx_words('player_assets.c', 'vk_player_death_tiles')
    shield_w = msx_words('player_assets.c', 'vk_player_shield_tiles')
    out = {}
    # 12 poses: walk right 0-2, walk left 3-5, jump 6-7, hit 8-9, landed 10-11 (right, left)
    names = pose_src + pose_src + ['pose_crouch'] * 2 + [art.recoil(RECOIL_CHOICE)] * 2 + ['pose_crouch'] * 2
    left = [0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1]
    out['player'] = [art.centred(n, msx_canvas(player_w, i * 12, 3, 4), left[i]) for i, n in enumerate(names)]
    # stairs: k + 4 x facing, k: up a, up b, down, hit
    stair_src = ['pose_stair_up_a', 'pose_stair_up_b', 'pose_stair_down_b', 'pose_stair_down_b']
    out['stair'] = [art.centred(stair_src[i % 4], msx_canvas(stair_w, i * 12, 3, 4), i >= 4) for i in range(8)]
    # death: the walking frame laid down, head first
    lying = art.frame('pose_walk1')
    lying = lying.crop(lying.getbbox()).rotate(-90, expand=True)
    out['death'] = []
    for side in range(2):
        rows = msx_canvas(death_w, side * 8, 4, 2)
        body = art.mirror(lying) if side else lying
        c = art.new(32, 16)
        c.paste(body, ((32 - body.size[0]) // 2, box(rows)[3] - body.size[1]), body)
        out['death'].append(c)
    # shield: 0-5 walk, 6-7 jump, 8-15 stairs, 16-17 crouched
    shield_names = pose_src + pose_src + ['pose_crouch'] * 2 + stair_src + stair_src + ['pose_crouch'] * 2
    shield_left = [0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1]
    out['shield'] = []
    for i, n in enumerate(shield_names):
        rows = msx_canvas(shield_w, i * 12, 3, 4)
        right = art.shield(art.centred(n, rows if not shield_left[i] else [r[::-1] for r in rows], False),
                           n == 'pose_crouch')
        out['shield'].append(art.mirror(right) if shield_left[i] else right)
    # standing attack, level 0: three frames facing right (flipped by the VDP for the left)
    w = msx_words('simon_attack_md.c', 'vk_simon_attack_tiles')
    out['attack'] = [art.wide(f'whip_{k}', msx_wide(w, k), 'pose_stand', tip=k == 2) for k in range(3)]
    # levels 1 (long whip) and 2.. (thrown weapons): right 0-2, left 3-5 as decoded pictures
    for key, path, name, src in (('whip1', 'whip_level1_md.c', 'vk_whip_level1_tiles', 'whipc0'),
                                 ('whip2', 'whip_level2_md.c', 'vk_whip_level2_tiles', 'throw')):
        w = msx_words(path, name)
        right = [art.wide(f'{src}_{k}', msx_wide(w, k), 'pose_stand', tip=(k == 2 and src != 'throw'))
                 for k in range(3)]
        out[key] = right + [art.mirror(f) for f in right]
    # crouch: body, then the three attack frames of level 0
    w = msx_words('simon_crouch_md.c', 'vk_simon_crouch_tiles')
    out['crouch'] = [art.wide('pose_crouch', msx_wide(w, 0), 'pose_crouch')] + \
        [art.wide(f'cwhip_{k}', msx_wide(w, k + 1), 'pose_crouch', tip=k == 2) for k in range(3)]
    # crouched: level 1 frames 0-2, thrown weapon frames 3-5 (the whip frames without their lash)
    w = msx_words('simon_crouch_levels_md.c', 'vk_simon_crouch_level_tiles')
    out['crouch_levels'] = [art.wide(f'cwhipc0_{k}', msx_wide(w, k), 'pose_crouch', tip=k == 2) for k in range(3)]
    for k in range(3):
        rows = msx_wide(w, 3 + k)
        frame = ('pose_crouch', 'pose_crouch', 'cwhip_2')[k]
        out['crouch_levels'].append(art.wide(frame, rows, 'pose_crouch', cut=box(rows)[2] + 3 if k == 2 else 32))
    return out


def encode(groups):
    weights = Counter()
    for frames in groups.values():
        for im in frames:
            for p in im.getdata():
                if p[3]:
                    weights[rgb3(p[:3])] += 1
    colours, index = pack_palette(weights, COLOURS)
    data = {}
    for key, frames in groups.items():
        words = []
        for im in frames:
            px = im.load()
            tw, th = im.size[0] // 8, im.size[1] // 8
            halves = [(0, tw)] if tw <= 4 else [(0, 4), (4, 8)]     # 64 px: two 32 x 32 sprites
            for x0, x1 in halves:
                for cx in range(x0, x1):
                    for cy in range(th):
                        for y in range(8):
                            v = 0
                            for x in range(8):
                                p = px[cx * 8 + x, cy * 8 + y]
                                v = v << 4 | (index[rgb3(p[:3])] if p[3] else 0)
                            words.append(v)
        data[key] = words
    return data, colours


def write_c(data, colours):
    sizes = {'player': (12, 96), 'death': (2, 64), 'stair': (8, 96), 'shield': (18, 96)}
    c = ['/* Generated by tools/build_simon16.py: Simon in the 16-bit art, same layout as the MSX arrays. Never hand edit. */',
         '#include "simon16_md.h"', '']
    h = ['/* Generated by tools/build_simon16.py (research/gfx16.md). Each array has the layout of the MSX',
         ' * one it stands for (player_assets.h, simon_attack_md.h, whip_level*_md.h, simon_crouch*_md.h).',
         ' * vk_simon16_palette: PAL3 entries 1..13 (0, 14 and 15 are not Simon\'s). */',
         '#ifndef VK_SIMON16_MD_H', '#define VK_SIMON16_MD_H', '#define VK_SIMON16_COLOURS %d' % COLOURS]
    for key, words in data.items():
        if key in sizes:
            n, per = sizes[key]
            assert len(words) == n * per, (key, len(words))
            decl = f'const unsigned long vk_simon16_{key}[{n}][{per}]'
            c.append(decl + ' = {')
            for i in range(n):
                c.append('    {' + ','.join(f'0x{v:08X}' for v in words[i * per:(i + 1) * per]) + '},')
        else:
            decl = f'const unsigned long vk_simon16_{key}[{len(words)}]'
            c.append(decl + ' = {')
            c += ['    ' + ','.join(f'0x{v:08X}' for v in words[i:i + 8]) + ',' for i in range(0, len(words), 8)]
        c.append('};')
        h.append('extern ' + decl + ';')
    pal = [(0, 0, 0)] + colours + [(0, 0, 0)] * (15 - len(colours))
    words = ','.join(f'0x{(b << 9) | (g << 5) | (r << 1):04X}' for r, g, b in pal)
    c.append(f'const unsigned short vk_simon16_palette[16] = {{{words}}};')
    h += ['extern const unsigned short vk_simon16_palette[16];', '#endif']
    (GEN / 'simon16_md.c').write_text('\n'.join(c) + '\n')
    (GEN / 'simon16_md.h').write_text('\n'.join(h) + '\n')


def preview(path, groups):
    from PIL import Image
    rows = list(groups.items())
    width = max(sum(im.size[0] + 2 for im in frames) for _, frames in rows)
    sheet = Image.new('RGB', (width, sum(frames[0].size[1] + 2 for _, frames in rows)), (30, 34, 40))
    y = 0
    for _, frames in rows:
        x = 0
        for im in frames:
            sheet.paste(im, (x, y), im)
            x += im.size[0] + 2
        y += frames[0].size[1] + 2
    sheet.resize((sheet.size[0] * 2, sheet.size[1] * 2), Image.NEAREST).save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--preview', type=Path)
    args = parser.parse_args()
    groups = build(Art())
    data, colours = encode(groups)
    write_c(data, colours)
    if args.preview:
        preview(args.preview, groups)
    print({k: len(v) // 8 for k, v in data.items()}, 'tiles;', len(colours), 'colours')
    return 0


if __name__ == '__main__':
    sys.exit(main())
