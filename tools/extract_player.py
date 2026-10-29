#!/usr/bin/env python3
"""Extract Simon's walking/jumping masks, positions and motion tables.
RLE: original 46EB/46F1; pattern tables A281/A2D1, position tables 7985/79D5.
"""
import argparse
import json
from pathlib import Path
import struct
from PIL import Image, ImageDraw
from probe import load_rom, ROOT
from extract_rooms import RomView, decode_room

POSES = [(0, 0), (1, 1), (2, 2), (10, 15), (11, 16), (12, 17), (6, 0), (16, 15),
         # Recoil (C420 = 5): 6FCA sets C42E/C42F = 07/03 (+10/+15 facing left, 765F),
         # landing (700C) adds 1 to both.
         (7, 3), (17, 18), (8, 4), (18, 19)]
JUMP_POSES = (6, 7)
# Rows of the 24x32 canvas above the feet: 30 (drawn at y-31), or 31 for the
# recoil poses whose upper part reaches one pixel higher (drawn at y-32).
POSE_TOP = {8: 31, 9: 31, 10: 31, 11: 31}


def decode_rle(view, address):
    output = bytearray()
    for _ in range(4096):
        tag = view.byte(address)
        address += 1
        if tag == 0:
            return bytes(output)
        if tag == 128:
            address = view.word(address)
            continue
        count = tag & 127
        if tag > 128:
            output.extend(view.read(address, count))
            address += count
        else:
            output.extend(bytes((view.byte(address),)) * count)
            address += 1
        if len(output) > 8192:
            raise ValueError('RLE output exceeds expected resource budget')
    raise ValueError('RLE stream did not terminate')


def extract_pose(data, lower, upper, jumping=False, top=30):
    patterns = RomView(data, (0, 11, 12, 13))
    positions = RomView(data, (0, 1, 2, 3))
    pixels = [0] * (24 * 32)
    provenance = []
    for part, frame, table, coords in [(0, lower, 0xA281, 0x7985),
                                       (1, upper, 0xA2D1, 0x79D5)]:
        source = patterns.word(table + frame * 2)
        raw = decode_rle(patterns, source)
        coordinate = positions.word(coords + frame * 2)
        if len(raw) != 64 or positions.byte(coordinate) != 2:
            raise ValueError('Pose requires a different sprite layout')
        provenance.append({'frame': frame, 'pattern_cpu': f'{source:04X}',
                           'pattern_file_offset': f'{patterns.offset(source):05X}',
                           'coordinate_cpu': f'{coordinate:04X}'})
        for layer in range(2):
            dy, dx = struct.unpack('bb', positions.read(coordinate + 1 + layer * 2, 2))
            if jumping and part == 0 and lower in (6, 16):
                dy -= 6  # Original 78B2..78C0, C420 == 1.
            for y in range(16):
                for x in range(16):
                    if not raw[layer * 32 + y + (16 if x >= 8 else 0)] & (128 >> (x & 7)):
                        continue
                    xx, yy = x + dx + 8, y + dy + top
                    if not (0 <= xx < 24 and 0 <= yy < 32):
                        raise ValueError('Nontransparent sprite pixel would be clipped')
                    # Original 790C colors: 1 then 0x42 (CC). Overlap gives color 3.
                    pixels[yy * 24 + xx] |= layer + 1
    return pixels, provenance


# Death (C420 = 6): 7073 sets C42E/C42F = 09/05 (+10/+15 facing left, 765F). Simon
# lies on the floor: both parts side by side, 32 x 16 px above the feet.
DEATH_POSES = [(9, 5), (19, 20)]
# Stairs (6E14, 6E4B, 6E84, 6EBC with 6F09 / 6F24): C42E/C42F = 03/01 facing right,
# 0D/10 facing left, plus one of these steps by (C42B >> 2) & 1: up (4,1) then (5,2),
# down (4,1) then (3,2). A hit on the stairs sets C42F := 2 (6FB4): (4,1) -> (4,2).
# Order per facing: (4,1), (5,2), (3,2), (4,2); facing left adds 10/15 (765F).
STAIR_POSES = [(4, 1), (5, 2), (3, 2), (4, 2), (14, 16), (15, 17), (13, 17), (14, 17)]


