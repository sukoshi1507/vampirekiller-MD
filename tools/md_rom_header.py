#!/usr/bin/env python3
"""Fix the Mega Drive header of a padded ROM: ROM end address (0x1A4) and the
standard checksum (0x18E, 16-bit sum of the big-endian words from 0x200 to the
end), which SGDK's sizebnd replaces with its own XOR. Usage: md_rom_header.py ROM..."""
import struct
import sys


def fix(path):
    data = bytearray(open(path, 'rb').read())
    if data[0x100:0x104] != b'SEGA':
        sys.exit(f'{path}: no SEGA header at 0x100')
    if len(data) & 1:
        data.append(0)
    struct.pack_into('>I', data, 0x1A4, len(data) - 1)
    total = sum(struct.unpack_from(f'>{(len(data) - 0x200) // 2}H', data, 0x200)) & 0xFFFF
    struct.pack_into('>H', data, 0x18E, total)
    open(path, 'wb').write(data)
    print(f'{path}: end {len(data) - 1:06X}, checksum {total:04X}')


if __name__ == '__main__':
    for p in sys.argv[1:]:
        fix(p)
