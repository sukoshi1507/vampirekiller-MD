#!/usr/bin/env python3
"""Title, stage map and ending in the 16-bit art (research/gfx16.md, section "Ecrans 16 bits").

usage: build_screens16.py <castlevania-md checkout> [--preview DIR]

Writes sgdk/src/generated/screens16_md.c/.h:
- title: the film-strip tile of the Castlevania-MD title (one tile, its palette line), and the
  recoloured palette of the MSX band (blue -> darker blue, red, dark grey); the logo keeps the MSX tiles;
- stage map (object 0F): palette of the map, brick texture of the panel (2 x 8 rows of 8 pixels), gilt
  frame cells (tiles around the 208 x 126 panel);
- ending: the Castlevania-MD ending picture (forest, cliff, castle sprites baked in) darkened, as
  plane B tiles behind the credits: tiles, (tile, palette line) pairs, one byte per cell, 3 palette lines.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'sgdk/src/generated'


def lv(v):                      # 8-bit channel -> 3-bit MD level, as the mock-ups' quantizer
    return round(v * 7 / 255)


def word(r, g, b):              # 8-bit RGB -> MD colour word
    return (lv(b) << 9) | (lv(g) << 5) | (lv(r) << 1)


def rgb(w):
    return (((w >> 1) & 7) * 255 // 7, ((w >> 5) & 7) * 255 // 7, ((w >> 9) & 7) * 255 // 7)


class Screens:
    def __init__(self, path):
        self.src = (path / 'src/screens.c').read_text()

    def nums(self, name):
        m = re.search(r'const \w+ %s\[[^\]]*\](?:\[\d+\])? = \{(.*?)\n?\};' % name, self.src, re.S)
        return [int(x, 0) for x in re.findall(r'-?0x[0-9A-Fa-f]+|-?\d+', m.group(1))]

    def screen(self, n):
        s = type('S', (), {})()
        s.tiles, s.pal, s.map = self.nums(n + '_tiles'), self.nums(n + '_palette'), self.nums(n + '_map')
        s.tpx = [[[(s.tiles[t * 8 + y] >> (28 - 4 * x)) & 15 for x in range(8)] for y in range(8)]
                 for t in range(len(s.tiles) // 8)]
        if n == 'ending':
            body = re.search(r'const \w+ ending_sprites\[[^\]]*\] = \{(.*?)\n?\};', self.src, re.S).group(1)
            s.spr = [tuple(int(v, 0) for v in e.split(',')) for e in re.findall(r'\{([^{}]+)\}', body)]
            body = re.search(r'const \w+ ending_lists\[[^\]]*\] = \{(.*?)\n?\};', self.src, re.S).group(1)
            s.lists = [tuple(int(v, 0) for v in e.split(',')) for e in re.findall(r'\{([^{}]+)\}', body)]
            s.frames = self.nums('ending_frames')
        return s


def tile_words(px):
    return [sum(px[y][x] << (28 - 4 * x) for x in range(8)) for y in range(8)]


def fmt(vals, width, per=8):
    f = '0x%%0%dX' % width
    lines = []
    for i in range(0, len(vals), per):
        lines.append('    ' + ','.join(f % v for v in vals[i:i + per]) + ',')
    return '\n'.join(lines)


def title(cv, out):
    t = cv.screen('title')
    # film strip: tile 1, palette line 2 of the NES title
    pal2 = [t.pal[2 * 16 + i] for i in range(16)]
    tile = tile_words(t.tpx[1])
    src = (OUT / 'intro_assets.c').read_text()
    m = re.search(r'scene_palette_1\[16\] = \{(.*?)\};', src, re.S)
    msx = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]
    repl = {word(0, 0, 255): word(36, 36, 146),      # blue -> darker blue
            word(255, 0, 36): word(219, 36, 73),     # red of the letters
            word(36, 36, 36): word(18, 18, 36)}      # shadow of the letters
    pal3 = [repl.get(c, c) for c in msx]
    out['title'] = (tile, pal2, pal3)


def map_screen(out):
    # colours: 3 stone, 4 mortar, 5 highlight, 8 dot and arrow, 9..11 frame (light, middle, dark), 14 walls
    pal = [0] * 16
    pal[3] = word(54, 36, 109)
    pal[4] = word(36, 36, 73)
    pal[5] = word(73, 54, 146)
    pal[8] = word(238, 0, 32)
    pal[9] = word(255, 219, 109)
    pal[10] = word(182, 109, 36)
    pal[11] = word(73, 36, 0)
    pal[14] = word(255, 236, 182)
    # brick texture of the panel: 4 pixel courses, 16 pixel bricks shifted by 8 on odd courses; the panel
    # starts on a cell boundary (x 24, y 48): 2 tiles (parity of the cell column), 8 rows each
    brick = []
    for parity in range(2):
        rows = []
        for line in range(8):
            v = 0
            for x in range(8):
                xo = parity * 8 + x + (8 if line >= 4 else 0)
                if line % 4 == 3 or xo % 16 == 15:
                    c = 4
                elif line % 4 == 0 or xo % 16 == 0:
                    c = 5
                else:
                    c = 3
                v = (v << 4) | c
            rows.append(v)
        brick.append(rows)
    # gilt frame: 3 pixels around the panel [24, 232) x [48, 174); colour from the distance to the panel
    X0, Y0, X1, Y1 = 24, 48, 232, 174

    def pix(x, y):
        dx = max(X0 - x, x - (X1 - 1), 0)
        dy = max(Y0 - y, y - (Y1 - 1), 0)
        d = max(dx, dy)
        return (9, 10, 11)[d - 1] if 1 <= d <= 3 else 0

    def cell(cx, cy):
        return tuple(tuple(pix(cx * 8 + x, cy * 8 + y) for x in range(8)) for y in range(8))

    # ring cells: columns 2 and 29, rows 5 and 22 (screen cells); the two last lines of the panel's last
    # cell row (y 174, 175) are the frame too, painted over the panel tiles at run time
    names = []
    tiles = []
    ring = {}
    for name, (cx, cy) in {'tl': (2, 5), 't': (10, 5), 'tr': (29, 5), 'l': (2, 10), 'r': (29, 10),
                           'bl': (2, 22), 'b': (10, 22), 'br': (29, 22), 'll': (2, 21), 'rl': (29, 21)}.items():
        names.append(name)
        tiles.append([sum(cell(cx, cy)[y][x] << (28 - 4 * x) for x in range(8)) for y in range(8)])
    # the last panel line: y 174, 175 (lines 6 and 7 of cell row 21) over the panel columns
    last = [pix(100, 174), pix(100, 175)]
    out['map'] = (pal, brick, names, tiles, last)


def ending(cv, out):
    e = cv.screen('ending')
    mul = (0.42, 0.45, 0.72)
    lines = {0: 1, 1: 2, 3: 3}                       # NES palette line -> MD line during the credits

    def dark(w):
        if w == 0:
            return 0
        r, g, b = rgb(w)
        return word(int(r * mul[0]), int(g * mul[1]), int(b * mul[2]))
    pals = {l: [dark(e.pal[l * 16 + i]) for i in range(16)] for l in lines}
    # screen rows 0..27 = map rows 1..28; castle: sprite frame 700 (x - 32, y)
    base = {}
    for r in range(28):
        for c in range(32):
            a = e.map[(r + 1) * 32 + c]
            base[(r, c)] = (a & 0x7FF, (a >> 13) & 3)
    first, count = e.lists[e.frames[700]]
    spx = {}                                         # (x, y) -> sprite colour index (NES line 0)
    for x, y, a in e.spr[first:first + count]:
        t, fh, fv = a & 0x7FF, (a >> 11) & 1, (a >> 12) & 1
        for k in range(2):
            for yy in range(8):
                for xx in range(8):
                    v = e.tpx[t + k][7 - yy if fv else yy][7 - xx if fh else xx]
                    if v:
                        spx[(x - 32 + xx, y + (8 * (1 - k) if fv else 8 * k) + yy)] = v
    cells = {}
    for (r, c), (t, p) in base.items():
        touched = any((c * 8 + x, r * 8 + y) in spx for y in range(8) for x in range(8))
        if not touched:
            cells[(r, c)] = (tuple(map(tuple, e.tpx[t])), p)
            continue
        want = []
        for y in range(8):
            for x in range(8):
                v = spx.get((c * 8 + x, r * 8 + y))
                want.append(pals[0][v] if v else pals[p][e.tpx[t][y][x]])
        best = None
        for l in lines:
            cand = []
            err = 0
            for w in want:
                cr, cg, cb = rgb(w)
                d, i = min(((rgb(pals[l][i])[0] - cr) ** 2 + (rgb(pals[l][i])[1] - cg) ** 2 +
                            (rgb(pals[l][i])[2] - cb) ** 2, i) for i in range(16))
                cand.append(i)
                err += d
            if best is None or err < best[0] or (err == best[0] and l == p):
                best = (err, l, cand)
        cells[(r, c)] = (tuple(tuple(best[2][y * 8:y * 8 + 8]) for y in range(8)), best[1])
    tiles, tindex, attrs, aindex, cmap = [], {}, [], {}, []
    for r in range(28):
        for c in range(32):
            px, l = cells[(r, c)]
            if px not in tindex:
                tindex[px] = len(tiles)
                tiles.append(px)
            key = (tindex[px], lines[l])
            if key not in aindex:
                aindex[key] = len(attrs)
                attrs.append(key)
            cmap.append(aindex[key])
    assert len(attrs) < 256
    out['ending'] = ([tile_words(t) for t in tiles], [(t | (l << 13)) for t, l in attrs], cmap,
                     [pals[0], pals[1], pals[3]])
    out['ending_cells'] = (cells, pals, lines)


def preview(out, dirpath):
    from PIL import Image
    dirpath.mkdir(parents=True, exist_ok=True)
    tiles, attrs, cmap, pals = out['ending']
    im = Image.new('RGB', (256, 224))
    for i, a in enumerate(cmap):
        t, at = attrs[a] & 0x1FFF, attrs[a] >> 13
        c, r = i & 31, i >> 5
        for y in range(8):
            for x in range(8):
                v = (tiles[t][y] >> (28 - 4 * x)) & 15
                im.putpixel((c * 8 + x, r * 8 + y), rgb(pals[at - 1][v]))
    im.save(dirpath / 'ending16.png')


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    cv = Screens(Path(args[0]))
    out = {}
    title(cv, out)
    map_screen(out)
    ending(cv, out)
    ttile, tpal2, tpal3 = out['title']
    mpal, brick, names, mtiles, last = out['map']
    etiles, eattr, emap, epal = out['ending']
    nt = len(etiles)
    h = """/* Generated by tools/build_screens16.py (Castlevania-MD art, research/gfx16.md). Read through the
 * screens of title (scene.c), stage map (stage_map_view.c) and ending (dracula_view.c). */
