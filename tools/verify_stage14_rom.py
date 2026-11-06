#!/usr/bin/env python3
"""Re-extract raw stage14 data from verified ROM, compare to stored C and JSON resources."""
from pathlib import Path
import sys,json,re,hashlib
from probe import load_rom
from build_stage14_foundations import extract,ROOT

def verify(path):
    data=load_rom(path)
    g,maps,rooms,ex,flags,pof,ptr,fof,dof,door,gof,group=extract(data)
    m=json.loads((ROOT/'research/stage14-foundations.json').read_text())
    assert m['rom_sha256']==hashlib.sha256(data).hexdigest()
    assert m['links']==g['rooms'] and m['rooms']==json.loads(json.dumps(rooms)) and m['asymmetries']==ex
    assert m['spawn_flags']['bytes']==flags
    assert m['spawn_flags']['pointer_cpu']==f'0x{ptr:04X}'
    assert m['door']['raw_hex']==bytes(door).hex() and m['door']['offset']==f'0x{dof:05X}'
    assert m['stage_group']['value']==group
    src=(ROOT/'sgdk/src/generated/stage14_foundations.c').read_text()
    # Parse the generated initializer, compare all 8x768 tile bytes, not merely hashes in JSON.
    region=src.split('vk_stage14_tiles[VK_STAGE14_ROOM_COUNT][768] = {',1)[1].split('};',1)[0]
    chunks=re.findall(r'\{([^{}]*)\}',region)
    assert len(chunks)==8
    for i,chunk in enumerate(chunks):
        actual=[int(x) for x in re.findall(r'\d+',chunk)]
        assert actual==maps[i], ('map C differs',i)
    h=(ROOT/'sgdk/src/generated/stage14_foundations.h').read_text()
    for name,val in [('ROOM',door[0]&15),('X',door[2]),('Y',door[1]),('RAW_FLAGS',door[0])]:
        assert f'#define VK_STAGE14_DOOR_{name} {val}' in h
    print('stage14 ROM verification: PASS (8 maps, links, flags, door; STATIC only)')

if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_stage14_rom.py path/to/reference.rom')
    verify(Path(sys.argv[1]))
