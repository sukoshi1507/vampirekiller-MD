#!/usr/bin/env python3
"""Decode the original room-link table of Vampire Killer (MSX2) from the ROM.

Static reading, no emulation. Table layout inferred from the Z80 code at
bank 13 (file 0x1B9BD..0x1B9D2 and 0x1B963..0x1B98C, writer 0x1B987 = LD (D001),A):
  - P[s] = 16-bit pointer at CPU 0xB9D3 + 2*s (window A000, bank 13), stage s.
  - Each room r of stage s has 2 bytes at P[s] + 2*r: b0 = (UP<<4)|DOWN,
    b1 = (LEFT<<4)|RIGHT. A nibble 0xF means no link.
  - Stage 0 is the courtyard (3 rooms), stages 1..18 are the castle.
The nibble order (UP/DOWN/LEFT/RIGHT) is inferred from traces: the LEFT/RIGHT
nibbles match the openMSX walks in research/room-links.md, and UP/DOWN are
reciprocal in the table. Stage 2 right of the courtyard is NOT in the table:
it is the castle entry, handled by code at 0x4357 (D000 += 1, D001 = 0).

Usage: decode_room_links.py [ROM_OR_ZIP]   (default: reference ROM from probe.py)
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe import load_rom, DEFAULT_ROM

TABLE_CPU = 0xB9D3
ROOM_COUNTS_AUDIT = [3, 8, 6, 6, 6, 6, 6, 9, 8, 9, 9, 6, 12, 12, 8, 10, 10, 12, 10]  # NAVIGATION_AUDIT.md (stages 0..18)
DIRS = ('UP', 'DOWN', 'LEFT', 'RIGHT')
OPP = {'UP': 'DOWN', 'DOWN': 'UP', 'LEFT': 'RIGHT', 'RIGHT': 'LEFT'}


def cpu_to_file(addr):
    """Window A000 is bank 13 for this table (file offset 0x1A000 + offset)."""
    assert 0xA000 <= addr < 0xC000
    return 0x1A000 + (addr - 0xA000)


def decode(data):
    stages = []
    ptrs = []
    for s in range(19):
        o = cpu_to_file(TABLE_CPU + 2 * s)
        ptrs.append(data[o] | (data[o + 1] << 8))
    for s in range(19):
        base = cpu_to_file(ptrs[s])
        rooms = []
        for r in range(ROOM_COUNTS_AUDIT[s]):
            b0, b1 = data[base + 2 * r], data[base + 2 * r + 1]
            nib = {'UP': b0 >> 4, 'DOWN': b0 & 15, 'LEFT': b1 >> 4, 'RIGHT': b1 & 15}
            rooms.append({d: (None if v == 15 else v) for d, v in nib.items()})
        stages.append({'stage': s, 'pointer': ptrs[s], 'rooms': rooms})
    return stages


def check(stages):
    problems = []
    for st in stages:
        if st['pointer'] < 0xA000 or st['pointer'] >= 0xC000:
            problems.append(f"stage {st['stage']}: pointer {st['pointer']:04X} outside A000-BFFF")
        for r, room in enumerate(st['rooms']):
            for d, t in room.items():
                if t is None:
                    continue
                if t >= len(st['rooms']):
                    problems.append(f"s{st['stage']} r{r} {d} -> {t}: out of range")
                    continue
                back = st['rooms'][t][OPP[d]]
                if back != r:
                    problems.append(f"s{st['stage']} r{r} {d}->{t} but r{t} {OPP[d]}->{back}")
    return problems


def main():
    rom = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_ROM
    data = load_rom(rom)
    stages = decode(data)
    for st in stages:
        print(f"stage {st['stage']} ptr={st['pointer']:04X}")
        for r, room in enumerate(st['rooms']):
            cells = ' '.join(f"{d[0]}={'-' if room[d] is None else room[d]}" for d in DIRS)
            print(f"  room {r}: {cells}")
    problems = check(stages)
    print('reciprocity problems:', len(problems))
    for p in problems:
        print('  ', p)


if __name__ == '__main__':
    main()