def extract_wide_pose(data, lower, upper):
    """32x16 canvas of a pose whose parts lie side by side; returns the pixels and
    the X offset of the canvas from Simon's X (the top is Y - 15)."""
    patterns = RomView(data, (0, 11, 12, 13))
    positions = RomView(data, (0, 1, 2, 3))
    parts = []
    for frame, table, coords in [(lower, 0xA281, 0x7985), (upper, 0xA2D1, 0x79D5)]:
        raw = decode_rle(patterns, patterns.word(table + frame * 2))
        coordinate = positions.word(coords + frame * 2)
        if len(raw) != 64 or positions.byte(coordinate) != 2:
            raise ValueError('Pose requires a different sprite layout')
        offsets = [struct.unpack('bb', positions.read(coordinate + 1 + layer * 2, 2))
                   for layer in range(2)]
        parts.append((raw, offsets))
    left = min(dx for _, offsets in parts for _, dx in offsets)
    pixels = [0] * (32 * 16)
    for raw, offsets in parts:
        for layer, (dy, dx) in enumerate(offsets):
            for y in range(16):
                for x in range(16):
                    if not raw[layer * 32 + y + (16 if x >= 8 else 0)] & (128 >> (x & 7)):
                        continue
                    xx, yy = x + dx - left, y + dy + 15
                    if not (0 <= xx < 32 and 0 <= yy < 16):
                        raise ValueError('Nontransparent sprite pixel would be clipped')
                    pixels[yy * 32 + xx] |= layer + 1
    return pixels, left


