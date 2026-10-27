#!/usr/bin/env python3
"""Generate SGDK 4bpp tiles for the stage 1 hall enemies (types A and B, 4 and 5).

Usage: VK_ROM_PATH=ROM.rom python3 -I tools/build_enemy_sprites_md.py
Writes sgdk/src/generated/enemy_sprites_md.c and .h (never hand edit).

Source: the twelve 16x16 layers of types A and B, decoded from the reference ROM
by tools/extract_enemy_sprites.py (research/enemy-assets.md). Pattern N (0x40..0x6C)
is stored at tile slot (N - 0x40) / 4 in the table, four 8x8 tiles per layer in
UL, LL, UR, LR order: the column-major order of an SGDK SPRITE_SIZE(2, 2).

Type 4 (hall room 4, a bat): six layer pairs 48/4C, 50/54, 58/5C (flying left) and
their mirrors 60/64, 68/6C, 70/74 (flying right), decoded from the RLE stream at
0x154DE (bytes 271..462), checked against the room 4 VRAM (scratchpad/type4/gfx,
research/enemy-behavior.md). They go in a second table, vk_enemy4_tiles, slot
(N - 0x48) / 4, because their pattern numbers overlap types A and B.

Type 5 (hall rooms 2, 3 and 7, the panther): the sixteen layers 40..7C of the RLE
stream at 0x15AB9 (417 bytes, decoded to 512), and their mirrors 80..BC (layer
N + 0x40 is layer N mirrored), checked against 38 VRAM captures (research/
enemy-behavior.md, section Type 5). Third table, vk_enemy5_tiles, slot (N - 0x40) / 4.

Pixels carry palette indices: the MSX2 colour of each layer (2 for N, 4 for N+4), read
in the sprite colour table (research/enemy-assets.md section 5). The second layer
of each pair has CC set: where both layers have a pixel, the V9938 shows the OR of
the two colours. The MD draws the first layer above the second, so that OR is
baked into the first layer. The game uses PAL2 (the room palette) for them.
Index 0 is transparent.
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
# MSX2 colour of each layer, read from the sprite colour table (VRAM F400 + 16 per sprite,
# same SAT and colour table in one VRAM dump) of the hall captures at images 2222 and 2844
# (research/enemy-assets.md, section 5): every first layer N has colour 02, every second
# layer N+4 colour 44 (colour 4, CC). All 16 lines carry the same byte. 48/4C were not
# on screen in those dumps and follow the same pairs. The colour index is the MSX palette
# index, which is the index of PAL2 (the room palette) in the game.
# pair_longs applies it to every pair (N, N+4) of types 1, 4 and 5.
# Type 4: same colour bytes 02 / 44 in the room 4 captures (scratchpad/type4/gfx).
TYPE4_STREAM = 0x154DE
TYPE4_OFFSET = 271
TYPE4_SHA256 = '2ede5a121ee46caab886b2e92d01b8726689445ee9c79efd53efa149faecfecc'
TYPE4_FIRST_PATTERN = 0x48
TYPE4_PATTERN_COUNT = 12
# Type 5: colour bytes 02 / 44 on the 16 lines, as types 1 and 4.
TYPE5_STREAM = 0x15AB9
TYPE5_SHA256 = 'c0845ef045b49981e35d7b640acc6ea6438668f075197a49573b34cbed39c4df'
TYPE5_FIRST_PATTERN = 0x40
TYPE5_PATTERN_COUNT = 32


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


def layers_of_type4(rom):
    """Return {pattern: 32-byte layer} for patterns 0x48..0x74 (type 4)."""
    stream = rle_decode(rom, TYPE4_STREAM)
    data = stream[TYPE4_OFFSET:TYPE4_OFFSET + 192]
    digest = hashlib.sha256(data).hexdigest()
    if digest != TYPE4_SHA256:
        raise ValueError(f'type 4 layers: unexpected bytes {digest}')
    layers = {}
    for index in range(6):
        layer = data[index * 32:index * 32 + 32]
        pattern = TYPE4_FIRST_PATTERN + 4 * index
        layers[pattern] = layer
        # Flying right: the left pair mirrored, 0x18 pattern numbers further (60/64, 68/6C, 70/74).
        layers[pattern + 0x18] = mirror_layer(layer)
    expected = [TYPE4_FIRST_PATTERN + 4 * k for k in range(TYPE4_PATTERN_COUNT)]
    assert sorted(layers) == expected, sorted(layers)
    return layers


def layers_of_type5(rom):
    """Return {pattern: 32-byte layer} for patterns 0x40..0xBC (type 5)."""
    data = bytes(rle_decode(rom, TYPE5_STREAM))
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != 512 or digest != TYPE5_SHA256:
        raise ValueError(f'type 5 layers: unexpected bytes {digest}')
    layers = {}
    for index in range(16):
        layer = data[index * 32:index * 32 + 32]
        layers[TYPE5_FIRST_PATTERN + 4 * index] = layer
        # Facing right: the same layer mirrored, 0x40 pattern numbers further.
        layers[TYPE5_FIRST_PATTERN + 0x40 + 4 * index] = mirror_layer(layer)
    return layers


def pair_longs(layers):
    """Tiles of every layer, the CC OR of each pair baked into its first layer."""
    longs = []
    for pattern in sorted(layers):
        # Pattern N is the first layer of a pair (N, N+4 with N & 4 == 0), N+4 the second (CC).
        if pattern & 4:
            words = tile_longs(layers[pattern], 4)
        else:
            words = tile_longs(layers[pattern], 2, layers[pattern + 4], 4)
        longs.append((pattern, words))
    return longs


def c_table(name, longs):
    lines = [f'const unsigned long {name}[{len(longs) * TILES_PER_LAYER * LONGS_PER_TILE}] = {{']
    for pattern, words in longs:
        lines.append(f'    /* pattern {pattern:02X} */')
        for i in range(0, len(words), LONGS_PER_TILE):
            chunk = ', '.join(f'0x{w:08X}UL' for w in words[i:i + LONGS_PER_TILE])
            lines.append(f'    {chunk},')
    lines.append('};')
    return lines


def tile_longs(layer, index_value, cc_layer=None, cc_value=0):
    """Eight 32-bit rows per 8x8 tile, four tiles per 16x16 layer (UL, LL, UR, LR).

    cc_layer: the CC layer drawn with this one; its colour is ORed where both are set."""
    words = []
    for tile in range(TILES_PER_LAYER):
        for row in range(8):
            byte_row = layer[tile * 8 + row]
            cc_row = cc_layer[tile * 8 + row] if cc_layer else 0
            word = 0
            for x in range(8):
                bit = 1 << (7 - x)
                pixel = 0
                if byte_row & bit:
                    pixel = index_value | (cc_value if cc_row & bit else 0)
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
    longs = pair_longs(layers_of_types_a_and_b(data))
    longs4 = pair_longs(layers_of_type4(data))
    longs5 = pair_longs(layers_of_type5(data))

    OUT.mkdir(parents=True, exist_ok=True)
    tile_count = PATTERN_COUNT * TILES_PER_LAYER
    tile_count4 = TYPE4_PATTERN_COUNT * TILES_PER_LAYER
    tile_count5 = TYPE5_PATTERN_COUNT * TILES_PER_LAYER
    lines = [
        '/* Generated from the Vampire Killer MSX2 ROM by tools/build_enemy_sprites_md.py.',
        ' * Types A and B of the stage 1 hall (patterns 0x40..0x6C), type 4 (room 4, 0x48..0x74)',
        ' * and type 5 (rooms 2, 3, 7, 0x40..0xBC).',
        ' * Colour index = MSX2 colour of each layer (2 for N, 4 for N+4): see research/enemy-assets.md.',
        ' * Never hand edit. */',
        '#include "enemy_sprites_md.h"',
    ]
    lines += c_table('vk_enemy_tiles', longs)
    lines += c_table('vk_enemy4_tiles', longs4)
    lines += c_table('vk_enemy5_tiles', longs5)
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
        f'#define VK_ENEMY4_FIRST_PATTERN 0x{TYPE4_FIRST_PATTERN:02X}',
        f'#define VK_ENEMY4_TILE_COUNT {tile_count4}',
        f'extern const unsigned long vk_enemy4_tiles[{tile_count4 * LONGS_PER_TILE}];',
        f'#define VK_ENEMY5_FIRST_PATTERN 0x{TYPE5_FIRST_PATTERN:02X}',
        f'#define VK_ENEMY5_TILE_COUNT {tile_count5}',
        f'extern const unsigned long vk_enemy5_tiles[{tile_count5 * LONGS_PER_TILE}];',
        '#endif',
    ]
    (OUT / 'enemy_sprites_md.h').write_text('\n'.join(header) + '\n')
    print(f'{PATTERN_COUNT} + {TYPE4_PATTERN_COUNT} + {TYPE5_PATTERN_COUNT} layers, '
          f'{tile_count} + {tile_count4} + {tile_count5} tiles -> {OUT}')


if __name__ == '__main__':
    generate(sys.argv[1] if len(sys.argv) == 2 else None)
