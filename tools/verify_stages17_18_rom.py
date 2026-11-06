#!/usr/bin/env python3
"""Verify ALL raw C maps/links/flags/doors vs original SHA256-authenticated ROM."""
import sys, re, json, hashlib
from pathlib import Path
from probe import load_rom
from build_stages17_18_foundations import ROOT,COUNTS,extract

def verify(rom_path):
    data=load_rom(rom_path)
    for stage,count in COUNTS.items():
        x=extract(data,stage)
        rep=json.loads((ROOT/f'research/stage{stage}-foundations.json').read_text())
        assert rep['rom_sha256']==hashlib.sha256(data).hexdigest()
        assert rep['links']==x['links'] and rep['asymmetries']==x['asymmetries']
        assert rep['spawn_flags']['bytes']==x['flags']
        assert rep['door']['raw_hex']==bytes(x['door']).hex()
        assert rep['stage_group']['value']==x['group']
        for i,room in enumerate(rep['rooms']):
            assert room['map_sha256']==hashlib.sha256(bytes(x['maps'][i])).hexdigest()
            assert room['unresolved_tile_ids']==x['rooms'][i]['unresolved_tile_ids']
            assert room['graphics_sha256']==x['rooms'][i]['graphics_sha256']
        src=(ROOT/f'sgdk/src/generated/stage{stage}_foundations.c').read_text()
        prefix=f'vk_stage{stage}'
        tiles=src.split(f'{prefix}_tiles[VK_STAGE{stage}_ROOM_COUNT][768] = {{',1)[1].split('};',1)[0]
        tile_rows=re.findall(r'\{([^{}]*)\}',tiles)
        assert len(tile_rows)==count
        for i,row in enumerate(tile_rows):
            assert [int(n) for n in re.findall(r'\d+',row)]==x['maps'][i],('tiles differ',stage,i)
        links=src.split(f'{prefix}_raw_links[VK_STAGE{stage}_ROOM_COUNT] = {{',1)[1].split('};',1)[0]
        link_rows=re.findall(r'\{([^{}]*)\}',links)
        assert len(link_rows)==count
        for i,row in enumerate(link_rows):
            actual=[v.strip() for v in row.split(',')]
            expected=['VK_ROOM_NONE' if x['links'][i][d] is None else str(x['links'][i][d]) for d in ('UP','DOWN','LEFT','RIGHT')]+['0']
            assert actual==expected,('links differ',stage,i)
        flags=src.split(f'{prefix}_raw_spawn_flags[VK_STAGE{stage}_ROOM_COUNT] = {{',1)[1].split('};',1)[0]
        assert [int(v) for v in re.findall(r'\d+',flags)]==x['flags']
        header=(ROOT/f'sgdk/src/generated/stage{stage}_foundations.h').read_text()
        for name,value in [('ROOM',x['door'][0]&15),('X',x['door'][2]),('Y',x['door'][1]),('RAW_FLAGS',x['door'][0])]:
            assert f'#define VK_STAGE{stage}_DOOR_{name} {value}' in header
        print(f'Stage {stage}: PASS: {count*768} tile indices, {count} links, {count} flags, door; static only')
    print('WARNING: these tests do not validate gameplay, enemies, objects, boss, ending or stage 18 room 9 graphics.')

if __name__=='__main__':
    if len(sys.argv)!=2: raise SystemExit('usage: verify_stages17_18_rom.py /path/to/reference.rom')
    verify(Path(sys.argv[1]))
