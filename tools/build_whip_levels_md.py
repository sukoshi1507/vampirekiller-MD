#!/usr/bin/env python3
"""Generate SGDK whip frames for MSX2 whip levels 1 and 2 (triplets 9-11 and 12-14).

Each frame is one standing attack composition: a lower component (C42E) and an upper
component (C42F). Right-facing frames use the lower index 0 and the upper triplet of the
level; left-facing frames use the lower index 10 and the mirrored triplet (+15 on the
upper index, the ROM index offset documented in research/whip-variants.md). Both sets are
decoded from the ROM position and mask tables (decode_extended_simon.components); nothing
is mirrored in pixels.

Output per level (same layout as the level 0 frames of build_simon_attack_md.py):
  sgdk/src/generated/whip_level{N}_md.c / .h   6 frames x 32 tiles (4 bpp, SGDK order),
                                               frames 0..2 right, 3..5 left;
                                               16-entry palette (MD 0000BBB0GGG0RRR0 words).
  sgdk/src/generated/whip_levels_manifest.json offsets, CPU addresses, banks, palettes, status.
  assets/whip_levels/*.png                     regenerable indexed previews (P mode, index 0
                                               = transparent, declared in the PNG).

Colour model: layer parity selects MSX colour plane 1 (even layers) or 2 (odd layers, the
colour-combination plane); overlaps give 3. The colour table written by the game at VRAM
0xF400 for level 0 is 0x01 for the first sprite and 0x42 (colour 2, CC set) for the second,
which agrees with this model for that level only. Levels 1 and 2 colours are not read back.
Index 0 is transparent in the VDP; the room palette entry 0 is not used for drawing.

Status: all frames are 'candidate' (ROM-derived). Mask layers without a ROM position are
listed in 'unplaced_layers' and not emitted (never invented). The VRAM evidence recorded in
the manifest ('vram_evidence') comes from openMSX traces (research/whip-variants.md), read
with the 2-tick lag observed between C42F and the sprite table.

Usage: build_whip_levels_md.py [ROM]   (default: reference ZIP, checked by SHA-256)
"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from decode_extended_simon import components  # noqa: E402
from extract_rooms import decode_room  # noqa: E402
from probe import load_rom, reference_rom  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'sgdk/src/generated'
PREVIEW = ROOT / 'assets/whip_levels'
W, H = 64, 32
ORIGIN_Y = -30
# Canvas origin in MSX pixels relative to Simon (C427/C425), as the level 0 right-facing
# buffer (x-24, y-30) and the engine's left-facing origin (x-39, see main.c / crouch manifest).
ORIGIN_X = {'right': -24, 'left': -39}
LEVELS = {
    1: {'right': [(0, 9), (0, 10), (0, 11)], 'left': [(10, 24), (10, 25), (10, 26)]},
    2: {'right': [(0, 12), (0, 13), (0, 14)], 'left': [(10, 27), (10, 28), (10, 29)]},
}
# What the openMSX VRAM traces (scratchpad, not committed) showed. Sprite pattern bytes were
# compared with the ROM layer bytes (identical 32-byte 16x16 layers, plain orientation),
# and positions with the ROM coordinate table. The sprite table lags C42F by 2 ticks.
VRAM_EVIDENCE = [
    {'level': 1, 'frame': 'right 11', 'ticks': '2281-2286', 'result': 'upper 0..5 and lower 0..1 matched at the ROM positions'},
    {'level': 1, 'frame': 'left 25', 'ticks': '2651-2652', 'result': 'upper 0..3 and lower 0..1 matched at the ROM positions'},
    {'level': 1, 'frame': 'left 26', 'ticks': '2653-2659', 'result': 'upper 0..1 and lower 0..1 matched; upper 2..5 not in the sprite table. Hypothesis: '
                                     'the game omits sprites with negative screen x (C427=14, dx -39 and -23); not confirmed'},
    {'level': 2, 'frame': 'right 12', 'ticks': '2277-2278', 'result': 'upper 0..1 and lower 0..1 matched; upper 2..5 never drawn, so the 4 extra mask layers are unused'},
    {'level': 2, 'frame': 'right 13', 'ticks': '2279-2280', 'result': 'upper 0..1 and lower 0..1 matched at the ROM positions'},
    {'level': 2, 'frame': 'right 14', 'ticks': '2281-2282', 'result': 'upper 0..3 and lower 0..1 matched at the ROM positions'},
    {'level': 2, 'frame': 'left 27', 'ticks': 'not checked', 'result': 'not checked yet'},
]
# Level 0 right frames already in the tree: used as a pixel reference for this code path.
LEVEL0_REFERENCE = (0, 6), (0, 7), (0, 8)
TILES_PER_FRAME = 32


def compose(rom, low, up, facing):
    """Return (canvas 64x32 of colour indices, per-component records, unplaced layers)."""
    ox, oy = ORIGIN_X[facing], ORIGIN_Y
    canvas = [0] * (W * H)
    records, unplaced = {}, []
    for part, index in (('lower', low), ('upper', up)):
        comp = components(rom, index, part)
        records[part] = {
            'index': index,
            'pattern_cpu': comp['pattern_cpu'],
            'pattern_file_offset': comp['pattern_file_offset'],
            'coordinate_cpu': comp['coordinate_cpu'],
            'coordinate_file_offset': comp['coordinate_file_offset'],
            'layer_count': comp['layer_count'],
            'position_count': comp['position_count'],
            'offsets_dy_dx': [list(p) for p in comp['offsets_dy_dx']],
        }
        placed = []
        for k, mask in enumerate(comp['layers']):
            if k >= comp['position_count']:
                unplaced.append({'part': part, 'layer': k, 'pattern_cpu': comp['pattern_cpu'],
                                 'pattern_file_offset': comp['pattern_file_offset']})
                continue
            dy, dx = comp['offsets_dy_dx'][k]
            bit = 1 if k % 2 == 0 else 2
            placed.append(k)
            for y in range(16):
                for x in range(16):
                    if not mask[y * 16 + x]:
                        continue
                    cx, cy = dx - ox + x, dy - oy + y
                    if not (0 <= cx < W and 0 <= cy < H):
                        raise ValueError(f'{facing} {low}/{up}: layer {k} outside the 64x32 canvas')
                    canvas[cy * W + cx] |= bit
        records[part]['placed_layers'] = placed
    return canvas, records, unplaced


def tile_words(canvas):
    """32 words per frame: two 32x32 halves, 16 tiles each in column-major order (SGDK)."""
    words = []
    for half in range(2):
        for tx in range(4):
            for ty in range(4):
                for py in range(8):
                    word = 0
                    for px in range(8):
                        word = (word << 4) | canvas[(ty * 8 + py) * W + half * 32 + tx * 8 + px]
                    words.append(word)
    assert len(words) == TILES_PER_FRAME * 8
    return words


def md_palette(rom):
    """16 entries from the stage 1 room 0 palette (same source as build_simon_attack_md.py)."""
    rgb = decode_room(rom, 1, 0)[0]['palette_rgb333']
    words = []
    for r, g, b in rgb:
        if not all(0 <= v <= 7 for v in (r, g, b)):
            raise ValueError('RGB333 component out of range')
        words.append((b << 9) | (g << 5) | (r << 1))
    return rgb, words


def check_level0_reference(rom, lookup_path):
    """Same code path must reproduce the committed level 0 right frames exactly."""
    text = lookup_path.read_text()
    committed = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{8})UL', text)]
    got = []
    for low, up in LEVEL0_REFERENCE:
        canvas, _, unplaced = compose(rom, low, up, 'right')
        if unplaced:
            raise ValueError('Level 0 reference frame has unplaced layers')
        got.extend(tile_words(canvas))
    if got != committed[:len(got)]:
        raise ValueError('Level 0 reference mismatch: code path does not match simon_attack_md.c')
    return len(got)


def write_preview(canvas, palette_rgb, path):
    img = Image.new('P', (W, H))
    flat = []
    for r, g, b in palette_rgb:
        flat += [round(r * 255 / 7), round(g * 255 / 7), round(b * 255 / 7)]
    img.putpalette(flat + [0] * (768 - len(flat)))
    img.putdata(canvas)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, transparency=0)


def generate(rom_path=None, out=OUT, preview=PREVIEW):
    rom_path = rom_path or reference_rom()
    rom = load_rom(rom_path)
    rgb, palette_words = md_palette(rom)
    checked = check_level0_reference(rom, OUT / 'simon_attack_md.c')
    manifest = {
        'rom_sha256': hashlib.sha256(rom).hexdigest(),
        'asset_source': 'MSX2 original, position/mask tables',
        'palette_source': 'stage 1 room 0 palette (decode_room(rom,1,0)), all 16 entries, RGB333 -> MD 0000BBB0GGG0RRR0',
        'palette_rgb333': [list(c) for c in rgb],
        'palette_md_words': ['0x%04X' % w for w in palette_words],
        'index0': 'transparent (VDP); palette entry 0 not used for drawing',
        'colour_model': 'layer parity: even -> plane 1, odd -> plane 2 (CC), overlap -> 3',
        'level0_reference_words_checked': checked,
        'canvas': {'width': W, 'height': H, 'origin_y': ORIGIN_Y, 'origin_x': ORIGIN_X,
                   'halves': 'two 32x32 sprites per frame at canvas x 0 and 32'},
        'frame_order': 'frames 0..2 right, 3..5 left (triplet order = C42F sequence)',
        'levels': {},
        'phase_source': 'research/whip-variants.md (C42F sequence, right: 2,2,8 ticks; level 1 left 25 x2, 26 x8 seen)',
        'vram_evidence': VRAM_EVIDENCE,
    }
    out.mkdir(parents=True, exist_ok=True)
    for level, facings in LEVELS.items():
        tiles, frames = [], []
        n = 0
        for facing in ('right', 'left'):
            for low, up in facings[facing]:
                canvas, records, unplaced = compose(rom, low, up, facing)
                words = tile_words(canvas)
                tiles.extend(words)
                counts = {str(c): canvas.count(c) for c in sorted(set(canvas))}
                if max(canvas) > 3 or len(counts) > 16:
                    raise ValueError('colour index out of range')
                png = f'whip_level{level}_{facing}_{low}-{up}.png'
                write_preview(canvas, rgb, preview / png)
                frames.append({
                    'frame': n, 'facing': facing, 'lower_index': low, 'upper_index': up,
                    'origin_world_xy': [ORIGIN_X[facing], ORIGIN_Y],
                    'png': f'{preview.relative_to(ROOT)}/{png}',
                    'tiles': TILES_PER_FRAME,
                    'colour_counts': counts,
                    'components': records,
                    'unplaced_layers': unplaced,
                    'unplaced_layers_note': ('mask layers beyond the coordinate count: no position in the ROM, '
                                             'not emitted. Not drawn by the game in the VRAM check, see vram_evidence'
                                             if unplaced else ''),
                    'status': 'candidate',
                    'classification': 'ROM-derived candidate; full on-screen match not verified for every frame',
                })
                n += 1
        name = f'whip_level{level}_md'
        count = len(tiles)
        (out / f'{name}.h').write_text(
            '/* Generated ROM-derived whip frames, level %d. Never hand edit. */\n'
            '#ifndef VK_WHIP_LEVEL%d_MD_H\n'
            '#define VK_WHIP_LEVEL%d_MD_H\n'
            '#define VK_WHIP_LEVEL%d_FRAME_COUNT %d\n'
            '#define VK_WHIP_LEVEL%d_TILES_PER_FRAME %d\n'
            'extern const unsigned long vk_whip_level%d_tiles[%d];\n'
            'extern const unsigned short vk_whip_level%d_palette[16];\n'
            '#endif\n' % (level, level, level, level, len(frames), level, TILES_PER_FRAME,
                          level, count, level))
        lines = ['/* Generated from Vampire Killer MSX2 ROM. Frames 0..2 right, 3..5 left. */',
                 '#include "%s.h"' % name,
                 f'const unsigned long vk_whip_level{level}_tiles[{count}] = {{']
        for i in range(0, count, 8):
            lines.append('    ' + ', '.join('0x%08XUL' % v for v in tiles[i:i + 8]) + ',')
        lines.append('};')
        lines.append(f'const unsigned short vk_whip_level{level}_palette[16] = {{')
        lines.append('    ' + ', '.join('0x%04X' % w for w in palette_words) + ',')
        lines.append('};')
        (out / f'{name}.c').write_text('\n'.join(lines) + '\n')
        manifest['levels'][str(level)] = {'symbol_tiles': f'vk_whip_level{level}_tiles',
                                          'symbol_palette': f'vk_whip_level{level}_palette',
                                          'tile_words': count, 'frames': frames}
    (out / 'whip_levels_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('rom', nargs='?', type=Path, default=None)
    args = ap.parse_args()
    m = generate(args.rom)
    print('Generated', sum(len(v['frames']) for v in m['levels'].values()), 'frames for levels',
          ', '.join(m['levels']))
