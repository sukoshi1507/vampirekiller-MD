#!/usr/bin/env python3
"""Bat boss of the stage 3 (room 5) in the 16-bit art (research/gfx16.md, section "Boss chauve-souris 16 bits").

usage: build_boss3_16.py <castlevania-md checkout> [--preview FILE.png]

Writes sgdk/src/generated/boss3_16_md.c/.h: the tiles of the SAME layout as vk_boss_tiles (7 cells of the two body poses
and the fireball, 32 tiles) and vk_orb_tiles (3 poses of 32 x 32, 48 tiles), and vk_boss16_palette, the PAL2 of the boss
room in 16-bit mode. The code (poses, cells, positions, hit boxes, SAT) is the MSX one: only the pictures change.

- Pictures (Castlevania-MD, src/level.c): pose 4E (wings spread, 3 cells on top + body cell) = obj_51 (48 x 32), pose 4F
  (wings folded, 2 cells on top + body cell) = obj_52 (32 x 32). Both fit their cells exactly (the rows below the cells are
  empty in the pictures). The fireball is a red 12 x 12 sphere (build_items16.sphere, the style of obj_47)
  centred on the MSX fireball. The orb (poses 8F 90 91 90 of the MSX animation) = obj_56 (8F, 91) and obj_57 (90, the
  ring of light), centred and standing on the lowest row of the MSX orb.
- Palette: the MSX colours that other PAL2 content uses keep their entry (as vk_enemy16_palette). The orb of the end of the
  fight rewrites the entries 4 and 6 of PAL2 to 006 and 00A (actors.c, VK_BOSS_EV_ORB_TILES, the MSX "room goes dark red"
  effect): in this palette those two entries ALREADY hold 006 and 00A, which are the dark red and the red of the orb and of
  the wings, so the rewrite changes nothing. The entries 5, 9, 10 and 11 take the other colours of the pictures that fit
  best (greedy choice + swaps). The body is gone when the orb tiles load, so no picture sees a colour change.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_enemies16 as E  # noqa: E402
import build_items16 as I  # noqa: E402

GEN = E.GEN
ORB_RED = (0x006, 0x00A)          # entries 4 and 6 after the orb (actors.c)
FREE = (5, 9, 10, 11)


def arr(name, text):
    return E.nums(text, 'const (?:unsigned long|u32|u16) ' + name)


def cells_of(words, ncell_tiles):
    return [words[i * 8:(i + 1) * 8] for i in range(len(words) // 8)]


def tile_rows(words, first, cols, rows):
    """Tiles first.. (column by column, as the MSX arrays) -> rows of indices cols*8 x rows*8."""
    out = [[0] * (cols * 8) for _ in range(rows * 8)]
    i = first
    for c in range(cols):
        for r in range(rows):
            for y in range(8):
                w = words[i * 8 + y]
                for x in range(8):
                    out[r * 8 + y][c * 8 + x] = (w >> (28 - 4 * x)) & 15
            i += 1
    return out


def to_tiles(rows, cols, nrows):
    out = []
    for c in range(cols):
        for r in range(nrows):
            for y in range(8):
                out.append(sum(rows[r * 8 + y][c * 8 + x] << (28 - 4 * x) for x in range(8)))
    return out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cv = E.CV(sys.argv[1])
    boss = arr('vk_boss_tiles', (GEN / 'boss_md.c').read_text())
    orb = arr('vk_orb_tiles', (GEN / 'boss_md.c').read_text())
    msx_pal = E.room_palettes()[19]          # stage 3, room 5
    assert len(boss) == 32 * 8 and len(orb) == 48 * 8

    # ---- pictures on canvases (levels or None) --------------------------------------------------
    pics = {}
    # body: (pose name, obj, canvas size, cells [(cell, x, y)])
    for name, obj, size, cells in (('4E', 51, (48, 32), [(0, 0, 0), (1, 16, 0), (2, 16, 16), (3, 32, 0)]),
                                   ('4F', 52, (32, 32), [(4, 0, 0), (5, 16, 0), (6, 8, 16)])):
        img = cv.pose(obj)
        assert (len(img[0]), len(img)) == size
        covered = {(x + i, y + j) for _, x, y in cells for i in range(16) for j in range(16)}
        lost = [(i, j) for j, r in enumerate(img) for i, p in enumerate(r) if p is not None and (i, j) not in covered]
        assert not lost, (name, lost)           # every pixel of the picture lies in a cell of the pose
        pics[name] = (img, cells, size)
    # fireball: the 12 x 12 sphere of the loot balls (build_items16.sphere, style of obj_47) in the red ramp, centred on the MSX fireball
    ball = I.sphere([(2, 0, 0), (4, 0, 0), (5, 1, 1), (5, 1, 1), (7, 5, 4)])
    ref = E.ref_box(tile_rows(boss, 28, 2, 2))
    pics['fire'] = (E.place(ball, 16, 16, ref, 'centre'), [(7, 0, 0)], (16, 16))
    # orb: obj_56 / obj_57 centred on the MSX orb of pose 8F (bottom row kept: it lands on the floor)
    oref = E.ref_box(tile_rows(orb, 0, 4, 4))
    for k, obj in enumerate((56, 57, 56)):
        img = E.crop(cv.pose(obj))
        pics['orb%d' % k] = (E.place(img, 32, 32, oref, 'feet'), None, (32, 32))

    # ---- palette ---------------------------------------------------------------------------------
    pal = [0] * 16
    for i in E.FIXED:
        pal[i] = msx_pal[i]
    pal[4], pal[6] = ORB_RED
    colours = {}
    for img, _, _ in pics.values():
        for row in img:
            for p in row:
                if p is not None:
                    colours[p] = colours.get(p, 0) + 1
    E.FREE = FREE
    fixed = [E.lvl(pal[i]) for i in E.FIXED + (4, 6)]
    free, err = E.choose_free(colours, fixed)
    for i, c in zip(FREE, free):
        pal[i] = E.word(c)

    # ---- tiles -----------------------------------------------------------------------------------
    def idx(p):
        return 0 if p is None else E.to_index(pal, p)
    body = [0] * (32 * 8)
    for name in ('4E', '4F', 'fire'):
        img, cells, _ = pics[name]
        for c, x, y in cells:
            rows = [[idx(img[y + j][x + i]) for i in range(16)] for j in range(16)]
            body[c * 32:(c + 1) * 32] = to_tiles(rows, 2, 2)
    orb16 = []
    for k in range(3):
        img = pics['orb%d' % k][0]
        orb16 += to_tiles([[idx(p) for p in row] for row in img], 4, 4)

    def fmt(vals, width):
        f = '0x%%0%dX' % width
        return '\n'.join('    ' + ','.join(f % v for v in vals[i:i + 8]) + ',' for i in range(0, len(vals), 8))
    (GEN / 'boss3_16_md.h').write_text('\n'.join([
        '/* Generated by tools/build_boss3_16.py (Castlevania-MD art, research/gfx16.md). Same layout as vk_boss_tiles and',
        ' * vk_orb_tiles (boss_md.h); loaded in their place by actors.c in 16-bit mode, stage 3 boss room. */',
        '#ifndef VK_BOSS3_16_MD_H', '#define VK_BOSS3_16_MD_H', '#include <genesis.h>',
        '/* PAL2 of the boss room: the MSX colours other content uses keep their entries; entries 4 and 6 already hold the colours the orb writes. */',
        'extern const u16 vk_boss16_palette[16];',
        'extern const u32 vk_boss16_tiles[256];   /* as vk_boss_tiles */',
        'extern const u32 vk_orb16_tiles[384];   /* as vk_orb_tiles */', '#endif', '']))
    (GEN / 'boss3_16_md.c').write_text('\n'.join([
        '/* Generated by tools/build_boss3_16.py. */', '#include "boss3_16_md.h"',
        'const u16 vk_boss16_palette[16] = {\n%s\n};' % fmt(pal, 4),
        'const u32 vk_boss16_tiles[256] = {\n%s\n};' % fmt(body, 8),
        'const u32 vk_orb16_tiles[384] = {\n%s\n};' % fmt(orb16, 8), '']))
    print('boss 32 + orb 48 tiles = %d B, palette %s, colour error %.1f/255' % (
        (32 + 48) * 32, ' '.join('%03X' % w for w in pal), err))
    if '--preview' in sys.argv:
        from PIL import Image
        S = 5
        mp = [(0, 0, 0)] + [tuple(v * 36 for v in E.lvl(pal[i])) for i in range(1, 16)]
        mm = [(0, 0, 0)] + [tuple(v * 36 for v in E.lvl(msx_pal[i])) for i in range(1, 16)]
        sets = [(boss, 0, 7 * 4, mm), (body, 0, 7 * 4, mp), (orb, 0, 48, mm), (orb16, 0, 48, mp)]
        im = Image.new('RGB', (1000, 4 * 34 * S), (60, 70, 90))
        for r, (words, _, n, colors) in enumerate(sets):
            x0 = 0
            if n == 28:
                shapes = [(2, 2)] * 8
            else:
                shapes = [(4, 4)] * 3
            first = 0
            for cols, rws in shapes:
                px = tile_rows(words, first, cols, rws)
                first += cols * rws
                for y, row in enumerate(px):
                    for x, v in enumerate(row):
                        if v:
                            for a in range(S):
                                for b in range(S):
                                    im.putpixel((x0 + x * S + a, r * 34 * S + y * S + b), colors[v])
                x0 += cols * 8 * S + 8
        im.save(sys.argv[sys.argv.index('--preview') + 1])


if __name__ == '__main__':
    main()
