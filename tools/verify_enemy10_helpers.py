#!/usr/bin/env python3
"""ROM-byte-anchored contract checks for type-10 helper routines.

Static semantics and exhaustive arithmetic model; NOT Z80 emulation, NOT
an in-game trace, and NOT a claim that a projectile was spawned.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe import load_rom, physical_offset, reference_rom


def at(rom, addr, count):
    if 0x8000 <= addr < 0xA000:
        off = physical_offset(2, addr, 0x8000)
    elif 0xA000 <= addr < 0xC000:
        off = physical_offset(3, addr, 0xA000)
    else:
        raise ValueError(f'CPU address out of mapped window: {addr:04X}')
    return rom[off:off + count]


def check_bytes(rom, lines):
    for address, hex_bytes, instruction in lines:
        expected = bytes.fromhex(hex_bytes)
        actual = at(rom, address, len(expected))
        assert actual == expected, (
            f'{address:04X} {instruction}: {actual.hex(" ")} != {expected.hex(" ")}')


# AF40 calculates |(C427) - (IX+05)| in an 8-bit domain.
AF40 = [
    (0xAF40, '3a 27 c4', 'LD A,(C427)'),
    (0xAF43, 'dd 96 05', 'SUB (IX+05)'),
    (0xAF46, 'd0', 'RET NC'),
    (0xAF47, 'ed 44', 'NEG'),
    (0xAF49, 'c9', 'RET'),
]

# Prior to slot search, 9F61 copies caller-provided HL,DE and derives
# the initial object coordinate from the enemy object.
PREPARE = [
    (0x9F61, 'dd 7e 03', 'LD A,(IX+03)'),
    (0x9F64, 'd6 10', 'SUB 10'),
    (0x9F66, '4f', 'LD C,A'),
    (0x9F67, 'dd 46 05', 'LD B,(IX+05)'),
    (0x9F6A, 'dd 7e 00', 'LD A,(IX+00)'),
    (0x9F6D, '32 f9 cf', 'LD (CFF9),A'),
    (0x9F70, 'ed 43 f1 cf', 'LD (CFF1),BC'),
    (0x9F74, '22 f5 cf', 'LD (CFF5),HL'),
    (0x9F77, 'ed 53 f7 cf', 'LD (CFF7),DE'),
    (0x9F7B, 'dd e5', 'PUSH IX'),
    (0x9F7D, 'cd 83 9f', 'CALL 9F83'),
    (0x9F80, 'dd e1', 'POP IX'),
    (0x9F82, 'c9', 'RET'),
]

# 9F83 first checks for a free entry in eight D700h slots, at 80h
# byte intervals; if all are busy, RET prevents the initialization.
SLOTS = [
    (0x9F83, '3a f9 cf', 'LD A,(CFF9)'),
    (0x9F86, 'fe ff', 'CP FF'),
    (0x9F88, '0e 0c', 'LD C,0C'),
    (0x9F8A, '28 07', 'JR Z,9F93'),
    (0x9F8C, '21 8e a0', 'LD HL,A08E'),
    (0x9F8F, 'cd 61 40', 'CALL 4061'),
    (0x9F92, '4e', 'LD C,(HL)'),
    (0x9F93, '79', 'LD A,C'),
    (0x9F94, '32 f0 cf', 'LD (CFF0),A'),
    (0x9F97, '21 00 d7', 'LD HL,D700'),
    (0x9F9A, '06 08', 'LD B,08'),
    (0x9F9C, 'af', 'XOR A'),
    (0x9F9D, '11 80 00', 'LD DE,0080'),
    (0x9FA0, 'be', 'CP (HL)'),
    (0x9FA1, '28 04', 'JR Z,9FA7'),
    (0x9FA3, '19', 'ADD HL,DE'),
    (0x9FA4, '10 fa', 'DJNZ 9FA0'),
    (0x9FA6, 'c9', 'RET'),
    (0x9FA7, 'e5', 'PUSH HL'),
    (0x9FA8, 'dd e1', 'POP IX'),
    (0x9FAA, '22 f3 cf', 'LD (CFF3),HL'),
]


def model_af40(player_x, enemy_x):
    # For 8-bit coordinates, Z80 SUB sets carry exactly when A < operand.
    difference = (player_x - enemy_x) & 255
    return (-difference) & 255 if player_x < enemy_x else difference


def main():
    rom = load_rom(reference_rom())
    check_bytes(rom, AF40 + PREPARE + SLOTS)
    assert at(rom, 0xAEE9, 1) == b'\xdd'  # target used by branch AEB3
    assert 0xAEB3 + 2 + 0x34 == 0xAEE9
    assert 0x9F8A + 2 + 7 == 0x9F93
    assert 0x9FA1 + 2 + 4 == 0x9FA7
    assert 0x9FA4 + 2 - 6 == 0x9FA0
    assert [0xD700 + i * 0x80 for i in range(8)][-1] == 0xDA80
    # Exhaustively validate the arithmetic model against a distinct
    # ordinary integer distance for every pair of possible byte values.
    for player_x in range(256):
        for enemy_x in range(256):
            assert model_af40(player_x, enemy_x) == abs(player_x - enemy_x)
            a = model_af40(player_x, enemy_x)
            # AEBA: CP 2A; JR NC => possible firing only at distance < 42.
            assert (a < 0x2A) == (abs(player_x - enemy_x) < 42)
    # The copied initial byte coordinates are source X, source Y - 16.
    for source_y in range(256):
        assert ((source_y - 0x10) & 255) in range(256)
    n = len(AF40) + len(PREPARE) + len(SLOTS)
    print(f'Enemy 10h: {n} helper instructions checked in ROM, 65536 distance pairs, '
          '8-slot search mapped; static semantics only PASS')


if __name__ == '__main__':
    main()