#ifndef VK_SCREENS16_MD_H
#define VK_SCREENS16_MD_H
#include <genesis.h>
/* Title: the film-strip tile (PAL2 of the title) and the recoloured palette of the MSX band (PAL3). */
extern const u32 vk_title16_tile[8];
extern const u16 vk_title16_pal2[16];
extern const u16 vk_title16_pal3[16];
/* Stage map: PAL0 while the map is open; brick of the panel for an even / odd cell column (8 rows);
 * the frame tiles in the order TL T TR L R BL B BR LL RL (colour indices 9, 10, 11); `last` the colour
 * of the lines 174 and 175 of the panel. */
#define VK_MAP16_FRAME_TILES %d
extern const u16 vk_map16_palette[16];
extern const u32 vk_map16_brick[2][8];
extern const u32 vk_map16_frame[VK_MAP16_FRAME_TILES * 8];
#define VK_MAP16_LAST_A %d
#define VK_MAP16_LAST_B %d
/* Ending: %d tiles for plane B, attr = tile | line << 13 (line 1..3, tile from the first tile of the
 * set), one byte per cell for the 32 x 28 cells, the 3 palette lines (PAL1..PAL3). */
#define VK_END16_TILES %d
#define VK_END16_ATTRS %d
extern const u32 vk_end16_tiles[VK_END16_TILES * 8];
extern const u16 vk_end16_attr[VK_END16_ATTRS];
extern const u8 vk_end16_map[32 * 28];
extern const u16 vk_end16_palette[3][16];
#endif
""" % (len(mtiles), last[0], last[1], nt, nt, len(eattr))
    (OUT / 'screens16_md.h').write_text(h)
    c = '/* Generated by tools/build_screens16.py. */\n#include "screens16_md.h"\n'
    c += 'const u32 vk_title16_tile[8] = {\n%s\n};\n' % fmt(ttile, 8)
    c += 'const u16 vk_title16_pal2[16] = {\n%s\n};\n' % fmt(tpal2, 4)
    c += 'const u16 vk_title16_pal3[16] = {\n%s\n};\n' % fmt(tpal3, 4)
    c += 'const u16 vk_map16_palette[16] = {\n%s\n};\n' % fmt(mpal, 4)
    c += 'const u32 vk_map16_brick[2][8] = {\n%s\n};\n' % ('    {' + fmt(brick[0], 8).strip().rstrip(',') + '},\n    {' + fmt(brick[1], 8).strip().rstrip(',') + '}')
    c += 'const u32 vk_map16_frame[VK_MAP16_FRAME_TILES * 8] = {\n%s\n};\n' % fmt([w for t in mtiles for w in t], 8)
    c += 'const u32 vk_end16_tiles[VK_END16_TILES * 8] = {\n%s\n};\n' % fmt([w for t in etiles for w in t], 8)
    c += 'const u16 vk_end16_attr[VK_END16_ATTRS] = {\n%s\n};\n' % fmt(eattr, 4)
    c += 'const u8 vk_end16_map[32 * 28] = {\n%s\n};\n' % fmt(emap, 2, 32)
    c += 'const u16 vk_end16_palette[3][16] = {\n%s\n};\n' % ',\n'.join('    {' + fmt(p, 4).strip().rstrip(',') + '}' for p in epal)
    (OUT / 'screens16_md.c').write_text(c)
    print('title tile 32 B, ending tiles %d (%d B), attrs %d, frame tiles %d' % (nt, nt * 32, len(eattr), len(mtiles)))
    if '--preview' in sys.argv:
        preview(out, Path(sys.argv[sys.argv.index('--preview') + 1]))


if __name__ == '__main__':
    main()
