#!/usr/bin/env python3
"""Decode les fonds de salles via les routines 4FAF, 564C, 570D et 5780.
Sorties: cartes, palettes, PNG et ressources C SGDK. Aucun objet dynamique.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from probe import load_rom, ROOT


class RomView:
    def __init__(self, data, banks):
        self.data, self.banks = data, banks

    def offset(self, address):
        if not 0x4000 <= address < 0xC000:
            raise ValueError(f'Adresse ROM hors fenetres: {address:04X}')
        return self.banks[(address - 0x4000) // 8192] * 8192 + (address & 8191)

    def read(self, address, count):
        return bytes(self.data[self.offset(address + i)] for i in range(count))

    def byte(self, address):
        return self.read(address, 1)[0]

    def word(self, address):
        return struct.unpack('<H', self.read(address, 2))[0]


def palette_patch(view, address, palette):
    start = address
    for _ in range(16):
        index = view.byte(address)
        if index == 255:
            return {'cpu_address': f'{start:04X}', 'file_offset': f'{view.offset(start):05X}'}
        if index > 15:
            raise ValueError('Index palette invalide')
        rb, green = view.read(address + 1, 2)
        if rb & 0x88 or green & 0xF8:
            raise ValueError('Composantes hors RGB333')
        palette[index] = ((rb >> 4) & 7, green & 7, rb & 7)
        address += 3
    raise ValueError('Palette sans terminateur FF')


def decode_room(data, stage, room):
    fixed = RomView(data, (0, 1, 2, 3))
    layout = RomView(data, (0, 11, 12, 13))
    colors = RomView(data, (0, 1, 9, 10))
    start = layout.byte(0x6000 + stage)
    map_address = layout.word(0x6013 + 2 * (start + room))
    metatile_address = layout.word(0x7EBB + 2 * stage)
    metatiles = layout.read(map_address, 48)
    tilemap = [0] * 768
    for i, metatile in enumerate(metatiles):
        cells = layout.read(metatile_address + 16 * metatile, 16)
        for j, tile in enumerate(cells):
            x, y = (i % 8) * 4 + j % 4, (i // 8) * 4 + j // 4
            tilemap[y * 32 + x] = tile
    graphics_view = RomView(data, (0, 4, 5, 6) if stage < 13 else (0, 4, 7, 8))
    graphics_address = fixed.word(0x5742 + 2 * stage)
    # 564C charge 191 tuiles dans les emplacements 1..191; zero reste vierge.
    graphics = bytes(32) + graphics_view.read(graphics_address, 191 * 32)
    palette = [None] * 16
    patches = [palette_patch(colors, 0xBF88, palette)]
    patches.append(palette_patch(colors, colors.word(0xBEA7 + stage * 2), palette))
    room_table = colors.word(0x9AB0 + (stage - 1) * 2)
    patches.append(palette_patch(colors, colors.word(room_table + room * 4 + 2), palette))
    used_colors = {c for tile in set(tilemap[64:]) if tile < 192
                   for byte in graphics[tile * 32:(tile + 1) * 32]
                   for c in (byte >> 4, byte & 15)}
    undefined_colors = [i for i, c in enumerate(palette) if c is None]
    if used_colors & set(undefined_colors):
        raise ValueError(f'Couleurs utilisees non definies: stage={stage} room={room}')
    # Certaines entrees sont conservees par le jeu mais inutilisees dans ce fond.
    palette = [c if c is not None else (0, 0, 0) for c in palette]
    missing = sorted(set(tilemap[64:]) - set(range(192)))
    manifest = {
        'stage': stage, 'room': room,
        'status': 'static_background_decoded' if not missing else 'unresolved_tile_ids',
        'map_cpu': f'{map_address:04X}', 'map_file_offset': f'{layout.offset(map_address):05X}',
        'metatile_table_cpu': f'{metatile_address:04X}',
        'metatile_table_file_offset': f'{layout.offset(metatile_address):05X}',
        'graphics_cpu': f'{graphics_address:04X}',
        'graphics_file_offset': f'{graphics_view.offset(graphics_address):05X}',
        'graphics_sha256': hashlib.sha256(graphics).hexdigest(),
        'palette_patches': patches, 'palette_rgb333': palette,
        'undefined_but_unused_colors': undefined_colors,
        'unresolved_tile_ids': missing,
        'visible_source_rect': {'x': 0, 'y': 16, 'width': 256, 'height': 176},
        'destination_rect': {'x': 0, 'y': 32, 'width': 256, 'height': 176},
        'limitations': ['no dynamic objects', 'no VRAM modifications after load',
                        'not compared to running MSX2 yet'],
    }
    return manifest, graphics, tilemap


def render(graphics, tilemap, palette, rgb_override=None):
    from PIL import Image
    image = Image.new('RGB', (256, 176))
    rgb = rgb_override or [tuple(round(v * 255 / 7) for v in c) for c in palette]
    for index, tile in enumerate(tilemap[64:]):
        if tile >= 192:
            block = Image.new('RGB', (8, 8), (255, 0, 255))
        else:
            raw = graphics[tile * 32:(tile + 1) * 32]
            block = Image.new('RGB', (8, 8))
            block.putdata([rgb[v] for byte in raw for v in (byte >> 4, byte & 15)])
        image.paste(block, ((index % 32) * 8, (index // 32) * 8))
    return image


def write_c(path, supported, unique_graphics):
    path.mkdir(parents=True, exist_ok=True)
    (path / 'rooms.h').write_text('''/* Generated by tools/extract_rooms.py. */
#ifndef VK_ROOMS_H
#define VK_ROOMS_H
#include <genesis.h>
#define VK_TILE_COUNT 192
typedef struct { u16 first; u16 count; u16 tileset; } VKStage;
extern const VKStage vk_stages[18];
extern const u32 vk_tiles[][VK_TILE_COUNT * 8];
extern const u16 vk_maps[][704];
extern const u16 vk_palettes[][16];
#endif
''')
    lines = ['/* Generated from the reference ROM. Static backgrounds only. */', '#include "rooms.h"']
    lines.append('const VKStage vk_stages[18] = {')
    cursor = 0
    for stage in range(1, 19):
        group = [r for r in supported if r[0]['stage'] == stage]
        assert [r[0]['room'] for r in group] == list(range(len(group)))
        tileset = list(unique_graphics).index(group[0][0]['graphics_sha256'])
        lines.append(f'    {{{cursor}, {len(group)}, {tileset}}},')
        cursor += len(group)
    lines.append('};\nconst u32 vk_tiles[][VK_TILE_COUNT * 8] = {')
    for graphics in unique_graphics.values():
        words = struct.unpack('>1536I', graphics)
        lines.append('    {')
        lines.extend('        ' + ','.join(f'0x{v:08X}' for v in words[i:i + 8]) + ','
                     for i in range(0, len(words), 8))
        lines.append('    },')
    lines.append('};\nconst u16 vk_maps[][704] = {')
    for manifest, _, tilemap in supported:
        lines.append(f"    {{ /* stage {manifest['stage']} room {manifest['room']} */")
        visible = tilemap[64:]
        lines.extend('        ' + ','.join(str(v) for v in visible[i:i + 32]) + ','
                     for i in range(0, len(visible), 32))
        lines.append('    },')
    lines.append('};\nconst u16 vk_palettes[][16] = {')
    for manifest, _, _ in supported:
        # Mega Drive CRAM: 0000 BBB0 GGG0 RRR0.
        words = [(b << 9) | (g << 5) | (r << 1) for r, g, b in manifest['palette_rgb333']]
        lines.append('    {' + ','.join(f'0x{v:04X}' for v in words) + '},')
    lines.append('};')
    (path / 'rooms.c').write_text('\n'.join(lines) + '\n')


def main():
    from PIL import Image, ImageDraw
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / 'assets')
    parser.add_argument('--c-out', type=Path, default=ROOT / 'sgdk/src/generated')
    args = parser.parse_args()
    data = load_rom(args.rom)
    layout = RomView(data, (0, 11, 12, 13))
    offsets = list(layout.read(0x6000, 19)) + [156]
    assert layout.word(0x6013 + 156 * 2) == 0
    assert all(a < b for a, b in zip(offsets, offsets[1:]))
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'rooms').mkdir(exist_ok=True)
    manifest_all, supported, unique = [], [], {}
    sheet = Image.new('RGB', (3 * 256, 6 * 200), (18, 20, 24))
    draw = ImageDraw.Draw(sheet)
    for stage in range(1, 19):
        for room in range(offsets[stage + 1] - offsets[stage]):
            manifest, graphics, tilemap = decode_room(data, stage, room)
            name = f'stage-{stage:02d}-room-{room:02d}'
            manifest['png'] = f'rooms/{name}.png'
            image = render(graphics, tilemap, manifest['palette_rgb333'])
            image.save(args.out / manifest['png'])
            (args.out / 'rooms' / (name + '.tilemap.bin')).write_bytes(bytes(tilemap))
            if room == 0:
                x, y = ((stage - 1) % 3) * 256, ((stage - 1) // 3) * 200
                draw.text((x + 5, y + 4), f'STAGE {stage:02d} / ROOM 00', fill=(238, 235, 225))
                sheet.paste(image, (x, y + 22))
            manifest_all.append(manifest)
            if not manifest['unresolved_tile_ids']:
                supported.append((manifest, graphics, tilemap))
                unique.setdefault(manifest['graphics_sha256'], graphics)
    sheet.save(args.out / 'stage-overview.png')
    write_c(args.c_out, supported, unique)
    summary = {'rom_sha256': hashlib.sha256(data).hexdigest(), 'stage_count': 18,
               'room_records': len(manifest_all), 'viewer_rooms': len(supported),
               'unique_tilesets': len(unique),
               'note': 'Static backgrounds; not an emulator capture or gameplay reconstruction.',
               'rooms': manifest_all}
    (args.out / 'manifest.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps({k: v for k, v in summary.items() if k != 'rooms'}, indent=2))


if __name__ == '__main__':
    main()
