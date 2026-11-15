#!/usr/bin/env python3
"""16-bit graphics mode (research/gfx16.md): the MSX rooms redrawn with the 16-bit art of
Castlevania-MD (tiles redrawn by Fiuke Bnuy, given to the author of this port).

The MSX layout is kept cell for cell: every MSX tile id of a room gets a 16-bit drawing by a rule
(RULES): a texture of the source pictures (assets/gfx16/*.png) taken at the cell position modulo
the texture period, a picture cell laid over the wall, or a stencil (the MSX tile's own shape,
filled with 16-bit textures). A tile id without a rule keeps its MSX pixels.

Output per room (sgdk/src/generated/gfx16_md.c/.h; the tiles are stored once, in vk_gfx16_pool):
- tiles: slot t is the drawing of MSX tile t at its first place in the block, for the tiles of the
  room and those the game draws by number (hidden blocks, vk_key_block_tiles); the free slots hold the
  variants a room needs (a texture wider than the tile's own period: wallpaper, water);
- map: 32 x 22 slots (the vk_maps layout);
- palette: 15 colours + black at 0, packed from the room picture;
- msx: for each MSX colour of the room palette, the closest colour of that palette.
Slots stop at SLOTS (the HUD frame tiles follow in VRAM).

Usage: python3 tools/build_gfx16.py [ROM_OR_ZIP] [--preview DIR]
"""
import argparse
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))

from probe import load_rom, reference_rom  # noqa: E402
from extract_rooms import RomView, decode_room  # noqa: E402

SLOTS = 180                      # TILE_USER_INDEX + 180 = HUD_FRAME_TILE_INDEX (hud.h)
BLOCKS = {1: (1, 2, 3), 2: (4, 5, 6), 3: (7, 8, 9), 4: (10, 11, 12), 5: (13, 14, 15), 6: (16, 17), 7: (18,)}          # block -> stages done so far


def load_sources():
    from PIL import Image
    return {name: Image.open(ROOT / 'assets/gfx16' / f'{name}.png').convert('RGB').load()
            for name in ('hall16', 'under16', 'court16', 'gate16', 'block2', 'block3', 'block4', 'block5', 'block6', 'block7')}


# Textures: (picture, x, y, period x, period y). Period 1: the 8x8 cell at (x, y) itself.
WALL = ('hall16', 128, 8, 32, 32)        # wallpaper
BRICK = ('under16', 40, 56, 16, 16)      # dark brick wall
CRACK = ('hall16', 8, 116, 16, 16)       # bricks seen through the broken wallpaper
BLOCK_TOP = ('hall16', 0, 168, 16, 1)
BLOCK_LOW = ('hall16', 0, 176, 16, 1)


