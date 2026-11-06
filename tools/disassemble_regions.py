#!/usr/bin/env python3
"""Listing de regions de code selectionnees. Depend de z80dis==1.0.6.
Les destinations d'appels bankes necessitent le mapping actif a l'execution.
"""
import argparse
from pathlib import Path
from z80dis import z80
from probe import load_rom, ROOT

REGIONS = [(0x417D, 0x4300), (0x4314, 0x44E9), (0x483E, 0x484C),
           (0x4A27, 0x4ACB), (0x4B41, 0x4B59), (0x4F83, 0x5076),
           (0x53B6, 0x53FC), (0x564C, 0x5670), (0x570D, 0x5742),
           (0x5780, 0x57B4), (0x62D0, 0x6333)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    data = load_rom(args.rom)
    lines = ['; Regions selectionnees. z80dis 1.0.6.',
             '; Mapping du listing: 00/01/02/03 dans 4000/6000/8000/A000.',
             '; Les appels doivent etre interpretes selon le mapping actif.',
             '; Adresse CPU | octets | instruction. Noms semantiques non generes.']
    count = 0
    for start, end in REGIONS:
        lines.append(f'\n; Region {start:04X}-{end - 1:04X}')
        address = start
        while address < end:
            instruction = z80.decode(data[address - 0x4000:address - 0x4000 + 4], address)
            if instruction.status != z80.DECODE_STATUS.OK or address + instruction.len > end:
                raise ValueError(f'Instruction invalide ou hors region: {address:04X}')
            raw = data[address - 0x4000:address - 0x4000 + instruction.len]
            lines.append(f'{address:04X} | {raw.hex(" ").upper():11s} | {z80.disasm(instruction)}')
            address += instruction.len
            count += 1
    (ROOT / 'research/loading-and-states.asm').write_text('\n'.join(lines) + '\n')
    print(f'{count} instructions decodees; aucun traitement des cibles indirectes.')


if __name__ == '__main__':
    main()
