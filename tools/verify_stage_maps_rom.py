#!/usr/bin/env python3
"""Compare the C maps and links of stages 10-18 with the MSX2 ROM, independently of the generators.

Maps: output of the original Z80 routine 4F83 (verify_z80_maps.execute_map, CPU + mapper harness)
against vk_stage10_collision, vk_stage11_collision and vk_stage12..18_tiles. Links: ROM table B9D3
(decode_room_links.decode) against vk_stage10_links, vk_stage11_links and vk_stage12..18_raw_links.
Not covered: graphics taken from openMSX snapshots, objects, enemies, gameplay.
Usage: python3 tools/verify_stage_maps_rom.py [ROM_OR_ZIP]   (default: VK_ROM_PATH, else the ZIP at the root)
Requires z80==1.2.0. Exit code 1 on any mismatch.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from probe import load_rom, reference_rom  # noqa: E402
from verify_z80_maps import execute_map  # noqa: E402
from decode_room_links import decode  # noqa: E402

GENERATED = ROOT / 'sgdk/src/generated'
STAGES = (
    (10, 'stage10_maps.c', 'vk_stage10_collision', 'vk_stage10_links'),
    (11, 'stage11_maps.c', 'vk_stage11_collision', 'vk_stage11_links'),
) + tuple((s, f'stage{s}_foundations.c', f'vk_stage{s}_tiles', f'vk_stage{s}_raw_links') for s in range(12, 19))


def _array_body(text, name):
    return text.split(name + '[', 1)[1].split('= {', 1)[1].split('};', 1)[0]


def _maps(text, name):
    body = re.sub(r'/\*.*?\*/', '', _array_body(text, name), flags=re.S)
    flat = [int(x) for x in re.findall(r'\d+', body)]
    if len(flat) % 768:
        raise ValueError(f'{name}: {len(flat)} bytes, not a multiple of 768')
    return [bytes(flat[i:i + 768]) for i in range(0, len(flat), 768)]


def _links(text, name):
    rows = []
    for row in re.findall(r'\{([^{}]*)\}', _array_body(text, name)):
        cells = [None if t.strip() == 'VK_ROOM_NONE' else int(t) for t in row.split(',')]
        rows.append(cells[:4])
    return rows


def check(data):
    """Return {stage: (room_count, [problems])}."""
    graph = decode(data)
    report = {}
    for stage, fname, mapname, linkname in STAGES:
        text = (GENERATED / fname).read_text()
        maps = _maps(text, mapname)
        problems = []
        for room, cmap in enumerate(maps):
            z80 = execute_map(data, stage, room)[0]
            diff = sum(a != b for a, b in zip(z80, cmap))
            if diff:
                problems.append(f'room {room}: {diff} bytes differ from 4F83')
        rom_links = [[d[k] for k in ('UP', 'DOWN', 'LEFT', 'RIGHT')] for d in graph[stage]['rooms']]
        c_links = _links(text, linkname)
        if len(c_links) != len(rom_links) or len(maps) != len(rom_links):
            problems.append(f'room count: maps {len(maps)}, C links {len(c_links)}, ROM {len(rom_links)}')
        for room, (c, r) in enumerate(zip(c_links, rom_links)):
            if c != r:
                problems.append(f'room {room}: C links {c}, ROM {r}')
        report[stage] = (len(maps), problems)
    return report


def main():
    rom = Path(sys.argv[1]) if len(sys.argv) > 1 else reference_rom()
    report = check(load_rom(rom))
    failed = False
    for stage, (count, problems) in report.items():
        print(f'stage {stage}: {count} maps vs 4F83, links vs B9D3: {"PASS" if not problems else "FAIL"}')
        for p in problems:
            print('   ', p)
        failed |= bool(problems)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