def block1_rules(rooms):
    """rules[tile id] = (kind, ...). Kinds: wall, tex texture, mask picture cell over the wall,
    stencil {MSX colour: texture} (others: wall colours -> wall, the rest -> CRACK),
    bstencil (brick where the MSX tile has its brick colours, black elsewhere)."""
    r = {}
    for t in (0x0E, 0x0F, 0x10, 0x11):
        r[t] = ('wall',)
    for k in range(4):                    # pillar: 32 px wide, 8 px period, 16 px base
        for base in (0x2C, 0x30, 0x3C):
            r[base + k] = ('tex', ('hall16', 32 + 8 * k, 24, 1, 1))
        r[0x34 + k] = ('tex', ('hall16', 32 + 8 * k, 152, 1, 1))
        r[0x38 + k] = ('tex', ('hall16', 32 + 8 * k, 160, 1, 1))
    for t, k in ((0x48, 0), (0x49, 1), (0x4A, 0), (0x4B, 1)):          # cracked shaft (left half, two rows)
        r[t] = ('tex', ('hall16', 32 + 8 * k, 24, 1, 1))
    for t, dx, dy in ((0x5C, 0, 0), (0x5D, 8, 0), (0x5E, 0, 8), (0x5F, 8, 8)):   # window pane 16x16
        r[t] = ('tex', ('hall16', 96 + dx, 72 + dy, 1, 1))
    for t in (1, 2):
        r[t] = ('tex', BLOCK_TOP)
    for t in (9, 0xA, 0xB):
        r[t] = ('tex', BLOCK_LOW)
    # curtain: the tile ids of the left curtain of stage 1 room 0 (columns 4-7, rows 0-17) take the
    # 16-bit curtain cell at the same place
    grid = rooms[(1, 0)]['grid']
    for cy in range(18):
        for cx in range(4, 8):
            t = grid[cy * 32 + cx]
            if t not in r:
                r[t] = ('mask', ('hall16', 64 + (cx - 4) * 8, 8 + cy * 8, 1, 1))
    for t in range(0x12, 0x18):           # cracks of the wallpaper
        r[t] = ('stencil', {})
    stairs = {11: ('hall16', 0, 170, 16, 8), 14: ('hall16', 0, 168, 16, 8),
              2: ('hall16', 0, 180, 16, 8), 13: ('hall16', 0, 180, 16, 8)}
    for t in (3, 4, 6, 7, 0xC, 0xD):
        r[t] = ('stencil', stairs)
    r[0x41] = ('tex', BRICK)
    for t in (0x42, 0x43, 0x44, 0x45, 0x46, 0x4C, 0x4D, 0x4E):
        r[t] = ('bstencil',)
    r[0x47] = ('tex', ('under16', 160, 112, 8, 8))                # grille
    for t, sy in ((0x29, 152), (0x2A, 160), (0x2B, 168)):          # water
        r[t] = ('tex', ('under16', 0, sy, 32, 1))
    for t, sy in ((0x50, 144), (0x51, 144), (0x53, 136), (0x54, 136)):   # grass on the bank
        r[t] = ('tex', ('under16', 224, sy, 16, 1))
    for t, sx, sy in ((0x55, 32, 96), (0x56, 40, 96), (0x57, 56, 96), (0x58, 32, 104),
                      (0x59, 56, 104), (0x5A, 32, 112), (0x5B, 40, 112)):   # hanging moss
        r[t] = ('tex', ('under16', sx, sy, 1, 1))
    return r


