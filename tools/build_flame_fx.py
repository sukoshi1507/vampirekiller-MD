#!/usr/bin/env python3
"""Build the courtyard flame sprites for SGDK from the reference MSX2 ROM.

Replaces the former V15 prototype pixels (research/flame_v15_source.txt).

Proven facts (see research/courtyard-flames.md):
- Screen mode SCREEN 5 (CHGMOD A=5), 256x212, 4 bpp, 128 bytes per VRAM line.
- The flame states are 16x16 blocks at VRAM (48,368) = state A and (32,368) = state B,
  on the hidden page. The game copies them to the visible page with LMMM
  (R#46=0x98, transparent: colour 0 is not copied).
- The same 128-byte blocks exist in the ROM as packed 16x16 4 bpp, row-major,
  left pixel in the high nibble: file offset 0xDF48 = state A, 0xDEC8 = state B.
- Flame A (x=64) shows A for 16 ticks then B for 16 ticks; flame B (x=192) runs 4 ticks earlier.

The ROM bytes are decoded here. They are checked pixel by pixel against the VRAM produced
by running the ROM courtyard routines in IntroHarness (same path as extract_intro.py).
The palette is also taken from that run (game writes on port 9A).

Output: sgdk/src/generated/flame_fx_md.h (names unchanged for sgdk/src/main.c),
sgdk/src/generated/flame_fx_manifest.json, and indexed PNGs in assets/flame_fx/.
"""
from pathlib import Path
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from probe import load_rom, reference_rom  # noqa: E402  (verifies the SHA-256)
from intro_harness import IntroHarness  # noqa: E402

ROOM = 0  # courtyard, stage 0 room 0 (same call path as extract_intro.py)
BANK_SIZE = 0x2000
STATES = {
    'A': {'rom_offset': 0xDF48, 'vram': (48, 368)},
    'B': {'rom_offset': 0xDEC8, 'vram': (32, 368)},
}
# main.c selects (flame_clock >> 3) & (VK_FLAME_FRAMES - 1): 8 ticks per frame.
# A, A, B, B gives 16 ticks per state, period 32, as in the ROM.
FLAME_FRAME_STATES = ['A', 'A', 'B', 'B']
# main.c burst (courtyard hit, 12 ticks): no ROM spark exists in the courtyard.
# The three burst frames reuse the ROM flame states; this is an integration choice.
SPARK_FRAME_STATES = ['A', 'B', 'A']
OUT_HEADER = ROOT / 'sgdk/src/generated/flame_fx_md.h'
OUT_MANIFEST = ROOT / 'sgdk/src/generated/flame_fx_manifest.json'
OUT_PNG = ROOT / 'assets/flame_fx'


