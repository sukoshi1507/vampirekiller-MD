#!/usr/bin/env python3
"""Reconstruct startup/title/castle scenes by executing selected original routines.
The harness substitutes synchronous BIOS/VDP graphics operations, not game logic.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image
from probe import load_rom, ROOT
from intro_harness import IntroHarness


def scene_tiles(pixels):
    tiles, lookup, tilemap = [bytes(32)], {bytes(32): 0}, []
    for ty in range(28):
        for tx in range(32):
            tile = bytearray()
            for yy in range(8):
                for xx in range(0, 8, 2):
                    row = (ty * 8 + yy) * 256 + tx * 8 + xx
                    tile.append((pixels[row] << 4) | pixels[row + 1])
            tile = bytes(tile)
            if tile not in lookup:
                lookup[tile] = len(tiles)
                tiles.append(tile)
            tilemap.append(lookup[tile])
    if len(tiles) >= 992:
        raise ValueError('Scene tiles overlap the reserved sprite region')
    words = [int.from_bytes(tile[i:i + 4], 'big') for tile in tiles for i in range(0, 32, 4)]
    return words, tilemap, len(tiles)


def sprite_words(pixels):
    words = []
    for tx in range(2):
        for ty in range(2):
            for yy in range(8):
                word = 0
                for xx in range(8):
                    word = (word << 4) | pixels[(ty * 8 + yy) * 16 + tx * 8 + xx]
                words.append(word)
    return words


def initialization(rom):
    harness = IntroHarness(rom)
    harness.call(0x4B59)
    return harness


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    rom = load_rom(args.rom)
    output = ROOT / 'assets/intro'
    output.mkdir(parents=True, exist_ok=True)
    gen = ROOT / 'sgdk/src/generated'
    h = initialization(rom)
    h.call(0x6202)
    for _ in range(98):
        h.call(0x624C)
    scenes = []

    def capture(name, harness, composite=False):
        image = harness.composite() if composite else harness.image()
        padded = Image.new('RGB', (256, 224))
        padded.paste(image, (0, 0))
        padded.save(output / f'{name}.png')
        pixels = bytearray(harness.indices()) + bytearray(256 * 12)
        if composite:
            for group in reversed(harness.sprite_groups()):
                for yy in range(16):
                    for xx in range(16):
                        color = group['pixels'][yy * 16 + xx]
                        x, y = group['x'] + xx, group['y'] + yy
                        if color and 0 <= x < 256 and 0 <= y < 212:
                            pixels[y * 256 + x] = color
        words, tilemap, count = scene_tiles(pixels)
        scenes.append({'name': name, 'words': words, 'map': tilemap,
                       'tile_count': count, 'palette': list(harness.palette),
                       'pixels_sha256': hashlib.sha256(pixels).hexdigest()})

    capture('konami', h)
    h.call(0x4D47)
    capture('title', h)
    h.call(0x44C6)
    h.cpu.memory[0xC41A] = 1
    h.call(0x63D3)
    capture('castle', h)
    sprites, sprite_lookup, frames = [], {}, []
    for frame in range(320):
        h.cpu.memory[0xC003] = (frame + 1) & 255
        h.call(0x4E93)
        groups = h.sprite_groups()
        if len(groups) != 8:
            raise ValueError(f'Unexpected intro sprite layout: frame {frame}')
        entries = []
        for group in groups:
            pixels = bytes(group['pixels'])
            if pixels not in sprite_lookup:
                sprite_lookup[pixels] = len(sprites)
                sprites.append(sprite_words(pixels))
            entries.append((group['x'], group['y'], sprite_lookup[pixels] * 4))
        frames.append(entries)
        if frame in (0, 60, 120, 240, 319):
            padded = Image.new('RGB', (256, 224))
            padded.paste(h.composite(), (0, 0))
            padded.save(output / f'castle-frame-{frame:03d}.png')
    if len(sprites) * 4 > 400:
        raise ValueError('Intro sprite data overlaps the font')
    maps, neighbors = [], []
    for room in range(3):
        g = initialization(rom)
        g.call(0x4D47)
        g.call(0x44C6)
        g.cpu.memory[0xC41A] = 1
        g.call(0x63D3)
        g.cpu.memory[0xC41A] = 0
        g.call(0x62D0)
        g.cpu.memory[0xD001] = room
        g.call(0x422C)
        if g.cpu.memory[0xD000] != 0 or g.cpu.memory[0xD001] != room:
            raise ValueError('Unexpected starting stage')
        capture(f'courtyard-{room}', g, composite=True)
        maps.append(list(g.cpu.memory[0xD100:0xD400]))
        neighbors.append(list(g.cpu.memory[0xC41C:0xC420]))
    g.cpu.memory[0xD000] = 1
    g.cpu.memory[0xD001] = 0
    g.cpu.memory[0xC411] = 1
    g.cpu.memory[0xC410] = 3
    g.call(0x422C)
    capture('interior', g, composite=True)
    interior_collision = list(g.cpu.memory[0xD100:0xD400])
    from extract_rooms import decode_room
    reference = decode_room(rom, 1, 0)[2]
    if interior_collision != reference:
        raise ValueError('Hall map differs from stage 1 room 0 decoded geometry')
    header = '''/* Generated by extract_intro.py from the supplied ROM. */
#ifndef VK_INTRO_ASSETS_H
#define VK_INTRO_ASSETS_H
#include <genesis.h>
typedef struct { const u32 *tiles; const u16 *map; u16 count; const u16 *palette; } VKScene;
typedef struct { s16 x, y; u16 tile; } VKIntroSprite;
extern const VKScene vk_scenes[7];
extern const u32 vk_intro_sprite_tiles[];
extern const u16 vk_intro_sprite_tile_count;
extern const VKIntroSprite vk_intro_frames[320][8];
extern const unsigned char vk_courtyard_maps[3][768];
extern const unsigned char vk_courtyard_neighbors[3][4];
#endif
'''
    (gen / 'intro_assets.h').write_text(header)
    lines = ['/* Generated ROM scene graphics and original entity trajectories. */', '#include "intro_assets.h"']

    def array(ctype, name, values, columns=16):
        lines.append(f'const {ctype} {name}[{len(values)}] = {{')
        lines.extend('    ' + ','.join(values[i:i + columns]) + ',' for i in range(0, len(values), columns))
        lines.append('};')

    for i, scene in enumerate(scenes):
        array('u32', f'scene_tiles_{i}', [f'0x{v:08X}' for v in scene['words']], 8)
        array('u16', f'scene_map_{i}', [str(v) for v in scene['map']], 32)
        array('u16', f'scene_palette_{i}', [f'0x{(b << 9) | (g << 5) | (r << 1):04X}' for r, g, b in scene['palette']])
    lines.append('const VKScene vk_scenes[7] = {')
    lines.extend(f'    {{scene_tiles_{i}, scene_map_{i}, {s["tile_count"]}, scene_palette_{i}}},'
                 for i, s in enumerate(scenes))
    lines.append('};')
    array('u32', 'vk_intro_sprite_tiles', [f'0x{v:08X}' for sprite in sprites for v in sprite], 8)
    lines.append(f'const u16 vk_intro_sprite_tile_count = {len(sprites) * 4};')
    lines.append('const VKIntroSprite vk_intro_frames[320][8] = {')
    lines.extend('    {' + ','.join('{' + f'{x},{y},{tile}' + '}' for x, y, tile in frame) + '},' for frame in frames)
    lines.append('};\nconst unsigned char vk_courtyard_maps[3][768] = {')
    for tilemap in maps:
        lines.append('    {')
        lines.extend('        ' + ','.join(str(v) for v in tilemap[i:i + 32]) + ',' for i in range(0, 768, 32))
        lines.append('    },')
    lines.append('};\nconst unsigned char vk_courtyard_neighbors[3][4] = {')
    lines.extend('    {' + ','.join(str(v) for v in n) + '},' for n in neighbors)
    lines.append('};')
    (gen / 'intro_assets.c').write_text('\n'.join(lines) + '\n')
    manifest = {'source_rom_sha256': hashlib.sha256(rom).hexdigest(),
                'scenes': [{k: v for k, v in s.items() if k not in ('words', 'map')} for s in scenes],
                'castle_frames': 320, 'sprite_groups_per_frame': 8, 'unique_sprite_images': len(sprites),
                'courtyard_neighbors': neighbors,
                'courtyard_collision_maps': maps,
                'interior_collision_map': interior_collision,
                'castle_hero_x': [frames[0][6][0], frames[240][6][0], frames[319][6][0]],
                'reconstruction': 'Original scene/entity/palette/command routines with synchronous VDP port adapters; no full MSX BIOS/timing/audio.',
                'frontend_adaptations': ['Konami fade duration', 'PRESS START replaces PUSH SPACE KEY',
                                         'courtyard flames are static', 'door activation is simplified']}
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'scene_tiles': [s['tile_count'] for s in scenes],
                      'castle_frames': 320, 'unique_sprites': len(sprites), 'neighbors': neighbors}, indent=2))


if __name__ == '__main__':
    main()
