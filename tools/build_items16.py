#!/usr/bin/env python3
"""Candles, objects, thrown weapons, flames and loot in the 16-bit art (research/gfx16.md, section "Objets 16 bits").

usage: build_items16.py <castlevania-md checkout> [--preview FILE.png]

Writes sgdk/src/generated/items16_md.c/.h: tile arrays of the SAME layout as the MSX arrays (object_tiles_md.c
vk_object_tiles, weapons_md.c, loot_md.c, death_flame_md.c; the candle flames come from the loot balls), so that the
code, the sprite tables and the VRAM cells stay those of the MSX port: only the picture of each 16 x 16 cell
changes. Every cell is drawn with PAL1 (the HUD palette, vk_item16_palette) in 16-bit mode.

- Palette: the HUD uses the entries 1 (08AE), 8 (020E), 14 and 15 (0EEE) of PAL1 (hud_palette of room_view.c) and
  never another one: they keep their colours, the other 11 take the colours below (option 1 of the mock-up of the
  user, one palette for the whole game). Entry 2 stays black (the text background).
- Pictures: sprites of Castlevania-MD (src/level.c obj_N, listed in OBJECTS / WEAPONS) laid on the MSX cell (centre,
  or `feet` for the wall candles) and quantised to the palette; 15 objects without a Castlevania-MD equivalent are
  drawn by hand (ART, rows of letters of LETTERS); the gold cross (05) and the cross icon (1D) keep the diagonal of
  the MSX icon (diag_cross); the three loot / candle balls are 12 x 12 spheres in the style of obj_47.
- Layout: objects: one cell per distinct picture, reached from the MSX object 1..0x22 through vk_item16_object_cell
  (0xFF: no picture, the MSX cell is empty or never drawn); weapons: the 13 frames of vk_weapon_tiles; loot: heart,
  red / skin / white balls, heart (the 5 cells of vk_loot_tiles); the flames of the candles are the balls (one copy);
  death flames: patterns F4 and F8.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import build_enemies16 as be   # noqa: E402  (reading of the CV-MD sprites, distances, placement)

GEN = ROOT / 'sgdk/src/generated'

# ---- palette (PAL1) -----------------------------------------------------------------------------
P1 = {1: be.lvl(0x08AE), 8: be.lvl(0x020E), 14: (7, 7, 7), 15: (7, 7, 7),      # held by the HUD
      2: (0, 0, 0), 3: (2, 3, 4), 4: (4, 0, 0), 5: (5, 1, 1), 6: (2, 1, 0), 7: (5, 3, 2),
      9: (7, 5, 0), 10: (6, 6, 7), 11: (6, 3, 7), 12: (0, 2, 6), 13: (2, 5, 7)}
HUD_ENTRIES = (1, 2, 8, 14, 15)      # 2: black, the text background
# letters of the hand-drawn pictures -> entries
LETTERS = {'K': 2, 's': 3, 'd': 4, 'r': 5, 'b': 6, 't': 7, 'g': 9, 'l': 10, 'p': 11, 'u': 12, 'c': 13,
           'S': 1, 'R': 8, 'W': 14}


def pal_words():
    return [0] + [be.word(P1[i]) for i in range(1, 16)]


def nearest(c):
    return min(range(1, 16), key=lambda i: (be.dist(P1[i], c), i))


# ---- sources ---------------------------------------------------------------------------------------
def msx_arrays():
    def a(f, name):
        return be.nums((GEN / f).read_text(), name)
    return dict(obj=a('object_tiles_md.c', 'vk_object_tiles'), wpn=a('weapons_md.c', 'vk_weapon_tiles'),
                loot=a('loot_md.c', 'vk_loot_tiles'), flame=a('candle_flames_md.c', 'vk_candle_flame_tiles'),
                death=a('death_flame_md.c', 'vk_death_flame_tiles'))


def box(cell):
    return be.ref_box(cell)


def lay(img, ref, mode='centre', mirror=False):
    img = be.crop(img)
    if mirror:
        img = be.mirror(img)
    return be.place(img, 16, 16, ref, mode)


def ramp(img, colours):
    """Recolours an image with a ramp of colours (dark -> light) after its luminance."""
    lums = sorted({sum(p) for row in img for p in row if p})
    out = []
    for row in img:
        o = []
        for p in row:
            if p is None:
                o.append(None)
            elif sum(p) <= 2:
                o.append((0, 0, 0))
            else:
                k = lums.index(sum(p)) / max(len(lums) - 1, 1)
                o.append(colours[min(int(k * len(colours)), len(colours) - 1)])
        out.append(o)
    return out


# ---- hand-drawn pictures (16 x 16, letters of LETTERS) --------------------------------------------------
def mir(half):
    return [h + h[::-1] for h in half]


ART = {}
ART[0x03] = mir([
    "........", "..KKKKKK", ".KRRRRRW", ".KRRRRRW", ".KRWWWWW", ".KRWWWWW", ".KRRRRRW", ".KRRRRRW",
    ".KRRRRRW", "..KRRRRW", "..KdRRRW", "...KdRRR", "....KdRR", ".....KdR", "......KK", "........"])
ART[0x04] = mir([
    "........", ".....KKK", "...KKSSS", "..KSSSSb", "..KSSSSb", "..KSbbbb", "..KSbbbb", "..KSSSSb",
    "..KStSSb", "..KtSSSb", "...KtSSb", "...KttSb", "....KttS", ".....KKK", "........", "........"])
ART[0x09] = [
    "................", "......KKKK......", ".....KcWuuK.....", ".....KcuuuK.....", "......KuuK......",
    ".....KKKKKK.....", "...KKgggggSKK...", "..KgSKKKKKKgtK..", "..KgK......KtbK.",
    "..KgK......KtbK.", "..KgK......KtbK.", "..KgtK....KtbbK.", "...KtgKKKKgtbK..",
    "....KbtgggtbK...", ".....KKKKKKK....", "................"]
ART[0x0A] = mir([
    "..KKKKKK", "..KgggSS", "..KKKKKK", "...KllWl", "...KlllR", "....KllR", ".....KlR", "......KR",
    "......KR", ".....KlR", "....KlRR", "...KlRRR", "...KRRRR", "..KKKKKK", "..KgggSS", "..KKKKKK"])
ART[0x0B] = mir([
    "........", "........", "........", "KKKKK...", "KRRRRKK.", "KRRRRRRK", "KRrrrrRK", "KRrrrrKd",
    "KRRRRRRK", "KRRRRKK.", "KKKKK...", "........", "........", "........", "........", "........"])
ART[0x0C] = mir([
    "........", "..KKKKK.", "..KWWWlK", "..KWWWlK", "..KWWWlK", "..KWWWlK", "..KWWWlK", "..KWWWlK",
    "KKKWWWlK", "KWWWWWlK", "KWWWWllK", "KllllllK", "KssssssK", "KKKKKKKK", "........", "........"])
ART[0x0D] = [
    "................", "..........KKKKK.", ".......KKKKWWWlK", ".....KKWWWWWWllK", "....KWWWWWWWlllK",
    "...KWWWWWWWllsK.", "..KWWWWWWWllssK.", "..KWWWWWllsssK..", ".KWWWWWllsssK...",
    ".KWWWllssK.K....", "KWWllssK........", "KWllsK..........", "KlsK............", ".KK.............",
    "................", "................"]
ART[0x0E] = [
    "................", ".......KK.......", "......KRRK......", ".....KRgRRK.....", ".....KRggRK.....",
    ".....KRggRK.....", "......KRRK......", ".......KK.......", ".......K........", ".....KKKKKK.....",
    ".....KWWWlK.....", ".....KWWWlK.....", ".....KWWWlK.....", ".....KWWWlK.....",
    "....KKKKKKKK....", "................"]
ART[0x0F] = [
    "................", "..KKKKKKKKKKKK..", ".KSSSSSSSSSSSdK.", ".KSSSSSSSSRSRSdK", ".KSSSSSSSSSRSSdK",
    ".KSSSSSSSSRSRSdK", ".KSSSbSSSSSSSSdK", ".KSSbSbSSSSSSSdK", ".KSbSSSbSbSSSSdK",
    ".KSSSSSSSbSSSSdK", ".KSbbSSSSSSbSSdK", ".KSSSbbbbbbSSSdK", ".KSSSSSSSSSSSdK.",
    "..KKKKKKKKKKKK..", "................", "................"]
ART[0x10] = [
    "................", "..KKKKKKKKKKKK..", ".KRRKddddddddK..", ".KRRKdKKKKKKdK..", ".KRRKdKKcKKKdK..",
    ".KRRKdKcccKKdK..", ".KRRKdKKcKKKdK..", ".KRRKdKKcKKKdK..", ".KRRKdKKcKKKdK..",
    ".KRRKdKKKKKKdK..", ".KRRKddddddddK..", ".KrrKKKKKKKKKK..", ".KrrKbbbbbbbbK..",
    "..KKKKKKKKKKKK..", "................", "................"]
ART[0x11] = [
    "................", "..KKKKKKKKKKKK..", ".KllKWWWWWWWWK..", ".KllKWKKKKKKWK..", ".KllKWKKKlKKWK..",
    ".KllKWKKlllKWK..", ".KllKWKKKlKKWK..", ".KllKWKKKlKKWK..", ".KllKWKKKlKKWK..",
    ".KllKWKKKKKKWK..", ".KllKWWWWWWWWK..", ".KssKKKKKKKKKK..", ".KssKllllllllK..",
    "..KKKKKKKKKKKK..", "................", "................"]
KEY = [
    "................", "..........KKKK..", ".........KhhmmK.", ".........KhKKmK.", ".........KmKKsK.",
    "..........KmmsK.", ".........KmsK...", "........KmsK....", ".......KmsK.....", "......KmsKKK....",
    ".....KmsKmmK....", "....KmsKKsK.....", "...KmmKKmK......", "...KsKKKK.......",
    "....KK..........", "................"]


def key_art(h, m, s):
    return [r.replace('h', h).replace('m', m).replace('s', s) for r in KEY]


ART[0x12] = key_art('S', 'R', 'd')       # the red key (three yellow keys)
ART[0x17] = key_art('W', 'g', 't')       # the yellow key
ART[0x18] = key_art('W', 'W', 'l')       # the white key (the placed one stays MSX, composed over the room)
ART[0x19] = [
    "................", "................", "..KKKKKKKKKKKK..", ".KttttttttttttK.", ".KtSSSSSSSSSStK.",
    ".KtttgggggtttK..", ".KKKKKgSgKKKKK..", ".KbbbbggbbbbbK..", ".KbttbgKgbttbK..", ".KbttbgSgbttbK..",
    ".KbttbggbbttbK..", ".KbttbbbbbttbK..", ".KKKKKKKKKKKKK..", "................", "................",
    "................"]


def art(rows):
    assert len(rows) == 16 and all(len(r) == 16 for r in rows)
    return [[0 if ch == '.' else LETTERS[ch] for ch in r] for r in rows]


# ---- diagonal crosses (05 and 1D keep the diagonal of the MSX icon) ------------------------------------------
def diag_cross(body, edge, light, dark):
    """16 x 16 indices of a cross whose long arm runs from the bottom left to the top right (as the MSX icons) and
    whose cross bar runs from the top left to the bottom right; drawn by geometry at 4 x resolution: body inside
    the strokes, `edge` on the outline, `light` along the upper left edge of the body, `dark` along the lower right."""
    S = 4
    cx, cy = 7.5, 8.0
    ux, uy = 0.7071, -0.7071          # long arm: towards the top right
    vx, vy = 0.7071, 0.7071           # cross bar: towards the bottom right
    hw, la, lb, u0, bw = 1.25, 7.4, 7.4, 2.6, 4.6      # half width, arm lengths (u), bar position / half length

    def inside(px, py, grow=0.0):
        x, y = px - cx, py - cy
        u, v = x * ux + y * uy, x * vx + y * vy
        a = abs(v) <= hw + grow and -la - grow <= u <= lb + grow
        b = abs(u - u0) <= hw + grow and abs(v) <= bw + grow
        return a or b

    def cover(x, y, grow):
        n = 0
        for j in range(S):
            for i in range(S):
                if inside(x + (i + .5) / S, y + (j + .5) / S, grow):
                    n += 1
        return n / (S * S)
    out = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            if cover(x, y, 0) >= .5:
                px, py = x + .5, y + .5
                lit = not inside(px - 1, py) or not inside(px, py - 1)
                shade = not inside(px + 1, py) or not inside(px, py + 1)
                out[y][x] = light if lit and not shade else dark if shade and not lit else body
            elif cover(x, y, 1.0) >= .3:
                out[y][x] = edge
    return out


# ---- balls (loot and candle flames): 12 x 12 spheres in the style of CV obj_47 ------------------------------
def sphere(colours, d=12):
    """d x d disc shaded from its upper left; colours: dark -> light (5 levels, the first is the rim)."""
    rows = []
    r = d / 2
    for y in range(d):
        row = []
        for x in range(d):
            dx, dy = x + .5 - r, y + .5 - r
            rad = (dx * dx + dy * dy) ** .5
            if rad > r - .05:
                row.append(None)
                continue
            lx, ly = x + .5 - (r - 1.6), y + .5 - (r - 1.6)      # distance to the light spot
            t = min((lx * lx + ly * ly) ** .5 / (r * 1.45), 1.0)
            k = 4 - int(t * 5)
            if rad > r - 1.15:
                k = 0
            row.append(colours[max(0, min(4, k))])
        rows.append(row)
    return rows


BALLS = {            # name: ramp rim -> highlight (CV obj_47 for the red one)
    'red': [(2, 0, 0), (4, 0, 0), (5, 1, 1), (7, 5, 0), (7, 6, 4)],
    'skin': [(2, 1, 0), (5, 3, 2), (7, 5, 4), (7, 6, 5), (7, 7, 7)],
    'white': [(2, 3, 4), (4, 5, 6), (6, 6, 7), (7, 7, 7), (7, 7, 7)],
}


# ---- what goes where ---------------------------------------------------------------------------------------
# object: (CV obj, mode, mirror, recolour)
OBJECTS = {0x01: (22,), 0x02: (3,), 0x06: (26,), 0x07: (56,),
           0x08: (56, 'centre', False, [(0, 0, 3), (0, 2, 6), (2, 5, 7), (6, 6, 7)]),
           0x13: (62,), 0x14: (23,), 0x16: (39,), 0x1A: (4,), 0x1B: (5,), 0x1C: (32,), 0x1E: (27,),
           0x21: (9, 'feet'), 0x22: (10, 'feet')}
HAND = (0x03, 0x04, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x17, 0x18, 0x19)
DIAG = {0x05: ('g', 'K', 'S', 'b'), 0x1D: ('c', 'l', 'W', 'u')}   # body, edge, light, dark letters
WEAPONS = [(5, True), (5, False), (32, False), (33, False), (34, False), (35, False),
           (36, False), (37, False), (38, False), (37, False), (29, False)]     # frames 0..10 (11, 12: flames)


def two_flames(cv, n):
    p = be.crop(cv.pose(n))
    h = len(p)
    out = [[None] * 16 for _ in range(16)]
    for dx in (0, 8):
        for y in range(min(h, 16)):
            for x in range(len(p[0])):
                out[16 - min(h, 16) + y][dx + x] = p[y][x]
    return out


def quant(img):
    return [[0 if p is None else nearest(p) for p in row] for row in img]


def cell_tiles(rows):
    """16 x 16 indices -> 32 words in the order UL, LL, UR, LR (as the MSX arrays)."""
    return be.cell_tiles(rows)


def build(cv, msx):
    cells = {}                      # (kind, number) -> 16 x 16 indices

    def msx_cell(words, c):
        return be.Msx.cell(words, c)

    def cvpic(n, ref, mode='centre', mirror=False):
        return lay(cv.pose(n), ref, mode, mirror)
    for i, spec in OBJECTS.items():
        n, mode, mirror, rec = (spec + ('centre', False, None))[:4]
        pic = cvpic(n, box(msx_cell(msx['obj'], i - 1)), mode, mirror)
        if rec:
            pic = ramp(pic, rec)
        cells[('obj', i)] = quant(pic)
    for i in HAND:
        cells[('obj', i)] = art(ART[i])
    for i, (body, edge, light, dark) in DIAG.items():
        cells[('obj', i)] = diag_cross(LETTERS[body], LETTERS[edge], LETTERS[light], LETTERS[dark])
    for k, (n, mirror) in enumerate(WEAPONS):
        cells[('wpn', k)] = quant(cvpic(n, box(msx_cell(msx['wpn'], k)), 'centre', mirror))
    cells[('wpn', 11)] = quant(two_flames(cv, 20))
    f55 = [r[:16] for r in be.crop(cv.pose(55))]
    cells[('wpn', 12)] = quant(be.place(f55, 16, 16, box(msx_cell(msx['wpn'], 12)), 'feet'))
    # loot: heart, red / skin / white balls (cells 1..3 of loot_md.c), heart again (cell 4: the constructor placeholder)
    heart = quant(cvpic(22, box(msx_cell(msx['loot'], 0))))
    cells[('loot', 0)] = cells[('loot', 4)] = heart
    for k, name in enumerate(('red', 'skin', 'white')):
        ref = box(msx_cell(msx['flame'], {'red': 2, 'skin': 1, 'white': 0}[name]))
        cells[('loot', k + 1)] = quant(be.place(sphere(BALLS[name]), 16, 16, ref, 'centre'))
    for k, n in enumerate((20, 21)):
        cells[('death', k)] = quant(two_flames(cv, n))
    return cells


def fmt(vals, width, per=8):
    f = '0x%%0%dX' % width
    return '\n'.join('    ' + ','.join(f % v for v in vals[i:i + per]) + ',' for i in range(0, len(vals), per))


def write(cells):
    # objects: one stored cell per distinct picture, the 34 MSX objects reach them through the map
    stored, mapping = [], []
    for i in range(1, 0x23):
        c = cells.get(('obj', i))
        if c is None:
            mapping.append(0xFF)
            continue
        t = cell_tiles(c)
        if t not in stored:
            stored.append(t)
        mapping.append(stored.index(t))
    assert len(stored) < 0xFF
    arrays = [('vk_item16_object_tiles', [w for t in stored for w in t], 'object cells (through vk_item16_object_cell)'),
              ('vk_item16_weapon_tiles', [w for k in range(13) for w in cell_tiles(cells[('wpn', k)])], 'as vk_weapon_tiles'),
              ('vk_item16_loot_tiles', [w for k in range(5) for w in cell_tiles(cells[('loot', k)])],
               'as vk_loot_tiles (heart, red / skin / white balls, heart); the candle flames are cells 3 - colour'),
              ('vk_item16_death_flame_tiles', [w for k in range(2) for w in cell_tiles(cells[('death', k)])],
               'as vk_death_flame_tiles')]
    h = ['/* Generated by tools/build_items16.py (Castlevania-MD art, research/gfx16.md). Same layout as the MSX arrays',
         ' * of object_tiles_md.h, weapons_md.h, loot_md.h and death_flame_md.h; drawn with PAL1 (vk_item16_palette) in 16-bit mode. */',
         '#ifndef VK_ITEMS16_MD_H', '#define VK_ITEMS16_MD_H', '#include <genesis.h>',
         '/* PAL1: the HUD entries (1, 2, 8, 14, 15) as hud_palette of room_view.c, the others the colours of the items. */',
         'extern const u16 vk_item16_palette[16];',
         '/* Cell of vk_item16_object_tiles of the MSX object 1..0x22 (index - 1), 0xFF: no picture. */',
         'extern const u8 vk_item16_object_cell[34];']
    c = ['/* Generated by tools/build_items16.py. */', '#include "items16_md.h"',
         'const u16 vk_item16_palette[16] = {\n%s\n};' % fmt(pal_words(), 4),
         'const u8 vk_item16_object_cell[34] = {\n%s\n};' % fmt(mapping, 2, 17)]
    for name, words, comment in arrays:
        h.append('extern const u32 %s[%d];   /* %s */' % (name, len(words), comment))
        c.append('const u32 %s[%d] = {\n%s\n};' % (name, len(words), fmt(words, 8)))
    h.append('#endif')
    (GEN / 'items16_md.h').write_text('\n'.join(h) + '\n')
    (GEN / 'items16_md.c').write_text('\n'.join(c) + '\n')
    return arrays, len(stored)


def preview(cells, msx, path):
    from PIL import Image, ImageDraw
    S = 4
    items = [('obj', i) for i in range(1, 0x23) if ('obj', i) in cells]
    items += [('wpn', k) for k in range(13)] + [('loot', k) for k in range(5)] + [('death', k) for k in range(2)]
    cols = 10
    cw, ch = 16 * S + 8, 16 * S + 18
    im = Image.new('RGB', (cols * cw, ((len(items) + cols - 1) // cols) * ch), (118, 118, 118))
    d = ImageDraw.Draw(im)
    mp = [None] + [tuple(v * 36 for v in P1[i]) for i in range(1, 16)]
    for n, key in enumerate(items):
        ox, oy = (n % cols) * cw + 4, (n // cols) * ch + 14
        d.text((ox, oy - 12), '%s %02X' % key, fill=(255, 255, 0))
        d.rectangle([ox, oy, ox + 16 * S - 1, oy + 16 * S - 1], fill=(70, 70, 70))
        rows = cells[key]
        for y in range(16):
            for x in range(16):
                if rows[y][x]:
                    d.rectangle([ox + x * S, oy + y * S, ox + x * S + S - 1, oy + y * S + S - 1], fill=mp[rows[y][x]])
    im.save(path)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cv = be.CV(sys.argv[1])
    msx = msx_arrays()
    cells = build(cv, msx)
    arrays, nstored = write(cells)
    print('object cells %d stored, data %d B, palette %s' % (
        nstored, sum(len(w) for _, w, _ in arrays) * 4 + 34 + 32, ' '.join('%03X' % w for w in pal_words())))
    if '--preview' in sys.argv:
        preview(cells, msx, sys.argv[sys.argv.index('--preview') + 1])


if __name__ == '__main__':
    main()