def tile_words(pixels):
    """24x32 pixels as 12 tiles in the column-major order of Mega Drive sprites."""
    words = []
    for tile_x in range(3):
        for tile_y in range(4):
            for row in range(8):
                word = 0
                for column in range(8):
                    word = (word << 4) | pixels[(tile_y * 8 + row) * 24 + tile_x * 8 + column]
                words.append(word)
    return words


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    data = load_rom(args.rom)
    out = ROOT / 'assets/player'
    out.mkdir(parents=True, exist_ok=True)
    gen = ROOT / 'sgdk/src/generated'
    palette = decode_room(data, 1, 0)[0]['palette_rgb333']
    colors = [tuple(round(v * 255 / 7) for v in color) for color in palette]
    records, arrays = [], []
    sheet = Image.new('RGB', (len(POSES) * 80, 160), (28, 33, 40))
    draw = ImageDraw.Draw(sheet)
    for index, (lower, upper) in enumerate(POSES):
        pixels, sources = extract_pose(data, lower, upper, index in JUMP_POSES,
                                       POSE_TOP.get(index, 30))
        image = Image.new('RGBA', (24, 32))
        image.putdata([(*colors[p], 255) if p else (0, 0, 0, 0) for p in pixels])
        image.save(out / f'pose-{index:02d}.png')
        sheet.paste(image.resize((72, 96), Image.Resampling.NEAREST), (index * 80 + 4, 24),
                    image.resize((72, 96), Image.Resampling.NEAREST))
        draw.text((index * 80 + 6, 130), f'{lower}/{upper}', fill='white')
        records.append({'pose': index, 'lower_frame': lower, 'upper_frame': upper,
                        'jumping': index in JUMP_POSES, 'top': POSE_TOP.get(index, 30),
                        'source': sources})
        arrays.append(tile_words(pixels))
    sheet.save(out / 'poses.png')
    stair_arrays = []
    for index, (lower, upper) in enumerate(STAIR_POSES):
        pixels, sources = extract_pose(data, lower, upper)
        image = Image.new('RGBA', (24, 32))
        image.putdata([(*colors[p], 255) if p else (0, 0, 0, 0) for p in pixels])
        image.save(out / f'stair-{index}.png')
        records.append({'stair_pose': index, 'lower_frame': lower, 'upper_frame': upper,
                        'top': 30, 'source': sources})
        stair_arrays.append(tile_words(pixels))
    death_words, death_left = [], []
    for index, (lower, upper) in enumerate(DEATH_POSES):
        pixels, left = extract_wide_pose(data, lower, upper)
        image = Image.new('RGBA', (32, 16))
        image.putdata([(*colors[p], 255) if p else (0, 0, 0, 0) for p in pixels])
        image.save(out / f'death-{index}.png')
        words = []
        for tile_x in range(4):
            for tile_y in range(2):
                for row in range(8):
                    word = 0
                    for column in range(8):
                        word = (word << 4) | pixels[(tile_y * 8 + row) * 32 + tile_x * 8 + column]
                    words.append(word)
        death_words.append(words)
        death_left.append(left)
        records.append({'death_pose': index, 'lower_frame': lower, 'upper_frame': upper,
                        'left': left, 'top': 15})
    jump = [int.from_bytes(bytes((v,)), 'big', signed=True) for v in data[0x2D95:0x2DA8]]
    fall = list(data[0x2F81:0x2F85])
    limits = list(data[0x3C78:0x3C8B])
    room_map = decode_room(data, 1, 0)[2]
    (gen / 'player_assets.h').write_text(f'''/* Generated by tools/extract_player.py. */
#ifndef VK_PLAYER_ASSETS_H
#define VK_PLAYER_ASSETS_H
#include "../player.h"
#define VK_PLAYER_POSES {len(POSES)}
#define VK_PLAYER_TILES_PER_POSE 12
#define VK_PLAYER_RECOIL_POSE 8     /* 8, 9: hit (right, left); 10, 11: landed */
extern const unsigned long vk_player_tiles[{len(POSES)}][96];
extern const VKByte vk_player_room[768];
extern const signed char vk_jump_deltas[19];
extern const VKByte vk_fall_deltas[4];
extern const VKByte vk_collision_limits[19];
/* Death pose (C420 = 6, 09/05 and 19/20 facing left): 4 x 2 tiles, top at Y - 15,
 * left at X + vk_player_death_left[facing]. Poses VK_PLAYER_DEATH_POSE + facing. */
#define VK_PLAYER_DEATH_POSE {len(POSES)}
#define VK_PLAYER_DEATH_TILES 8
extern const unsigned long vk_player_death_tiles[2][64];
extern const signed char vk_player_death_left[2];
/* Stair poses of the ROM (STAIR_POSES of tools/extract_player.py): VK_PLAYER_STAIR_POSE
 * + k + 4 x facing, k: 0 = 04/01, 1 = 05/02, 2 = 03/02, 3 = 04/02 (hit, C42F := 2). */
#define VK_PLAYER_STAIR_POSE (VK_PLAYER_DEATH_POSE + 2)
#define VK_PLAYER_STAIR_POSES {len(STAIR_POSES)}
extern const unsigned long vk_player_stair_tiles[{len(STAIR_POSES)}][96];
#endif
''')
    lines = ['/* Generated from the supplied ROM. */', '#include "player_assets.h"',
             f'const unsigned long vk_player_tiles[{len(POSES)}][96] = {{']
    for words in arrays:
        lines.append('    {')
        lines.extend('        ' + ','.join(f'0x{v:08X}UL' for v in words[i:i + 8]) + ','
                     for i in range(0, 96, 8))
        lines.append('    },')
    lines.append('};')
    lines.append('const unsigned long vk_player_death_tiles[2][64] = {')
    for words in death_words:
        lines.append('    {')
        lines.extend('        ' + ','.join(f'0x{v:08X}UL' for v in words[i:i + 8]) + ','
                     for i in range(0, 64, 8))
        lines.append('    },')
    lines.append('};')
    lines.append(f'const signed char vk_player_death_left[2] = {{{death_left[0]}, {death_left[1]}}};')
    lines.append(f'const unsigned long vk_player_stair_tiles[{len(STAIR_POSES)}][96] = {{')
    for words in stair_arrays:
        lines.append('    {')
        lines.extend('        ' + ','.join(f'0x{v:08X}UL' for v in words[i:i + 8]) + ','
                     for i in range(0, 96, 8))
        lines.append('    },')
    lines.append('};')
    for ctype, name, values in [('VKByte', 'vk_player_room', room_map),
                               ('signed char', 'vk_jump_deltas', jump),
                               ('VKByte', 'vk_fall_deltas', fall),
                               ('VKByte', 'vk_collision_limits', limits)]:
        lines.append(f'const {ctype} {name}[{len(values)}] = {{')
        lines.extend('    ' + ','.join(str(v) for v in values[i:i + 32]) + ','
                     for i in range(0, len(values), 32))
        lines.append('};')
    (gen / 'player_assets.c').write_text('\n'.join(lines) + '\n')
    manifest = {'poses': records, 'jump_delta_cpu': '6D95', 'jump_deltas': jump,
                'fall_delta_cpu': '6F81', 'fall_deltas': fall,
                'collision_limit_cpu': '7C78', 'collision_limits': limits,
                'anchor': {'x': 8, 'y': 31},
                'note': 'ROM sprite layers/positions and normal player colors; no attack poses yet.'}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'{len(POSES)} poses; 19 jump deltas; 19 stage collision limits')


if __name__ == '__main__':
    main()
