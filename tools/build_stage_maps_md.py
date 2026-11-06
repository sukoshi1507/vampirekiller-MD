#!/usr/bin/env python3
"""Generate the collision maps and room links of castle stage 2 for SGDK.

Usage: python3 -I tools/build_stage_maps_md.py [ROM_OR_ZIP]
Writes sgdk/src/generated/stage2_maps.c and .h (never hand edit).

Collision map of a room = its decoded tilemap (tools/extract_rooms.py, routine
4FAF), the map the game samples at 7D2F; the two hidden top rows are zero-filled,
as for the hall rooms 1-7 of stage1_collision.c (which this method reproduces
exactly). Links = the ROM table at B9D3 (tools/decode_room_links.py); room 0 of
stage 2 matches the trace of research/door-room7.md (C41C 02, C41D 04, C41E FF,
C41F 01).
"""
import hashlib
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from decode_room_links import decode  # noqa: E402
from extract_rooms import decode_room  # noqa: E402
from probe import load_rom, reference_rom  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'sgdk/src/generated'
ROM_SHA256 = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'
STAGE = 2
HIDDEN = 64


def link(value):
    return 'VK_ROOM_NONE' if value is None else str(value)


def generate(rom_path=None):
    data = load_rom(Path(rom_path) if rom_path else reference_rom())
    digest = hashlib.sha256(data).hexdigest()
    if digest != ROM_SHA256:
        raise ValueError(f'ROM inattendue: {digest}')
    # The method must give back the hall maps of stage1_collision.c (rooms 1-7).
    rooms = decode(data)[STAGE]['rooms']
    maps = []
    for room in range(len(rooms)):
        _, _, tilemap = decode_room(data, STAGE, room)
        maps.append([0] * HIDDEN + list(tilemap[HIDDEN:]))
    count = len(maps)

    lines = [
        '/* Generated from the Vampire Killer MSX2 ROM by tools/build_stage_maps_md.py.',
        f' * Stage {STAGE}: collision maps (decoded tilemaps, hidden rows zero-filled)',
        ' * and room links (ROM table B9D3). Never hand edit. */',
        '#include "stage2_maps.h"',
        f'const VKByte vk_stage2_collision[{count}][768] = {{',
    ]
    for tilemap in maps:
        lines.append('    {')
        for i in range(0, 768, 32):
            lines.append('        ' + ','.join(str(v) for v in tilemap[i:i + 32]) + ',')
        lines.append('    },')
    lines.append('};')
    lines.append(f'const VKRoomLink vk_stage2_links[{count}] = {{')
    for r in rooms:
        lines.append(f"    {{{link(r['UP'])},{link(r['DOWN'])},{link(r['LEFT'])},{link(r['RIGHT'])},0}},")
    lines.append('};')
    (OUT / 'stage2_maps.c').write_text('\n'.join(lines) + '\n')

    header = [
        '/* Generated stage 2 maps and links. Never hand edit (tools/build_stage_maps_md.py). */',
        '#ifndef VK_STAGE2_MAPS_H',
        '#define VK_STAGE2_MAPS_H',
        '#include "../room_manager.h"',
        f'#define VK_STAGE2_ROOM_COUNT {count}',
        f'extern const VKByte vk_stage2_collision[{count}][768];',
        f'extern const VKRoomLink vk_stage2_links[{count}];',
        '#endif',
    ]
    (OUT / 'stage2_maps.h').write_text('\n'.join(header) + '\n')
    print(f'stage {STAGE}: {count} rooms -> {OUT}')


if __name__ == '__main__':
    generate(sys.argv[1] if len(sys.argv) == 2 else None)
