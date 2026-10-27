#!/usr/bin/env python3
"""Generate SGDK 4bpp tiles for the stage 1 hall enemies (types A and B).

Usage: VK_ROM_PATH=ROM.rom python3 -I tools/build_enemy_sprites_md.py
Writes sgdk/src/generated/enemy_sprites_md.c and .h (never hand edit).

Source: the twelve 16x16 layers of types A and B, decoded from the reference ROM
by tools/extract_enemy_sprites.py (research/enemy-assets.md). Pattern N (0x40..0x6C)
is stored at tile slot (N - 0x40) / 4 in the table, four 8x8 tiles per layer in
UL, LL, UR, LR order: the column-major order of an SGDK SPRITE_SIZE(2, 2).

Colour is NOT resolved (research/enemy-assets.md section 5). Pixels carry
provisional palette indices: 1 for the layer N, 2 for the layer N+4 of each pair.
The game uses PAL2 (the room palette) for them. Index 0 is transparent.
"""
import hashlib
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from extract_enemy_sprites import PATTERN_NUMBERS, STREAMS, mirror_layer, rle_decode  # noqa: E402
from probe import load_rom, reference_rom  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'sgdk/src/generated'
ROM_SHA256 = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'
FIRST_PATTERN = 0x40
PATTERN_COUNT = 12
TILES_PER_LAYER = 4
LONGS_PER_TILE = 8
PROVISIONAL_INDEX_N = 1      # layer N
PROVISIONAL_INDEX_N4 = 2     # layer N+4


def layers_of_types_a_and_b(rom):
    """Return {pattern: 32-byte layer} for patterns 0x40..0x6C (types A and B)."""
    stream = rle_decode(rom, STREAMS['A']['file_offset'])
    layers = {}
    for index, (pn_a, pn_b) in enumerate(PATTERN_NUMBERS['A']):
        start = STREAMS['A']['pairs'][index]
        layer_a = stream[start:start + 32]
        layer_b = stream[start + 32:start + 64]
        layers[pn_a] = layer_a
        layers[pn_b] = layer_b
        # Type B: horizontal mirror of the type A layer, with the B pattern numbers.
        b_a, b_b = PATTERN_NUMBERS['B'][index]
        layers[b_a] = mirror_layer(layer_a)
        layers[b_b] = mirror_layer(layer_b)
    expected = [FIRST_PATTERN + 4 * k for k in range(PATTERN_COUNT)]
    assert sorted(layers) == expected, sorted(layers)
    return layers


def tile_longs(layer, index_value):
    """Eight 32-bit rows per 8x8 tile, four tiles per 16x16 layer (UL, LL, UR, LR)."""
    words = []
    for tile in range(TILES_PER_LAYER):
        for row in range(8):
            byte_row = layer[tile * 8 + row]
            word = 0
            for x in range(8):
                pixel = index_value if (byte_row >> (7 - x)) & 1 else 0
                word = (word << 4) | pixel
            words.append(word)
    assert len(words) == TILES_PER_LAYER * LONGS_PER_TILE
    return words


def generate(rom_path=None):
    rom_path = Path(rom_path) if rom_path else reference_rom()
    data = load_rom(rom_path)
    digest = hashlib.sha256(data).hexdigest()
    if digest != ROM_SHA256:
        raise ValueError(f'ROM inattendue: {digest}')
    layers = layers_of_types_a_and_b(data)

    longs = []
    for pattern in sorted(layers):
        # Pattern N is the first layer of a pair (N, N+4 with N & 4 == 0), N+4 the second.
        value = PROVISIONAL_INDEX_N if (pattern & 4) == 0 else PROVISIONAL_INDEX_N4
        longs.append((pattern, tile_longs(layers[pattern], value)))

    OUT.mkdir(parents=True, exist_ok=True)
    tile_count = PATTERN_COUNT * TILES_PER_LAYER
    lines = [
        '/* Generated from the Vampire Killer MSX2 ROM by tools/build_enemy_sprites_md.py.',
        ' * Types A and B of the stage 1 hall (patterns 0x40..0x6C). Provisional colour',
        ' * indices 1 (layer N) and 2 (layer N+4): see research/enemy-assets.md. Never hand edit. */',
        '#include "enemy_sprites_md.h"',
        f'const unsigned long vk_enemy_tiles[{tile_count * LONGS_PER_TILE}] = {{',
    ]
    flat = []
    for pattern, words in longs:
        flat.append(f'    /* pattern {pattern:02X} */')
        for i in range(0, len(words), LONGS_PER_TILE):
            chunk = ', '.join(f'0x{w:08X}UL' for w in words[i:i + LONGS_PER_TILE])
            flat.append(f'    {chunk},')
    lines.extend(flat)
    lines.append('};')
    (OUT / 'enemy_sprites_md.c').write_text('\n'.join(lines) + '\n')

    header = [
        '/* Generated enemy sprite tiles, stage 1 hall. Never hand edit (tools/build_enemy_sprites_md.py). */',
        '#ifndef VK_ENEMY_SPRITES_MD_H',
        '#define VK_ENEMY_SPRITES_MD_H',
        f'#define VK_ENEMY_FIRST_PATTERN 0x{FIRST_PATTERN:02X}',
        f'#define VK_ENEMY_PATTERN_COUNT {PATTERN_COUNT}',
        f'#define VK_ENEMY_TILES_PER_LAYER {TILES_PER_LAYER}',
        f'#define VK_ENEMY_TILE_COUNT {tile_count}',
        f'extern const unsigned long vk_enemy_tiles[{tile_count * LONGS_PER_TILE}];',
        '#endif',
    ]
    (OUT / 'enemy_sprites_md.h').write_text('\n'.join(header) + '\n')
    print(f'{PATTERN_COUNT} layers, {tile_count} tiles -> {OUT}')


if __name__ == '__main__':
    generate(sys.argv[1] if len(sys.argv) == 2 else None)
