#!/usr/bin/env python3
"""Re-extract stage 13 from hashed MSX2 ROM and check all stored resources.

Usage: python tools/verify_stage13_rom.py /path/to/reference.rom
"""
import json
import hashlib
import sys
from pathlib import Path
from probe import load_rom
from build_stage13_foundations import extract,ROOT

def verify(path):
    data=load_rom(path)
    graph,maps,rooms,flags,ptr,ptr_off,flag_off,group,group_off=extract(data)
    manifest=json.loads((ROOT/'research/stage13-foundations.json').read_text())
    assert manifest['rom_sha256']==hashlib.sha256(data).hexdigest()
    assert manifest['links']==graph['rooms']
    assert manifest['room_navigation_pointer_cpu']==f'0x{graph["pointer"]:04X}'
    assert manifest['rooms']==json.loads(json.dumps(rooms))
    assert manifest['spawn_channel_flags']['bytes']==flags
    assert manifest['spawn_channel_flags']['pointer_cpu']==f'0x{ptr:04X}'
    assert manifest['spawn_channel_flags']['pointer_table_file_offset']==f'0x{ptr_off:05X}'
    assert manifest['spawn_channel_flags']['data_file_offset']==f'0x{flag_off:05X}'
    assert manifest['stage_group']=={'file_offset':f'0x{group_off:05X}','value':group}
    assert len(maps)==12 and all(len(x)==768 for x in maps)
    print('stage13 ROM manifest: PASS, 12 maps, 12 links, 12 channel flags')

if __name__=='__main__':
    if len(sys.argv)!=2: raise SystemExit(__doc__)
    verify(Path(sys.argv[1]))
