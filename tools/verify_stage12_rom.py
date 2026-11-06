#!/usr/bin/env python3
"""Strict read-only stage-12 verification from the owner-supplied MSX2 ROM.
Usage: python tools/verify_stage12_rom.py /path/to/reference.rom
No gameplay/Z80 claims: static tiles, navigation, secret passages and raw flags.
"""
import hashlib
import json
import sys
from pathlib import Path
from build_stage12_foundations import extract, ROOT
from probe import load_rom

def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    rom = load_rom(Path(sys.argv[1]))
    graph, maps, rooms, passages, flags, ptr, off, d002 = extract(rom)
    inventory = json.loads((ROOT/'research/stage12-foundations.json').read_text())
    assert inventory['rom_sha256'] == hashlib.sha256(rom).hexdigest()
    assert inventory['links'] == graph['rooms']
    assert inventory['secret_passages'] == passages
    assert inventory['spawn_flags_rom']['room_flags'] == flags
    assert inventory['spawn_flags_rom']['pointer_cpu'] == f'0x{ptr:04X}'
    assert inventory['spawn_flags_rom']['file_offset'] == f'0x{off:05X}'
    assert inventory['stage_group_d002']['value'] == d002
    assert all(a['map_sha256'] == b['map_sha256'] for a,b in zip(inventory['rooms'],rooms))
    for i, cells in enumerate(maps):
        assert len(cells)==768 and max(cells)<192
        assert inventory['rooms'][i]['map_sha256'] == hashlib.sha256(bytes(cells)).hexdigest()
    print('PASS: reference ROM hash, 12 tilemaps, 12 links, 10 passages, 12 raw spawn flags, D002')
    print('NOT TESTED: Z80 4F83 collision, 5A49/5B1B objects, 6154 fixed enemies, boss or gameplay')
if __name__ == '__main__':
    main()
