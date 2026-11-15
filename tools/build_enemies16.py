#!/usr/bin/env python3
"""Common enemies of block 1 (stages 1 to 3) in the 16-bit art (research/gfx16.md, section "Ennemis 16 bits").

usage: build_enemies16.py <castlevania-md checkout> [--preview FILE.png]

Writes sgdk/src/generated/enemies16_md.c/.h: tile arrays of the SAME layout as the MSX arrays of
enemy_sprites_md.c (zombie, bat, panther, fishman with its drops), so that the code, the sprite
table, the hit boxes and the VRAM cells stay those of the MSX port: only the picture of each
cell changes. Each MSX cell is 16 x 16 (4 tiles) and comes in two layers (N, N + 4) whose colours
the VDP ORs on the MSX; the 16-bit picture takes layer N, layer N + 4 stays empty.

- Pictures: the sprites of Castlevania-MD (src/level.c: zombie obj_6 / obj_7, panther obj_11..14,
  bat obj_41..43, fishman obj_44 / obj_45, drops obj_49). The MSX poses keep their logic: pose k of
  the MSX table gets the picture listed in POSES below. Left-facing pictures are mirrored for the
  right-facing cells (and type B zombies), as the MSX arrays are.
- Placement: a picture the size of its cells (zombie, fishman) is laid on them as is; the others
  (panther, bat) are laid with their feet on the lowest row of the MSX picture of that pose
  (centre for the bat) and centred on it.
- Palette (PAL2 of the rooms of stages 1 to 3 in 16-bit mode, vk_enemy16_palette): every colour of
  the MSX room palette that some other PAL2 content uses (objects, candles, flames, weapons, key,
  door, HUD boxes, text) keeps its entry and its colour, so those stay exactly as in MSX mode; the
  entries nothing else of these rooms uses (4, 5, 6, 9, 10, 11) take the 16-bit colours of the
  enemies that fit best (greedy choice + swaps, weighted error). Other colours of the pictures map
  to the nearest of the 15 entries. tests/test_enemies16.py checks the "nothing else uses" part.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'

FIXED = (1, 2, 3, 7, 8, 12, 13, 14, 15)     # MSX colours used by the other PAL2 content: kept
FREE = (4, 5, 6, 9, 10, 11)                 # entries free for the 16-bit colours
W = (3, 4, 2)                               # distance weights r, g, b (as tools/art.py of Castlevania-MD)


def nums(text, name):
    m = re.search(r'%s\[[^\]]*\](?:\[\d+\])?\s*=\s*\{(.*?)\};' % name, text, re.S)
    return [int(x, 0) for x in re.findall(r'-?0x[0-9A-Fa-f]+|-?\d+', m.group(1))]


def lvl(w):                    # MD colour word -> (r, g, b) levels 0..7
    return ((w >> 1) & 7, (w >> 5) & 7, (w >> 9) & 7)


def word(c):
    return (c[2] << 9) | (c[1] << 5) | (c[0] << 1)


def dist(a, b):
    return sum((x - y) ** 2 * k for x, y, k in zip(a, b, W)) * 36 * 36


class CV:
    """Sprites of Castlevania-MD level.c as rows of (r, g, b) levels or None."""

    def __init__(self, root):
        self.text = (Path(root) / 'src/level.c').read_text()
        self.tiles = nums(self.text, 'level_tiles')
        self.pal = [lvl(w) for w in nums(self.text, 'level_palette')]

    def pose(self, n):
        m = re.search(r'static const SimonSprite obj_%d\[\d+\] = \{(.*?)\};' % n, self.text, re.S)
        ents = [tuple(int(v, 0) for v in e.split(',')) for e in re.findall(r'\{([^{}]+)\}', m.group(1))]
        x0 = min(e[0] for e in ents)
        y0 = min(e[1] for e in ents)
        w = max(e[0] for e in ents) + 8 - x0
        h = max(e[1] for e in ents) + 16 - y0
        img = [[None] * w for _ in range(h)]
        for dx, dy, attr, tile in ents:
            line, hf, vf = (attr >> 13) & 3, attr & 0x800, attr & 0x1000
            for part in (0, 1):
                for y in range(8):
                    row = self.tiles[(tile + (1 - part if vf else part)) * 8 + (7 - y if vf else y)]
                    for x in range(8):
                        v = (row >> (28 - 4 * x)) & 15
                        if v:
                            img[dy - y0 + part * 8 + y][dx - x0 + (7 - x if hf else x)] = self.pal[line * 16 + v]
        return img


def mirror(img):
    return [r[::-1] for r in img]


def bbox(img):
    pts = [(x, y) for y, r in enumerate(img) for x, p in enumerate(r) if p is not None]
    if not pts:
        return None
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    return min(xs), min(ys), max(xs) + 1, max(ys) + 1


def crop(img):
    b = bbox(img)
    return [r[b[0]:b[2]] for r in img[b[1]:b[3]]]


# ---- MSX arrays (the layout to keep) ----------------------------------------------------------

class Msx:
    def __init__(self):
        self.text = (GEN / 'enemy_sprites_md.c').read_text()

    def words(self, name):
        return nums(self.text, 'const (?:unsigned long|u32) ' + name)

    @staticmethod
    def cell(words, c):
        out = [[0] * 16 for _ in range(16)]
        for t in range(4):                      # tiles of a cell: UL, LL, UR, LR
            cx, cy = (t // 2) * 8, (t % 2) * 8
            for y in range(8):
                row = words[(c * 4 + t) * 8 + y]
                for x in range(8):
                    out[cy + y][cx + x] = (row >> (28 - 4 * x)) & 15
        return out

    def layers(self, words, a, b):
        A = self.cell(words, a)
        B = self.cell(words, b) if b < len(words) // 32 else [[0] * 16 for _ in range(16)]
        return [[A[y][x] or B[y][x] for x in range(16)] for y in range(16)]


def canvas_of(parts):
    """parts: (x, y, rows of indices) -> rows over their union box (all start at 0, 0)."""
    w = max(x + len(r[0]) for x, y, r in parts)
    h = max(y + len(r) for x, y, r in parts)
    out = [[0] * w for _ in range(h)]
    for x, y, r in parts:
        for j, row in enumerate(r):
            for i, v in enumerate(row):
                if v:
                    out[y + j][x + i] = v
    return out


def ref_box(rows):
    pts = [(x, y) for y, r in enumerate(rows) for x, v in enumerate(r) if v]
    if not pts:
        return None
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    return min(xs), min(ys), max(xs) + 1, max(ys) + 1


def place(img, cw, ch, ref, mode):
    """Rows (ch x cw) of the picture laid on the canvas: `feet` = bottom row of the MSX box, `centre`
    = centre of it; always centred on its columns. A picture as large as the canvas stays as is."""
    h, w = len(img), len(img[0])
    ref = ref or (0, 0, cw, ch)
    x = round((ref[0] + ref[2] - w) / 2)
    y = ref[3] - h if mode == 'feet' else round((ref[1] + ref[3] - h) / 2)
    x = min(max(x, 0), max(cw - w, 0))
    y = min(max(y, 0), max(ch - h, 0))
    out = [[None] * cw for _ in range(ch)]
    for j in range(min(h, ch)):
        for i in range(min(w, cw)):
            out[y + j][x + i] = img[j][i]
    return out


# ---- what goes where ---------------------------------------------------------------------------
# Every job lays one picture on a canvas and writes some cells of it:
#   (array, picture(s), mirrored, mode, crop?, cells of the MSX layers composited as reference,
#    [(cell written, canvas x, canvas y)])
# A cell is 16 x 16; its layer N + 4 (the next cell) is written empty.

def jobs():
    J = []

    def job(arr, obj, mir, mode, crop_, ref, writes, size):
        J.append(dict(arr=arr, obj=obj, mir=mir, mode=mode, crop=crop_, ref=ref, writes=writes, size=size))

    # zombie (array of 12 cells): top A 0/1, bottom A pose A 2/3, pose B 4/5; type B 6/7, 8/9, 10/11 mirrored.
    # Both pictures are 16 x 32 like the cells; the tops of obj_6 and obj_7 are the same.
    for base, mir in ((0, False), (6, True)):
        job('zombie', 6, mir, 'feet', False, [(base, 0, 0), (base + 2, 0, 16)], [(base, 0, 0), (base + 2, 0, 16)], (16, 32))
        job('zombie', 7, mir, 'feet', False, [(base, 0, 0), (base + 4, 0, 16)], [(base + 4, 0, 16)], (16, 32))
    # bat (12 cells): wings flat / half / wide, left 0, 2, 4, right 6, 8, 10 (mirrored); one cell each.
    for obj, left, right in ((42, 0, 6), (43, 2, 8), (41, 4, 10)):
        job('bat', obj, False, 'centre', True, [(left, 0, 0)], [(left, 0, 0)], (16, 16))
        job('bat', obj, True, 'centre', True, [(right, 0, 0)], [(right, 0, 0)], (16, 16))
    # panther (32 cells: 4 poses facing left, 4 facing right, 4 cells each: left half N, N + 2 right half;
    # facing right the left half is the cell N + 2): pose 3F sitting, 40, 41 run, 42 (also the jump).
    for k, obj in enumerate((11, 13, 14, 12)):
        job('panther', obj, False, 'feet', True, [(4 * k, 0, 0), (4 * k + 2, 16, 0)],
            [(4 * k, 0, 0), (4 * k + 2, 16, 0)], (32, 16))
        job('panther', obj, True, 'feet', True, [(16 + 4 * k + 2, 0, 0), (16 + 4 * k, 16, 0)],
            [(16 + 4 * k + 2, 0, 0), (16 + 4 * k, 16, 0)], (32, 16))
    # fishman (21 cells): top left 0/1 (poses 08, 09, 0A), bottoms 2/3 (08, 0A) and 4/5 (09); right: top 10/11,
    # bottoms 12/13 and 14/15 (the right poses 0B, 0D and 0C); drops: cell 20 (pattern 90).
    for mir, top, b0, b1 in ((False, 0, 2, 4), (True, 10, 12, 14)):
        job('fish', 44, mir, 'feet', False, [(top, 0, 0), (b0, 0, 16)], [(top, 0, 0), (b0, 0, 16)], (16, 32))
        job('fish', 45, mir, 'feet', False, [(top, 0, 0), (b1, 0, 16)], [(b1, 0, 16)], (16, 32))
    job('fish', 49, False, 'feet', True, [(20, 0, 0)], [(20, 0, 0)], (16, 16))
    return J


ARRAYS = {   # name: (MSX array, cells, C name, comment)
    'zombie': ('vk_enemy_tiles', 12, 'vk_enemy16_zombie_tiles'),
    'bat': ('vk_enemy4_tiles', 12, 'vk_enemy16_bat_tiles'),
    'panther': ('vk_enemy5_tiles', 32, 'vk_enemy16_panther_tiles'),
    'fish': ('vk_enemy2_tiles', 21, 'vk_enemy16_fish_tiles'),
}


def pictures(cv, msx):
    """Every job's picture laid on its canvas, with colours (levels) or None."""
    out = []
    for j in jobs():
        words = msx.words(ARRAYS[j['arr']][0])
        img = cv.pose(j['obj'])
        if j['crop']:
            img = crop(img)
        if j['mir']:
            img = mirror(img)
        cw, ch = j['size']
        ref = ref_box(canvas_of([(x, y, msx.layers(words, c, c + 1)) for c, x, y in j['ref']]))
        out.append((j, place(img, cw, ch, ref, j['mode'])))
    return out