def decode_packed(data, offset):
    """16x16 4 bpp, 8 bytes per row, left pixel in the high nibble."""
    image = [[0] * 16 for _ in range(16)]
    for row in range(16):
        for col in range(0, 16, 2):
            byte = data[offset + row * 8 + col // 2]
            image[row][col] = byte >> 4
            image[row][col + 1] = byte & 15
    return image


def courtyard_harness(rom):
    harness = IntroHarness(rom)
    harness.call(0x4B59)
    harness.call(0x4D47)
    harness.call(0x44C6)
    harness.cpu.memory[0xC41A] = 1
    harness.call(0x63D3)
    harness.cpu.memory[0xC41A] = 0
    harness.call(0x62D0)
    harness.cpu.memory[0xD001] = ROOM
    harness.call(0x422C)
    if harness.cpu.memory[0xD000] != 0 or harness.cpu.memory[0xD001] != ROOM:
        raise ValueError('Unexpected starting stage for the courtyard')
    return harness


def vram_block(harness, x0, y0):
    return [[harness.pixel(x0 + x, y0 + y) for x in range(16)] for y in range(16)]


def tile_words(image, tx, ty):
    words = []
    for row in range(8):
        word = 0
        for col in range(8):
            word = (word << 4) | (image[ty * 8 + row][tx * 8 + col] & 15)
        words.append(word)
    return words


def sprite_tiles(image):
    # SGDK SPRITE_SIZE(2,2): the VDP numbers multi-tile sprites column by column
    # (top then bottom, then the next column). Order: TL, BL, TR, BR.
    order = [(0, 0), (0, 1), (1, 0), (1, 1)]
    words = []
    for tx, ty in order:
        words += tile_words(image, tx, ty)
    return words


def validate(rom, harness, images):
    for name, state in STATES.items():
        image = images[name]
        flat = [v for row in image for v in row]
        if len(flat) != 256 or max(flat) > 15:
            raise ValueError(f'State {name}: bad dimensions or index')
        if 0 not in flat or not any(flat):
            raise ValueError(f'State {name}: index 0 must be present and sprite must not be empty')
        x0, y0 = state['vram']
        if vram_block(harness, x0, y0) != image:
            raise ValueError(f'State {name}: ROM bytes differ from VRAM of the ROM run')
    if images['A'] == images['B']:
        raise ValueError('States A and B are identical')


def write_png(image, palette_rgb, path):
    from PIL import Image
    path.parent.mkdir(parents=True, exist_ok=True)
    picture = Image.new('P', (16, 16))
    picture.putdata([v for row in image for v in row])
    flat_palette = []
    for r, g, b in palette_rgb:
        flat_palette += [round(r * 255 / 7), round(g * 255 / 7), round(b * 255 / 7)]
    picture.putpalette(flat_palette + [0] * (768 - len(flat_palette)))
    picture.save(path)


def main():
    rom = load_rom(reference_rom())
    images = {name: decode_packed(rom, state['rom_offset']) for name, state in STATES.items()}
    harness = courtyard_harness(rom)
    validate(rom, harness, images)
    palette = list(harness.palette)
    used = sorted({v for name in images for row in images[name] for v in row})
    for index in used:
        if index >= len(palette):
            raise ValueError('Index without palette entry')

    frames = FLAME_FRAME_STATES + SPARK_FRAME_STATES
    flat = []
    for state in frames:
        flat += sprite_tiles(images[state])
    count = len(frames) * 4 * 8
    assert len(flat) == count == 224

    header = [
        '/* Generated by tools/build_flame_fx.py from the reference ROM (courtyard, SCREEN 5). */',
        '/* Provenance: sgdk/src/generated/flame_fx_manifest.json. Not the V15 prototype. */',
        '#ifndef VK_FLAME_FX_MD_H', '#define VK_FLAME_FX_MD_H',
        f'#define VK_FLAME_FRAMES {len(FLAME_FRAME_STATES)}',
        f'#define VK_SPARK_FRAMES {len(SPARK_FRAME_STATES)}',
        '#define VK_FLAME_TILES_PER_FRAME 4',
        f'static const u32 vk_flame_fx_tiles[{count}] = {{']
    for i in range(0, len(flat), 8):
        header.append('    ' + ', '.join(f'0x{w:08X}UL' for w in flat[i:i + 8])
                      + (',' if i + 8 < len(flat) else ''))
    header += ['};', '#endif']
    OUT_HEADER.write_text('\n'.join(header) + '\n')

    for name, state in STATES.items():
        write_png(images[name], palette, OUT_PNG / f'flame_state_{name}.png')

    def state_entry(name):
        state = STATES[name]
        offset = state['rom_offset']
        return {'state': name, 'file_offset': f'0x{offset:05X}',
                'bank': f'{offset // BANK_SIZE:02X}', 'offset_in_bank': f'0x{offset % BANK_SIZE:04X}',
                'cpu_address': None, 'cpu_address_note': 'depends on the bank window; not determined',
                'vram_x': state['vram'][0], 'vram_y': state['vram'][1],
                'bytes': 128, 'format': '16x16 4bpp row-major, left pixel high nibble',
                'pixel_sha256': hashlib.sha256(bytes(v for row in images[name] for v in row)).hexdigest(),
                'status': 'decoded',
                'status_note': 'ROM bytes equal the VRAM of the ROM run (IntroHarness) and the openMSX VRAM dump '
                               'at ticks 2300-2379 (page 0 block at x 64/192 matches on all stable frames).'}

    manifest = {
        'source_rom_sha256': hashlib.sha256(rom).hexdigest(),
        'screen_mode': 'SCREEN 5 (256x212, 4 bpp)',
        'transparency': 'index 0 transparent: LMMM with R#46=0x98 (TIMP) does not copy colour 0',
        'states': [state_entry('A'), state_entry('B')],
        'flame_frames': [{'index': i, 'state': s} for i, s in enumerate(FLAME_FRAME_STATES)],
        'spark_frames': [{'index': i, 'state': s, 'note': 'burst reuses ROM flame states; no ROM spark found'}
                         for i, s in enumerate(SPARK_FRAME_STATES)],
        'palette': {'room': ROOM, 'source': 'IntroHarness run of the ROM courtyard routines (port 9A writes)',
                    'status': 'candidate',
                    'status_note': 'not compared with the live VDP palette in openMSX',
                    'rgb333': [list(c) for c in palette],
                    'cram_md': [f'0x{((b << 9) | (g << 5) | (r << 1)):04X}' for r, g, b in palette],
                    'used_indices': used, 'index0': 'transparent for sprites'},
        'mega_drive_tiles': {'sprite_order': 'column-major (SPRITE_SIZE(2,2))', 'count': count // 8,
                             'status_note': 'column-major order to verify in an MD emulator'},
        'png': ['assets/flame_fx/flame_state_A.png', 'assets/flame_fx/flame_state_B.png'],
    }
    OUT_MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n')
    print('generated', OUT_HEADER, 'frames', len(FLAME_FRAME_STATES), 'sparks', len(SPARK_FRAME_STATES),
          'palette used', used)


if __name__ == '__main__':
    main()
