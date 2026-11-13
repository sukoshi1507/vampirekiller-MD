#!/usr/bin/env python3
"""Death flame placement on the original Z80 code (research/death-flame.md): an object of each type is killed through the ROM routine 9A3E
(erase 99F6, drop roll 9ABE, creation 5F1D, kosarev/z80 emulator on the stage maps) and the flame (object 1E) it creates must sit at
(X, Y - D) with D = table 9B33 [type - 1]. Checks the whole table (types 1 to 21h), which the openMSX traces had covered only for some types
(01, 02, 04-08, 0A, 0B, 0D): 03 and 21h (stage 10 fishman) are 10h, not 0 as the port had it.
Usage: verify_death_flame_z80.py [ROM]"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from castle_room_objects import make_machine, call
from probe import load_rom, reference_rom

# type: expected D (ROM bytes 9B34..9B55, type 1 at 9B34, written here for the port tests: tests/test_death_flame.c kTypes)
EXPECTED = {1: 0x10, 2: 0x10, 3: 0x10, 4: 0, 5: 0, 6: 0x10, 7: 0, 8: 0, 9: 0x10, 0xA: 0x10, 0xB: 0x10, 0xC: 0, 0xD: 0, 0xF: 0x18, 0x10: 0x10,
            0x11: 0x20, 0x12: 0x18, 0x13: 0x18, 0x14: 0x18, 0x15: 0x18, 0x16: 0x18, 0x21: 0x10}

SPECIAL = {9, 0x11}


def build(data, stage, room):
    m = make_machine(bytes(data))
    mem = m.memory
    mem[0xD000] = stage; mem[0xD001] = room; mem[0xD002] = data[0x1E6A + stage]; mem[0xC41A] = 0
    call(m, 0x5A49); call(m, 0x5B1B); call(m, 0x4F83)
    for w in range(4):
        m.set_memory_block(0x4000 + w * 8192, bytes(data)[w * 8192:(w + 1) * 8192])
    mem[0xD012] = 0; mem[0xC420] = 0; mem[0xCE00] = 0; mem[0xCE03] = 0
    for a in range(0xD600, 0xD700, 4): mem[a] = 0xE0
    for k in range(7):
        for i in range(0x80): mem[0xC800 + 0x80 * k + i] = 0
    return m


def main():
    data = load_rom(Path(sys.argv[1]) if len(sys.argv) > 1 else reference_rom())
    for typ, want in sorted(EXPECTED.items()):
        assert data[0x9B33 - 0x4000 + typ] == want, f'table 9B33[{typ:02X}] = {data[0x9B33 - 0x4000 + typ]:02X}'
        m = build(data, 10, 1)
        mem = m.memory
        b = 0xC800 + 0x80 * 2
        X, Y = 120, 176
        mem[b] = typ; mem[b + 3] = Y; mem[b + 5] = X; mem[b + 0x7E] = 1; mem[b + 0x7F] = 1; mem[b + 0xE] = 1
        mem[0xCF40] = 1   # not the 32nd death (13h drop)
        m.ix = b
        call(m, 0x9A3E)
        flames = [(mem[0xC800 + 0x80 * k + 5], mem[0xC800 + 0x80 * k + 3]) for k in range(7) if mem[0xC800 + 0x80 * k] == 0x1E]
        if typ in SPECIAL:   # no ordinary flame: 09 goes dormant (9AA9), 11h is not erased by this path
            assert not flames, f'type {typ:02X}: unexpected flames {flames}'
            continue
        assert flames == [(X, Y - want)], f'type {typ:02X}: flames {flames}, expected {[(X, Y - want)]}'
    print(f'death flame: {len(EXPECTED)} types in the table 9B33, {len(EXPECTED) - len(SPECIAL)} flames made by the ROM 9A3E on the Z80 emulator at (X, Y - D) PASS')


if __name__ == '__main__':
    main()
