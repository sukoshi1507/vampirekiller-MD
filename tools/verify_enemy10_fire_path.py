#!/usr/bin/env python3
"""Verify the *static* type-10 firing-path instructions in mapper bank 2.

This verifies instruction encoding and local branch destinations, not Z80
execution or an in-game state transition. No timing inference is made.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe import load_rom, reference_rom, physical_offset


def inspect(rom):
    def data(cpu, n):
        bank, window = (2, 0x8000) if cpu < 0xA000 else (3, 0xA000)
        pos = physical_offset(bank, cpu, window)
        return rom[pos:pos+n]

    # Explicitly account for each complete instruction on the firing branch.
    # Address, opcode, textual interpretation; no heuristic byte scanning.
    instructions = [
        (0xAEAF, 'dd cb 01 4e', 'BIT 1,(IX+01)'),
        (0xAEB3, '20 34', 'JR NZ,AEE9'),
        (0xAEB5, 'cd 40 af', 'CALL AF40'),
        (0xAEB8, 'fe 2a', 'CP 2A'),
        (0xAEBA, '30 23', 'JR NC,AEDF'),
        (0xAEBC, 'dd cb 10 46', 'BIT 0,(IX+10)'),
        (0xAEC0, 'c8', 'RET Z'),
        (0xAEC1, 'dd cb 01 ce', 'SET 1,(IX+01)'),
        (0xAEC5, 'dd e5', 'PUSH IX'),
        (0xAEC7, 'e1', 'POP HL'),
        (0xAEC8, '22 fc cf', 'LD (CFFC),HL'),
        (0xAECB, '21 00 00', 'LD HL,0000'),
        (0xAECE, '11 00 04', 'LD DE,0400'),
        (0xAED1, '3a 27 c4', 'LD A,(C427)'),
        (0xAED4, 'dd be 05', 'CP (IX+05)'),
        (0xAED7, '30 03', 'JR NC,AEDC'),
        (0xAED9, '11 00 fc', 'LD DE,FC00'),
        (0xAEDC, 'c3 61 9f', 'JP 9F61'),
    ]
    for addr, enc, meaning in instructions:
        b = bytes.fromhex(enc)
        assert data(addr, len(b)) == b, f'{addr:04X} {meaning}: ROM mismatch'
    for addr, expected in [(0xAEB3, 0xAEE9), (0xAEBA, 0xAEDF), (0xAED7, 0xAEDC)]:
        b = data(addr, 2)
        delta = b[1] if b[1] < 128 else b[1] - 256
        assert addr + 2 + delta == expected
    assert data(0x9F61, 6) == bytes.fromhex('dd 7e 03 d6 10 4f')
    return instructions


def main():
    instructions = inspect(load_rom(reference_rom()))
    if '--verbose' in sys.argv:
        for pc, op, meaning in instructions:
            print(f'{pc:04X}: {op:<12} {meaning}')
    print(f'Enemy 10h: {len(instructions)} instructions on conditional firing path verified in ROM; static only PASS')


if __name__ == '__main__':
    main()
