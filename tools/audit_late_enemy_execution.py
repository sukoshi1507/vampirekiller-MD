#!/usr/bin/env python3
"""Read-only evidence audit for late fixed-enemy Z80 handlers.

The mapping is the same bank-2, 0x8000 CPU window as the verified 9942
update table. This script does NOT claim to emulate any handler.
Usage: python tools/audit_late_enemy_execution.py ROM.rom [OUTPUT.json]
"""
import hashlib
import json
import sys
from pathlib import Path

ROUTINES = {0x09: (0xAF5F, 0xB090), 0x10: (0xADF8, 0xAF5F), 0x12: (0xBE70, 0xBF4A)}
# ADF8 and AF5F share a contiguous region; boundaries are known entry addresses,
# not a claim that all bytes in the ranges belong exclusively to those routines.
DISPATCH_TYPES = [0x09, 0x0B, 0x0D, 0x0E, 0x10, 0x12]
DISPATCH_ADDRS = [0xAF5F, 0xB37E, 0xB248, 0xA9B7, 0xADF8, 0xBE70]
EXPECTED_ROM_SHA = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'

def cpu_offset(address):
    if not 0x8000 <= address < 0xC000:
        raise ValueError('address outside bank-2 CPU window')
    return 0x4000 + (address - 0x8000)

def audit(rom):
    digest = hashlib.sha256(rom).hexdigest()
    if digest != EXPECTED_ROM_SHA:
        raise ValueError('ROM SHA-256 mismatch; no evidence can be certified: ' + digest)
    table = rom[cpu_offset(0x9942):cpu_offset(0x9942) + 2 * (max(DISPATCH_TYPES) + 1)]
    for typ, addr in zip(DISPATCH_TYPES, DISPATCH_ADDRS):
        actual = table[2 * (typ - 1)] | table[2 * (typ - 1) + 1] << 8
        if actual != addr:
            raise ValueError(f'dispatch mismatch type {typ:02X}: {actual:04X} != {addr:04X}')
    entries = []
    for typ, (start, end) in ROUTINES.items():
        chunk = rom[cpu_offset(start):cpu_offset(end)]
        entries.append({'type': f'{typ:02X}', 'cpu_entry': f'{start:04X}',
                        'cpu_range_end_exclusive': f'{end:04X}',
                        'bytes_in_audit_window': len(chunk),
                        'sha256': hashlib.sha256(chunk).hexdigest(),
                        'entry_bytes': chunk[:16].hex(' '),
                        'implemented_in_mega_drive': False})
    return {'original_rom_sha256': digest, 'dispatch_cpu_address': '9942',
            'bank_file_offset': '4000', 'routines': entries,
            'limitations': ['Static hash audit only, not CPU execution',
                            'Ranges are windows for auditing, not proven function extents',
                            'BE70 is shared with an earlier boss; stage-16 creation and initialization unverified',
                            'AF5F and ADF8 reference terrain/collision and internal object fields']}

def main():
    if len(sys.argv) not in (2, 3):
        raise SystemExit(__doc__)
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from probe import load_rom
    obj = audit(load_rom(Path(sys.argv[1])))
    data = json.dumps(obj, indent=2, ensure_ascii=False) + '\n'
    if len(sys.argv) == 3:
        Path(sys.argv[2]).write_text(data)
    else:
        print(data, end='')

if __name__ == '__main__':
    main()