def black_holes(graphics, t):
    """The black pixels of MSX tile t that belong to a large black area (an opening), as opposed
    to the thin black lines between bricks: 4-connected groups that hold a 3 x 3 black square."""
    def colour(x, y):
        b = graphics[t * 32 + y * 4 + x // 2]
        return b & 15 if x & 1 else b >> 4
    seen, holes = set(), set()
    for y0 in range(8):
        for x0 in range(8):
            if (x0, y0) in seen or colour(x0, y0):
                continue
            group, todo = set(), [(x0, y0)]
            while todo:
                x, y = todo.pop()
                if (x, y) in group or not (0 <= x < 8 and 0 <= y < 8) or colour(x, y):
                    continue
                group.add((x, y))
                todo += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
            seen |= group
            # an opening is thick: it holds a 3 x 3 black square (brick joints never do)
            if any(all((x + i, y + j) in group for i in range(3) for j in range(3)) for x, y in group):
                holes |= group
    return holes


def block2_rules(rooms, graphics):
    """Stages 4-6: pink brick wall, grey brick columns, yellow blocks, arches, blue windows. The MSX
    tiles are sorted by their colours (room palette of the block): 9, 11, 13 wall bricks, 12 with them
    the dark broken parts, 4, 6, 10, 14 the yellow blocks and steps, 15 the windows, 8 the red statues
    (kept in MSX pixels). Kind cs: {MSX colour: texture}, the thin black lines get the `mortar`
    texture, the large black areas stay black, any other colour keeps its MSX value."""
    wall, block = ('block2', 64, 0, 16, 16), ('block2', 224, 0, 16, 16)
    rubble, glass = ('block2', 458, 36, 16, 16), ('block2', 652, 34, 8, 8)
    tread, step = ('flat', 252, 216, 180), ('flat', 216, 108, 36)     # steps: light top, orange riser
    used = sorted({t for r in rooms.values() for t in r['grid']})
    r = {}
    for t in used:
        if not t:
            continue
        colours = {(graphics[t * 32 + i] >> 4) for i in range(32)} | {graphics[t * 32 + i] & 15 for i in range(32)}
        holes = black_holes(graphics, t)
        bricks = {9: wall, 11: wall, 13: wall, 12: wall}
        if 8 in colours:
            continue
        if t in (0x13, 0x14, 0x15):
            r[t] = ('tex', ('block2', 192 + 8 * (t - 0x13), 16, 1, 8))
        elif t in (0x01, 0x02, 0x0A, 0x0B):
            r[t] = ('tex', block)
        elif 15 in colours:
            r[t] = ('cs', {**bricks, 15: glass}, wall)
        elif colours & {4, 6, 10, 14}:
            r[t] = ('cs', {**bricks, 4: step, 6: step, 10: step, 14: tread}, None)
        elif colours <= {0, 9, 11, 13}:
            r[t] = ('tex', wall) if not holes else ('cs', bricks, wall)
        else:
            r[t] = ('cs', {9: wall, 11: rubble, 12: rubble, 13: rubble}, wall)
    return r


def tile_colours(graphics, t):
    return {(graphics[t * 32 + i] >> 4) for i in range(32)} | {graphics[t * 32 + i] & 15 for i in range(32)}


def block3_rules(rooms, graphics):
    """Stages 7-9: stone wall (MSX colours 2, 4, 10, 12, 13), moss (9), sky and water (15), round
    orange blocks and steps (6, 11, 14). Everything is the MSX shape filled with the 16-bit stone,
    moss, sky and block of Castlevania-MD's block 3."""
    stone, block, sky = ('block3', 160, 0, 16, 16), ('block3', 0, 0, 16, 16), ('flat', 0, 36, 144)
    moss = ('block3', 48, 16, 16, 16)
    tread, step = ('flat', 252, 216, 180), ('flat', 216, 108, 36)
    fill = {2: stone, 4: stone, 10: stone, 12: stone, 13: stone, 9: moss, 15: sky}
    r = {}
    for t in sorted({t for room in rooms.values() for t in room['grid']}):
        colours = tile_colours(graphics, t)
        if not t or 8 in colours:
            continue
        if t in (0x01, 0x02, 0x0A, 0x0B):
            r[t] = ('tex', block)
        elif colours & {6, 11, 14}:
            r[t] = ('cs', {**fill, 6: step, 11: step, 14: tread}, None)
        elif sum(1 for i in range(32) for v in (graphics[t * 32 + i] >> 4, graphics[t * 32 + i] & 15) if v == 9) >= 6:
            # a mossy tile: the stone under the scattered MSX green pixels is moss too
            r[t] = ('cs', {**{c: moss for c in (2, 4, 10, 12, 13, 9)}, 15: sky}, moss)
        else:
            r[t] = ('cs', fill, stone)
    return r


def block4_rules(rooms, graphics):
    """Stages 10-12: caves (stage 10), open air with ruined columns (11), blue brick building (12).
    Colour 15 is the bright blue of the cave rock and the sky of the open air, and the open air draws
    its mountains with cave rock tiles: the rules depend on the room (open air = tile 6A, the sky).
    Grey stone blocks -> the pink blocks of Castlevania-MD, cave rock -> its blue rock, water -> its
    water, columns -> its brick column, rubble ground -> its grass, building -> its blue bricks;
    skulls (38-3D) and the red striped tiles of room 12-6 keep their MSX pixels."""
    pink, rock = ('block4', 0, 112, 16, 16), ('block4', 448, 0, 32, 32)
    water, grass = ('block4', 0, 144, 32, 16), ('block4', 1664, 112, 32, 32)
    column, bricks = ('block4', 3808, 0, 32, 8), ('block4', 3072, 68, 32, 32)
    sky = ('flat', 0, 36, 144)
    hills = {10: ('flat', 72, 72, 108), 11: ('flat', 144, 144, 180), 15: sky}
    used = sorted({t for room in rooms.values() for t in room['grid']})
    stage = {t: {s for (s, _), room in rooms.items() if t in room['grid']} for t in used}

    def rules(outdoor):
        r = {}
        for t in used:
            colours = tile_colours(graphics, t)
            if not t or 8 in colours or 0x38 <= t <= 0x3D or 0x62 <= t <= 0x69:
                continue
            if t == 0x6A:
                r[t] = ('tex', sky)
            elif t in (0x20, 0x21):
                r[t] = ('cs', {10: water, 11: water, 15: water}, water)
            elif t <= 0x0F:
                r[t] = ('cs', {9: pink, 12: pink, 13: pink, **({15: sky} if outdoor else {})}, pink)
            elif colours <= {0, 10, 11, 15}:
                if outdoor:
                    r[t] = ('cs', hills, None)
                elif stage[t] & {12}:
                    r[t] = ('cs', {10: bricks, 11: bricks, 15: bricks}, bricks)
                else:
                    r[t] = ('cs', {10: rock, 11: rock, 15: rock}, rock)
            elif 0x8D <= t <= 0xA2:
                r[t] = ('cs', {9: grass, 10: grass, 12: grass, 13: grass, 15: sky}, grass)
            elif colours & {12, 14}:
                r[t] = ('cs', {9: column, 12: column, 13: column, 14: column, 15: sky}, None)
        return r
    out, cave = rules(True), rules(False)
    return lambda grid: out if 0x6A in grid else cave


def block5_rules(rooms, graphics):
    """Stages 13-15: brown blocks (01, 02, 0A, 0B, 61, 62), light stone wall of stage 13 (0E-2C, 60), dark
    rubble wall of stages 14-15 (41-57), grey columns (58-5F), grey lattice of the gallery (tiles of
    colours 2 and 13 only). The stones keep their MSX outlines (thin black lines), filled with the
    16-bit stone, rubble or column; chains, skeletons, bones, grates, windows, coffins, the gallery's
    white columns and the portraits keep their MSX pixels."""
    block, stone = ('block5', 0, 144, 16, 16), ('block5', 40, 88, 16, 16)
    rubble, column = ('block5', 1888, 80, 32, 32), ('block5', 0, 0, 32, 16)
    lattice = ('block5', 4416, 64, 24, 24)
    r = {}
    for t in sorted({t for room in rooms.values() for t in room['grid']}):
        colours = tile_colours(graphics, t)
        if t in (0x01, 0x02, 0x0A, 0x0B, 0x61, 0x62):
            r[t] = ('tex', block)
        elif 0x0E <= t <= 0x2C or t == 0x60:
            r[t] = ('cs', {11: stone, 12: stone, 13: stone}, None)
        elif 0x41 <= t <= 0x57 and colours <= {0, 11, 12, 13, 14}:
            r[t] = ('cs', {11: rubble, 12: rubble, 13: rubble}, None)
        elif 0x58 <= t <= 0x5F:
            r[t] = ('cs', {11: column, 12: column, 13: column, 14: column}, None)
        elif 0x8E <= t and colours <= {0, 2, 13}:
            r[t] = ('cs', {2: lattice, 13: lattice}, lattice)
    return r


def block6_rules(rooms, graphics):
    """Stages 16-17: grey stone towers (stage 16 under the sky, stage 17 inside), orange blocks (01, 02,
    0A, 0B), moss (21-23), sky (colour 15). Stone tiles are shared by both stages: a room with sky
    (tile 13) takes the lilac stone of Castlevania-MD's block 6 outside part, the others its grey
    stone. Stones keep their MSX outlines. The bridge, railings, windows, gears and pipes keep their
    MSX pixels."""
    block, moss = ('block6', 2048, 120, 32, 32), ('block6', 704, 48, 32, 32)
    lilac, grey = ('block6', 672, 138, 32, 32), ('block6', 1648, 144, 32, 32)
    sky = ('flat', 0, 36, 144)
    used = sorted({t for room in rooms.values() for t in room['grid']})
    stone = [t for t in used if 0x10 <= t <= 0x41 and t not in range(0x21, 0x24) and t not in range(0x26, 0x2B)
             and t not in range(0x2C, 0x2F) or t in (0x50, 0x51, 0x52, 0x53, 0x60, 0x61, 0x62, 0x63, 0x74)]

    def rules(outside):
        wall = lilac if outside else grey
        r = {}
        for t in used:
            colours = tile_colours(graphics, t)
            if t in (0x01, 0x02, 0x0A, 0x0B):
                r[t] = ('tex', block)
            elif 0x21 <= t <= 0x23:
                r[t] = ('cs', {4: moss, 9: moss, 11: moss, 15: sky}, moss)
            elif colours <= {0, 15}:
                r[t] = ('cs', {15: sky}, None)
            elif t in stone and colours <= {0, 6, 11, 12, 13, 15}:
                r[t] = ('cs', {6: wall, 11: wall, 12: wall, 13: wall, 15: sky}, None)
        return r
    out, inside = rules(True), rules(False)
    return lambda grid: out if 0x13 in grid else inside


def block7_rules(rooms, graphics):
    """Stage 18: grey brick tower (tiles of colours 9, 10, 12) -> the blue bricks of Castlevania-MD's
    clock tower, sky (colour 15) -> its night sky. The red platforms and pillars (01-08, 0C-0D,
    11-12, 14-15), the moon and the steps keep their MSX pixels: only the backdrop changes (asked
    by the tester after a run, 2026-11-15)."""
    bricks = ('block7', 1504, 0, 32, 32)
    sky = ('flat', 0, 36, 144)
    r = {}
    for t in sorted({t for room in rooms.values() for t in room['grid']}):
        colours = tile_colours(graphics, t)
        if t in (0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0C, 0x0D, 0x11, 0x12, 0x14, 0x15):
            continue
        elif t and colours <= {0, 15}:
            r[t] = ('cs', {15: sky}, None)
        elif 0x16 <= t <= 0x37 or t == 0x3E:
            r[t] = ('cs', {9: bricks, 10: bricks, 12: bricks, 15: sky}, bricks)
    return r


RULES = {1: lambda rooms, graphics: block1_rules(rooms), 2: block2_rules, 3: block3_rules, 4: block4_rules,
         5: block5_rules, 6: block6_rules, 7: block7_rules}


def tex(src, texture, X, Y):
    if texture[0] == 'flat':            # ('flat', r, g, b): one colour
        return texture[1:]
    name, sx, sy, px, py = texture
    return src[name][sx + X % px + (0 if px > 1 else X % 8), sy + Y % py + (0 if py > 1 else Y % 8)]


class Skin:
    def __init__(self, src, rules, graphics):
        self.src, self.rules, self.graphics = src, rules, graphics
        self.holes = {t: black_holes(graphics, t) for t, r in rules.items() if r[0] == 'cs'}
        self.wall_colours = {tex(src, WALL, x, y) for x in range(32) for y in range(32)}

    def msx(self, t, x, y):
        b = self.graphics[t * 32 + y * 4 + x // 2]
        return b & 15 if x & 1 else b >> 4

    def colours(self, t):
        return {self.msx(t, x, y) for x in range(8) for y in range(8)}

    def cell(self, t, cx, cy, paper, msx_rgb):
        """The 64 RGB pixels of MSX tile t drawn at cell (cx, cy)."""
        rule = self.rules.get(t)
        base = WALL if paper else BRICK
        out = []
        for y in range(8):
            for x in range(8):
                X, Y = cx * 8 + x, cy * 8 + y
                if rule is None:
                    p = msx_rgb[self.msx(t, x, y)]
                elif rule[0] == 'wall':
                    p = tex(self.src, base, X, Y)
                elif rule[0] == 'tex':
                    p = tex(self.src, rule[1], X, Y)
                elif rule[0] == 'mask':
                    p = tex(self.src, rule[1], X, Y)
                    if p in self.wall_colours:
                        p = tex(self.src, base, X, Y)
                elif rule[0] == 'cs':
                    v = self.msx(t, x, y)
                    if not v:
                        p = (0, 0, 0) if (x, y) in self.holes[t] or rule[2] is None else tex(self.src, rule[2], X, Y)
                    elif v in rule[1]:
                        p = tex(self.src, rule[1][v], X, Y)
                    else:
                        p = msx_rgb[v]
                elif rule[0] == 'stencil':
                    v = self.msx(t, x, y)
                    if v in rule[1]:
                        p = tex(self.src, rule[1][v], X, Y)
                    elif v in self.wall_idx:
                        p = tex(self.src, base, X, Y)
                    else:
                        p = tex(self.src, CRACK, X, Y)
                else:                     # bstencil
                    v = self.msx(t, x, y)
                    p = tex(self.src, BRICK, X, Y) if v and v in self.brick_idx else (0, 0, 0)
                out.append(p)
        return tuple(out)


def key_block_tiles(stages):
    """vk_key_block_tiles (generated/candles_md.c): the 16 tile ids of the hidden block of each stage."""
    import re
    text = (ROOT / 'sgdk/src/generated/candles_md.c').read_text()
    body = re.search(r'vk_key_block_tiles\[19\]\[16\]\s*=\s*\{(.*?)\};', text, re.S).group(1)
    rows = re.findall(r'\{([^{}]*)\}', body)
    return {int(v, 0) for stage in stages for v in re.findall(r'0x[0-9A-Fa-f]+|\d+', rows[stage])}


def distance(a, b):
    return 3 * (a[0] - b[0]) ** 2 + 4 * (a[1] - b[1]) ** 2 + 2 * (a[2] - b[2]) ** 2


def pack_palette(weights, size):
    """Ward-style merge of the closest colours until `size` are left (as Castlevania-MD's art.py).
    Returns (colours, {colour: index from 1})."""
    groups = [[{c}, c, w] for c, w in weights.items()]
    while len(groups) > size:
        best = None
        for i in range(len(groups)):
            for j in range(i + 1, len(groups)):
                a, b = groups[i], groups[j]
                cost = a[2] * b[2] / (a[2] + b[2]) * distance(a[1], b[1])
                if best is None or cost < best[0]:
                    best = (cost, i, j)
        _, i, j = best
        a, b = groups[i], groups[j]
        w = a[2] + b[2]
        mean = tuple(round((a[1][k] * a[2] + b[1][k] * b[2]) / w) for k in range(3))
        groups[i] = [a[0] | b[0], mean, w]
        del groups[j]
    groups.sort(key=lambda g: -g[2])
    return [g[1] for g in groups], {m: n + 1 for n, g in enumerate(groups) for m in g[0]}


def rgb3(p):
    """Picture colour (8 bits per channel) -> 3 bits per channel."""
    return tuple(min(7, (v + 18) // 36) for v in p)


def build_block(data, block, src):
    stages = BLOCKS[block]
    rooms, graphics = {}, None
    for stage in stages:
        room = 0
        layout = RomView(data, (0, 11, 12, 13))
        offsets = list(layout.read(0x6000, 19)) + [156]      # as room_index
        count = offsets[stage + 1] - offsets[stage]
        for room in range(count):
            manifest, graphics, tilemap = decode_room(data, stage, room)
            if manifest['unresolved_tile_ids']:
                continue
            rooms[(stage, room)] = {'grid': tilemap[64:], 'palette': manifest['palette_rgb333']}
    rules = RULES[block](rooms, graphics)        # a rule table, or grid -> rule table
    skins = {}

    def skin_of(grid):
        table = rules(grid) if callable(rules) else rules
        if id(table) not in skins:
            skins[id(table)] = skin = Skin(src, table, graphics)
            if block == 1:
                skin.wall_idx = skin.colours(0x0E) | skin.colours(0x11)
                skin.brick_idx = skin.colours(0x41)
        return skins[id(table)]
    keep = key_block_tiles(stages)        # MSX tile ids the game draws by number (hidden blocks)
    first = {}                           # tile id -> its first cell in the block
    for key in sorted(rooms):
        for i, t in enumerate(rooms[key]['grid']):
            first.setdefault(t, (i % 32, i // 32))
    out = {}
    for key in sorted(rooms):
        grid, palette = rooms[key]['grid'], rooms[key]['palette']
        msx_rgb = [tuple(v * 36 for v in c) for c in palette]
        paper = block == 1 and 0x0E in grid
        skin = skin_of(grid)
        canon = {t: skin.cell(t, *first.get(t, (0, 0)), paper, msx_rgb) for t in range(192)}
        cells = [skin.cell(t, i % 32, i // 32, paper, msx_rgb) for i, t in enumerate(grid)]
        # palette from the room picture
        weights = Counter(rgb3(p) for c in cells for p in c)
        weights.pop((0, 0, 0), None)
        colours, index = pack_palette(weights, 15)
        index[(0, 0, 0)] = 0
        pal = [(0, 0, 0)] + colours + [(0, 0, 0)] * (15 - len(colours))

        def encode(pixels):
            def near(c):
                if c not in index:
                    index[c] = 1 + min(range(len(colours)), key=lambda n: distance(colours[n], c))
                return index[c]
            v = [near(rgb3(p)) for p in pixels]
            return tuple(sum(v[y * 8 + x] << (28 - 4 * x) for x in range(8)) for y in range(8))
        used = set(grid)
        slots = {t: encode(canon[t]) for t in range(192) if t in used or t in keep}
        free = [s for s in range(1, SLOTS) if s not in slots]
        variant, room_map = {}, []
        for t, c in zip(grid, cells):
            e = encode(c)
            if e == slots[t]:
                room_map.append(t)
                continue
            if e not in variant:
                same = [s for s, d in slots.items() if d == e and s < SLOTS]
                if same:
                    variant[e] = same[0]
                else:
                    if not free:
                        raise ValueError(f'room {key}: more than {SLOTS} tiles')
                    variant[e] = free.pop(0)
                    slots[variant[e]] = e
            room_map.append(variant[e])
        if max(room_map) >= SLOTS or any(t >= SLOTS for t in used):
            raise ValueError(f'room {key}: tile slot beyond {SLOTS}')
        count = max(s for s in slots if s < SLOTS) + 1
        tiles = [slots.get(s, (0,) * 8) for s in range(count)]
        msx = [0] + [1 + min(range(len(colours)), key=lambda n: distance(colours[n], tuple(c)))
                     if tuple(c) != (0, 0, 0) else 0 for c in palette[1:]]
        out[key] = {'tiles': tiles, 'map': room_map, 'palette': pal, 'msx': msx,
                    'variants': len(variant), 'cells': cells}
    return out


def courtyard_pictures(src):
    """The three courtyard screens (ROM scenes 3, 4, 5) as 256 x 176 pictures: the 16-bit courtyard of
    Castlevania-MD with its floor on the MSX ground line (room y 144), a brazier stand under each
    MSX torch (x 64 and 192, the flame is the game's sprite), and for the third screen the castle
    tower with its doorway on the MSX door (x 224..239)."""
    from PIL import Image
    stand = Image.open(ROOT / 'assets/gfx16/simon/obj_0.png').convert('RGBA').crop((32, 20, 48, 36))

    def picture(columns, torches):
        im = Image.new('RGB', (256, 176))
        px = im.load()
        for x in range(256):
            name, sx = columns(x)
            for y in range(176):
                px[x, y] = src[name][sx, y]
        for tx in torches:
            im.paste(stand, (tx, 128), stand)
        return im
    return {
        ('scene', 3): picture(lambda x: ('court16', x), (64, 192)),
        ('scene', 4): picture(lambda x: ('court16', 256 + x), (64, 192)),
        ('scene', 5): picture(lambda x: ('court16', 62 + x) if x < 194 else ('gate16', x - 50), (64,)),
    }


def encode_picture(im):
    """A free picture -> unique tiles (slot 0 empty), 32 x 22 map, palette."""
    px = im.load()
    weights = Counter(rgb3(px[x, y]) for x in range(256) for y in range(176))
    weights.pop((0, 0, 0), None)
    colours, index = pack_palette(weights, 15)
    index[(0, 0, 0)] = 0
    tiles, slot, room_map = [(0,) * 8], {(0,) * 8: 0}, []
    for cy in range(22):
        for cx in range(32):
            t = tuple(sum(index[rgb3(px[cx * 8 + x, cy * 8 + y])] << (28 - 4 * x) for x in range(8))
                      for y in range(8))
            if t not in slot:
                slot[t] = len(tiles)
                tiles.append(t)
            room_map.append(slot[t])
    if len(tiles) > SLOTS:
        raise ValueError(f'picture: {len(tiles)} tiles, more than {SLOTS}')
    pal = [(0, 0, 0)] + colours + [(0, 0, 0)] * (15 - len(colours))
    return {'tiles': tiles, 'map': room_map, 'palette': pal, 'msx': [0] * 16, 'variants': 0}


def room_index(data):
    """(stage, room) -> index in vk_maps (tools/extract_rooms.py order)."""
    layout = RomView(data, (0, 11, 12, 13))
    offsets = list(layout.read(0x6000, 19)) + [156]
    index, n = {}, 0
    for stage in range(1, 19):
        for room in range(offsets[stage + 1] - offsets[stage]):
            manifest, _, _ = decode_room(data, stage, room)
            if not manifest['unresolved_tile_ids']:
                index[(stage, room)] = n
                n += 1
    return index, n


def write_c(out_dir, rooms, index, total):
    """Every distinct 16-bit tile once, in vk_gfx16_pool (in order of first use, so the new tiles of a
    room follow each other); a room keeps the pool index of each of its slots, and its map as bytes
    (slots < 256). About 60 % of the room tiles are shared with other rooms of the same art."""
    keys = sorted(rooms, key=str)
    pool, where = [], {}
    for k in keys:
        for t in rooms[k]['tiles']:
            if t not in where:
                where[t] = len(pool)
                pool.append(t)
    if len(pool) > 0xFFFF:
        raise ValueError(f'pool: {len(pool)} tiles, more than a u16 index')
    c = ['/* Generated by tools/build_gfx16.py: the rooms in the 16-bit art (research/gfx16.md). Never hand edit. */',
         '#include "gfx16_md.h"', '',
         f'const u32 vk_gfx16_pool[{len(pool) * 8}] = {{']
    c += ['    ' + ','.join(f'0x{v:08X}' for v in t) + ',' for t in pool]
    c += ['};', '']
    for k in keys:
        stage, room = k
        r = rooms[k]
        name = f'g16_{stage}_{room}'
        if max(r['map']) > 255:
            raise ValueError(f'{name}: slot {max(r["map"])} does not fit a byte')
        c.append(f'static const u16 {name}_slots[{len(r["tiles"])}] = {{')
        ids = [where[t] for t in r['tiles']]
        c += ['    ' + ','.join(str(v) for v in ids[i:i + 16]) + ',' for i in range(0, len(ids), 16)]
        c.append('};')
        c.append(f'static const u8 {name}_map[704] = {{')
        c += ['    ' + ','.join(str(v) for v in r['map'][i:i + 32]) + ',' for i in range(0, 704, 32)]
        c.append('};')
        words = ','.join(f'0x{(b << 9) | (g << 5) | (rr << 1):04X}' for rr, g, b in r['palette'])
        c.append(f'static const VKGfx16Room {name} = {{{name}_slots, {name}_map, {len(r["tiles"])}, '
                 f'{{{words}}}, {{{",".join(str(v) for v in r["msx"])}}}}};')
    c.append('const VKGfx16Room *const vk_gfx16_scenes[3] = {&g16_scene_3, &g16_scene_4, &g16_scene_5};')
    c.append(f'const VKGfx16Room *const vk_gfx16_rooms[VK_GFX16_ROOMS] = {{')
    by_index = {index[k]: k for k in rooms if k[0] != 'scene'}
    for n in range(total):
        c.append(f'    &g16_{by_index[n][0]}_{by_index[n][1]},' if n in by_index else '    0,')
    c.append('};')
    h = f'''/* Generated by tools/build_gfx16.py. One entry per room of vk_maps (0: no 16-bit art yet).
 * slots: `count` tiles loaded at TILE_USER_INDEX in place of the MSX tile set, each given by its
 * index in vk_gfx16_pool (slot t < 192 is the 16-bit drawing of MSX tile t, the others are
 * variants); map: the 32 x 22 cells (slots, one byte each); palette: PAL0; msx[c]: the palette
 * entry closest to MSX colour c of the room palette. Read them through gfx16.h. */
#ifndef VK_GFX16_MD_H
#define VK_GFX16_MD_H
#include <genesis.h>
#define VK_GFX16_ROOMS {total}
#define VK_GFX16_SLOTS {SLOTS}
#define VK_GFX16_POOL {len(pool)}
typedef struct {{ const u16 *slots; const u8 *map; u16 count; u16 palette[16]; u8 msx[16]; }} VKGfx16Room;
extern const u32 vk_gfx16_pool[VK_GFX16_POOL * 8];
extern const VKGfx16Room *const vk_gfx16_rooms[VK_GFX16_ROOMS];
/* The courtyard screens (ROM scenes 3, 4, 5): free pictures, no MSX tile slots. */
extern const VKGfx16Room *const vk_gfx16_scenes[3];
#endif
'''
    (out_dir / 'gfx16_md.c').write_text('\n'.join(c) + '\n')
    (out_dir / 'gfx16_md.h').write_text(h)


def preview(path, rooms):
    from PIL import Image
    path.mkdir(parents=True, exist_ok=True)
    for (stage, room), r in rooms.items():
        if stage == 'scene':
            stage = 0
        im = Image.new('RGB', (256, 176))
        px = im.load()
        for i, slot in enumerate(r['map']):
            tile = r['tiles'][slot]
            for y in range(8):
                for x in range(8):
                    px[(i % 32) * 8 + x, (i // 32) * 8 + y] = tuple(
                        v * 36 for v in r['palette'][(tile[y] >> (28 - 4 * x)) & 15])
        im.save(path / f'stage-{stage:02d}-room-{room:02d}.png')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('rom', nargs='?', type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / 'sgdk/src/generated')
    parser.add_argument('--preview', type=Path)
    args = parser.parse_args()
    data = load_rom(args.rom or reference_rom())
    src = load_sources()
    rooms = {}
    for block in BLOCKS:
        rooms.update(build_block(data, block, src))
    for key, im in courtyard_pictures(src).items():
        rooms[key] = encode_picture(im)
    index, total = room_index(data)
    write_c(args.out, rooms, index, total)
    if args.preview:
        preview(args.preview, rooms)
    for key, r in sorted(rooms.items(), key=lambda kv: str(kv[0])):
        print(f'stage {key[0]} room {key[1]}: {len(r["tiles"])} slots, {r["variants"]} variants')
    return 0


if __name__ == '__main__':
    sys.exit(main())