# ---- palette -----------------------------------------------------------------------------------

def room_palettes():
    t = (GEN / 'rooms.c').read_text()
    m = re.search(r'vk_palettes\[\]\[16\] = \{(.*?)\n\};', t, re.S)
    allp = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]
    st = re.search(r'vk_stages\[18\] = \{(.*?)\};', t, re.S)
    stages = [tuple(int(x) for x in re.findall(r'\d+', e)) for e in re.findall(r'\{([^{}]*)\}', st.group(1))]
    pals = []
    for first, count, _ in stages[:3]:
        pals += [allp[(first + r) * 16:(first + r) * 16 + 16] for r in range(count)]
    return pals


def choose_free(colours, fixed):
    cols = sorted(colours)

    def total(free):
        cents = fixed + list(free)
        return sum(k * min(dist(c, q) for q in cents) ** .5 for c, k in colours.items())
    free = []
    for _ in range(len(FREE)):
        free.append(min((c for c in cols if c not in free), key=lambda c: (total(free + [c]), c)))
    improved = True
    while improved:
        improved = False
        for i in range(len(free)):
            for c in cols:
                if c in free:
                    continue
                t = free[:]
                t[i] = c
                if total(t) < total(free) - 1e-9:
                    free, improved = t, True
    return free, total(free) / sum(colours.values())


