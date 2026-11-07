#!/usr/bin/env python3
"""Reproducible *static* checks for type 10h. Not an AI trace.

Mapping: 9942 table in mapper bank 02 at CPU 8000h; handler ADF8
and code around 9F61 map to the contiguous 8000h-BFFFh ROM image.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from probe import load_rom, reference_rom  # noqa: E402


def at(cpu):
    if not 0x8000 <= cpu < 0xC000:
        raise ValueError('CPU address out of 8000h-BFFFh window')
    return 0x4000 + cpu - 0x8000


def inspect(rom):
    dispatch = int.from_bytes(rom[at(0x9942 + 2 * (0x10 - 1)):
                                  at(0x9942 + 2 * (0x10 - 1)) + 2], 'little')
    assert dispatch == 0xADF8, f'type 10h points to {dispatch:04X}, not ADF8'
    # ADF8 begins with BIT 0,(IX+1) / JR NZ,+38h. This is a
    # binary signature; it says nothing about the bit's meaning.
    assert rom[at(0xADF8):at(0xADF8) + 5] == bytes.fromhex('dd cb 01 46 20')
    # 9F61 begins by reading (IX+3), subtracting 10h and saving to C.
    assert rom[at(0x9F61):at(0x9F61) + 7] == bytes.fromhex('dd 7e 03 d6 10 4f dd')
    # JP 9F61 present at AEDC within ADF8..AF5F, without claiming
    # reachability or that this is necessarily a projectile spawn.
    window = rom[at(0xADF8):at(0xAF5F)]
    direct_jumps = [f'{0xADF8 + i:04X}' for i in range(len(window) - 2)
                    if window[i:i + 3] == bytes.fromhex('c3 61 9f')]
    assert 'AEDC' in direct_jumps, direct_jumps
    return {
        'table': '9942', 'type': '10', 'update_entry': f'{dispatch:04X}',
        'entry_signature': rom[at(0xADF8):at(0xADF8) + 5].hex(' '),
        'target_9F61_signature': rom[at(0x9F61):at(0x9F61) + 7].hex(' '),
        'raw_jp_9F61_candidates_in_ADF8_AF5F': direct_jumps,
        'proof': 'ROM bytes and dispatch pointer only',
        'not_proven': ['jump reachability', 'state transitions', 'collision',
                       'sprites', 'projectile frame timing', 'in-game execution'],
    }


def main():
    rom = load_rom(reference_rom())
    result = inspect(rom)
    if '--json' in sys.argv[1:]:
        print(json.dumps(result, indent=2))
    else:
        print('type 10h: ROM dispatch 9942->ADF8 and byte signatures PASS; no frame trace')


if __name__ == '__main__':
    main()
