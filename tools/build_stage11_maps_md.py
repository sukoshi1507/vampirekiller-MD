#!/usr/bin/env python3
"""Generate the collision maps and room links of stage 11 for SGDK (sgdk/src/generated/stage11_maps.c/.h).

Maps: the 768 cells of D100 after the original routine 4F83 (as tools/extract_collision_maps.py for stages 1-10).
Links: the original routine B99A (C41C-C41F: up, down, left, right), as tools/extract_room_links.py.
Usage: build_stage11_maps_md.py [ROM]. Requires z80==1.2.0. Never hand edit the output.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from castle_room_objects import make_machine, call  # noqa: E402
from probe import load_rom, reference_rom  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
STAGE = 11
COUNT = 6   # ROOM_COUNTS_AUDIT[11] (decode_room_links.py)


def generate(data):
    maps, links = [], []
    for room in range(COUNT):
        m = make_machine(data)
        m.memory[0xD000] = STAGE
        m.memory[0xD001] = room
        call(m, 0x4F83)
        maps.append(list(m.memory[0xD100:0xD400]))
        m = make_machine(data)
        m.set_memory_block(0xA000, data[13 * 8192:14 * 8192])
        m.memory[0xD000] = STAGE
        m.memory[0xD001] = room
        call(m, 0xB99A)
        links.append(list(m.memory[0xC41C:0xC420]))
    c = ['/* Full 768-byte collision maps verified against original MSX2 routine 4F83 (tools/build_stage11_maps_md.py).',
         ' * Map-byte fidelity does not imply complete gameplay or enemy validation. */',
         '#include "stage11_maps.h"',
         'const VKByte vk_stage11_collision[VK_STAGE11_ROOM_COUNT][768] = {']
    for cells in maps:
        c.append('    {')
        c += ['        ' + ','.join(map(str, cells[i:i + 32])) + ',' for i in range(0, 768, 32)]
        c.append('    },')
    c.append('};')
    c.append('const VKRoomLink vk_stage11_links[VK_STAGE11_ROOM_COUNT] = {')
    for row in links:
        c.append('    {' + ','.join('VK_ROOM_NONE' if v == 255 else str(v) for v in row) + ',0},')
    c.append('};')
    out = ROOT / 'sgdk/src/generated'
    (out / 'stage11_maps.c').write_text('\n'.join(c) + '\n')
    (out / 'stage11_maps.h').write_text(
        '/* Full 768-byte collision maps verified against original MSX2 routine 4F83.\n'
        ' * Map-byte fidelity does not imply complete gameplay or enemy validation. */\n'
        '#ifndef VK_STAGE11_MAPS_H\n#define VK_STAGE11_MAPS_H\n#include "../room_manager.h"\n'
        f'#define VK_STAGE11_ROOM_COUNT {COUNT}\n'
        'extern const VKByte vk_stage11_collision[VK_STAGE11_ROOM_COUNT][768];\n'
        'extern const VKRoomLink vk_stage11_links[VK_STAGE11_ROOM_COUNT];\n#endif\n')
    print('links', links)


if __name__ == '__main__':
    generate(load_rom(Path(sys.argv[1]) if len(sys.argv) == 2 else reference_rom()))