def build_palette(pics):
    pals = room_palettes()
    for p in pals:
        assert [p[i] for i in FIXED] == [pals[0][i] for i in FIXED], 'fixed MSX colours differ between rooms'
    pal = [0] * 16
    for i in FIXED:
        pal[i] = pals[0][i]
    colours = {}
    for j, img in pics:
        for row in img:
            for p in row:
                if p is not None:
                    colours[p] = colours.get(p, 0) + 1
    free, err = choose_free(colours, [lvl(pal[i]) for i in FIXED])
    for i, c in zip(FREE, free):
        pal[i] = word(c)
    return pal, err


def to_index(pal, c):
    best = min(range(1, 16), key=lambda i: (dist(lvl(pal[i]), c), i))
    return best


# ---- output ------------------------------------------------------------------------------------

def cell_tiles(rows):
    """16 x 16 indices -> 32 words in the order UL, LL, UR, LR."""
    out = []
    for t in range(4):
        cx, cy = (t // 2) * 8, (t % 2) * 8
        for y in range(8):
            out.append(sum(rows[cy + y][cx + x] << (28 - 4 * x) for x in range(8)))
    return out


def build(cv, msx):
    pics = pictures(cv, msx)
    pal, err = build_palette(pics)
    arrs = {k: [[0] * 32 for _ in range(n)] for k, (_, n, _) in ARRAYS.items()}
    written = {}
    for j, img in pics:
        for c, x, y in j['writes']:
            rows = [[0 if img[y + dy][x + dx] is None else to_index(pal, img[y + dy][x + dx])
                     for dx in range(16)] for dy in range(16)]
            tiles = cell_tiles(rows)
            key = (j['arr'], c)
            assert written.get(key, tiles) == tiles, 'cell %s written twice with other pixels' % (key,)
            written[key] = tiles
            arrs[j['arr']][c] = tiles
    return arrs, pal, err


def fmt(vals, width, per=8):
    f = '0x%%0%dX' % width
    return '\n'.join('    ' + ','.join(f % v for v in vals[i:i + per]) + ',' for i in range(0, len(vals), per))


def write(arrs, pal):
    h = ['/* Generated by tools/build_enemies16.py (Castlevania-MD art, research/gfx16.md). Same layout as the MSX',
         ' * arrays of enemy_sprites_md.h; loaded in their place by actors.c (room_cells) in 16-bit mode, stages 1 to 3. */',
         '#ifndef VK_ENEMIES16_MD_H', '#define VK_ENEMIES16_MD_H', '#include <genesis.h>',
         '/* PAL2 of those rooms: the MSX colours other content uses keep their entries; the free ones hold the 16-bit colours. */',
         'extern const u16 vk_enemy16_palette[16];']
    c = ['/* Generated by tools/build_enemies16.py. */', '#include "enemies16_md.h"',
         'const u16 vk_enemy16_palette[16] = {\n%s\n};' % fmt(pal, 4)]
    for k, (msxname, n, cname) in ARRAYS.items():
        h.append('extern const u32 %s[%d];   /* as %s */' % (cname, n * 32, msxname))
        c.append('const u32 %s[%d] = {\n%s\n};' % (cname, n * 32, fmt([w for cell in arrs[k] for w in cell], 8)))
    h.append('#endif')
    (GEN / 'enemies16_md.h').write_text('\n'.join(h) + '\n')
    (GEN / 'enemies16_md.c').write_text('\n'.join(c) + '\n')


def preview(arrs, pal, msx, path):
    from PIL import Image, ImageDraw
    S = 4
    rows = []
    for k, (msxname, n, _) in ARRAYS.items():
        words = msx.words(msxname)
        for c in range(0, n, 2):
            rows.append((k, c, msx.layers(words, c, c + 1) if c + 1 < n else msx.cell(words, c),
                         Msx.cell([w for cell in arrs[k] for w in cell], c)))
    cols = 8
    nrow = (len(rows) + cols - 1) // cols
    im = Image.new('RGB', (cols * (36 * S + 8), nrow * (18 * S + 14)), (90, 100, 120))
    d = ImageDraw.Draw(im)
    mp = [(0, 0, 0)] + [tuple(v * 36 for v in lvl(pal[i])) for i in range(1, 16)]
    for n, (k, c, a, b) in enumerate(rows):
        ox, oy = (n % cols) * (36 * S + 8), (n // cols) * (18 * S + 14)
        d.text((ox, oy), '%s %d' % (k, c), fill=(255, 255, 255))
        for dx, rows_ in ((0, a), (17 * S + 4, b)):
            for y in range(16):
                for x in range(16):
                    v = rows_[y][x]
                    if v:
                        col = (255, 0, 255) if rows_ is a else mp[v]
                        d.rectangle([ox + dx + x * S, oy + 12 + y * S, ox + dx + x * S + S - 1, oy + 12 + y * S + S - 1], fill=col)
    im.save(path)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cv = CV(sys.argv[1])
    msx = Msx()
    arrs, pal, err = build(cv, msx)
    write(arrs, pal)
    nz = sum(1 for a in arrs.values() for cell in a if any(cell))
    print('cells %d of %d used, data %d B, mean colour error %.1f/255, palette %s' % (
        nz, sum(n for _, n, _ in ARRAYS.values()), sum(n for _, n, _ in ARRAYS.values()) * 128, err,
        ' '.join('%03X' % w for w in pal)))
    if '--preview' in sys.argv:
        preview(arrs, pal, msx, sys.argv[sys.argv.index('--preview') + 1])


if __name__ == '__main__':
    main()
