#!/usr/bin/env python3
"""16-bit graphics mode (research/gfx16.md): the MSX rooms redrawn with the 16-bit art of
Castlevania-MD (tiles redrawn by Fiuke Bnuy, given to the author of this port).

The MSX layout is kept cell for cell: every MSX tile id of a room gets a 16-bit drawing by a rule
(RULES): a texture of the source pictures (assets/gfx16/*.png) taken at the cell position modulo
the texture period, a picture cell laid over the wall, or a stencil (the MSX tile's own shape,
filled with 16-bit textures). A tile id without a rule keeps its MSX pixels.

Output per room (sgdk/src/generated/gfx16_md.c/.h):
- tiles: slot t (t < 192) is the drawing of MSX tile t at its first place in the block (so the
  code that draws MSX tile ids - hidden blocks - still finds them); the free slots hold the
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
BLOCKS = {1: (1, 2, 3)}          # block -> stages done so far


def load_sources():
    from PIL import Image
    return {name: Image.open(ROOT / 'assets/gfx16' / f'{name}.png').convert('RGB').load()
            for name in ('hall16', 'under16')}


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


RULES = {1: block1_rules}


def tex(src, texture, X, Y):
    name, sx, sy, px, py = texture
    return src[name][sx + X % px + (0 if px > 1 else X % 8), sy + Y % py + (0 if py > 1 else Y % 8)]


class Skin:
    def __init__(self, src, rules, graphics):
        self.src, self.rules, self.graphics = src, rules, graphics
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
        count = layout.byte(0x6000 + stage + 1) - layout.byte(0x6000 + stage)
        for room in range(count):
            manifest, graphics, tilemap = decode_room(data, stage, room)
            if manifest['unresolved_tile_ids']:
                continue
            rooms[(stage, room)] = {'grid': tilemap[64:], 'palette': manifest['palette_rgb333']}
    skin = Skin(src, RULES[block](rooms), graphics)
    skin.wall_idx = skin.colours(0x0E) | skin.colours(0x11)
    skin.brick_idx = skin.colours(0x41)
    first = {}                           # tile id -> its first cell in the block
    for key in sorted(rooms):
        for i, t in enumerate(rooms[key]['grid']):
            first.setdefault(t, (i % 32, i // 32))
    out = {}
    for key in sorted(rooms):
        grid, palette = rooms[key]['grid'], rooms[key]['palette']
        msx_rgb = [tuple(v * 36 for v in c) for c in palette]
        paper = 0x0E in grid
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
        slots = {t: encode(canon[t]) for t in range(192) if t in used or skin.rules.get(t)}
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
    c = ['/* Generated by tools/build_gfx16.py: the rooms in the 16-bit art (research/gfx16.md). Never hand edit. */',
         '#include "gfx16_md.h"', '']
    for (stage, room), r in sorted(rooms.items()):
        name = f'g16_{stage}_{room}'
        c.append(f'static const u32 {name}_tiles[{len(r["tiles"]) * 8}] = {{')
        c += ['    ' + ','.join(f'0x{v:08X}' for v in t) + ',' for t in r['tiles']]
        c.append('};')
        c.append(f'static const u16 {name}_map[704] = {{')
        c += ['    ' + ','.join(str(v) for v in r['map'][i:i + 32]) + ',' for i in range(0, 704, 32)]
        c.append('};')
        words = ','.join(f'0x{(b << 9) | (g << 5) | (rr << 1):04X}' for rr, g, b in r['palette'])
        c.append(f'static const VKGfx16Room {name} = {{{name}_tiles, {name}_map, {len(r["tiles"])}, '
                 f'{{{words}}}, {{{",".join(str(v) for v in r["msx"])}}}}};')
    c.append(f'const VKGfx16Room *const vk_gfx16_rooms[VK_GFX16_ROOMS] = {{')
    by_index = {index[k]: k for k in rooms}
    for n in range(total):
        c.append(f'    &g16_{by_index[n][0]}_{by_index[n][1]},' if n in by_index else '    0,')
    c.append('};')
    h = f'''/* Generated by tools/build_gfx16.py. One entry per room of vk_maps (0: no 16-bit art yet).
 * tiles: `count` tiles loaded at TILE_USER_INDEX in place of the MSX tile set (slot t < 192 is
 * the 16-bit drawing of MSX tile t, the others are variants); map: the 32 x 22 cells (slots);
 * palette: PAL0; msx[c]: the palette entry closest to MSX colour c of the room palette. */
#ifndef VK_GFX16_MD_H
#define VK_GFX16_MD_H
#include <genesis.h>
#define VK_GFX16_ROOMS {total}
#define VK_GFX16_SLOTS {SLOTS}
typedef struct {{ const u32 *tiles; const u16 *map; u16 count; u16 palette[16]; u8 msx[16]; }} VKGfx16Room;
extern const VKGfx16Room *const vk_gfx16_rooms[VK_GFX16_ROOMS];
#endif
'''
    (out_dir / 'gfx16_md.c').write_text('\n'.join(c) + '\n')
    (out_dir / 'gfx16_md.h').write_text(h)


def preview(path, rooms):
    from PIL import Image
    path.mkdir(parents=True, exist_ok=True)
    for (stage, room), r in rooms.items():
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
    data = load_rom(args.rom) if args.rom else reference_rom()
    src = load_sources()
    rooms = {}
    for block in BLOCKS:
        rooms.update(build_block(data, block, src))
    index, total = room_index(data)
    write_c(args.out, rooms, index, total)
    if args.preview:
        preview(args.preview, rooms)
    for key, r in sorted(rooms.items()):
        print(f'stage {key[0]} room {key[1]}: {len(r["tiles"])} slots, {r["variants"]} variants')
    return 0


if __name__ == '__main__':
    sys.exit(main())
