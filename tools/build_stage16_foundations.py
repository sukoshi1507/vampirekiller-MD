#!/usr/bin/env python3
"""Extract stage 16 raw static resources only from SHA256-verified MSX2 ROM.

Does not execute Z80; does not generate collisions, objects or enemy definitions.
"""
from pathlib import Path
import sys,json,hashlib
from probe import load_rom
from extract_rooms import decode_room
from decode_room_links import decode,check
ROOT=Path(__file__).resolve().parents[1]
STAGE=16
COUNT=10

def extract(data):
    g=decode(data)[STAGE]
    assert len(g['rooms'])==COUNT
    exceptions=check([g])
    assert exceptions==['s16 r1 DOWN->7 but r7 UP->None', 's16 r2 DOWN->8 but r8 UP->None', 's16 r3 DOWN->9 but r9 UP->None'],exceptions
    maps=[]; meta=[]
    for r in range(COUNT):
        m, gfx, tiles=decode_room(data,STAGE,r)
        assert m['status']=='static_background_decoded' and len(tiles)==768
        maps.append(tiles)
        meta.append({'room':r,'map_cpu':m['map_cpu'],'map_file_offset':m['map_file_offset'],
            'map_sha256':hashlib.sha256(bytes(tiles)).hexdigest(),
            'graphics_sha256':hashlib.sha256(gfx).hexdigest(),'palette_rgb333':m['palette_rgb333']})
    flag_ptr_off=0x1C5A6+STAGE*2
    flag_ptr=int.from_bytes(data[flag_ptr_off:flag_ptr_off+2],'little')
    assert 0x8000<=flag_ptr<0xA000
    flag_off=0x1C000+flag_ptr-0x8000
    flags=list(data[flag_off:flag_off+COUNT])
    door_off=0x1BB61+3*STAGE
    raw=list(data[door_off:door_off+3]); assert len(raw)==3
    assert raw[0]&0x80 and (raw[0]&15)<COUNT
    group_off=0x1E6A+STAGE
    return g,maps,meta,exceptions,flags,flag_ptr_off,flag_ptr,flag_off,door_off,raw,group_off,data[group_off]

def generate(data):
    g,maps,meta,ex,flags,pof,ptr,fof,dof,door,gof,group=extract(data)
    out=ROOT/'sgdk/src/generated';out.mkdir(parents=True,exist_ok=True)
    h='''/* Stage 16 raw ROM tables only. Not a playable gameplay stage. */
#ifndef VK_STAGE16_FOUNDATIONS_H
#define VK_STAGE16_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE16_ROOM_COUNT 10
extern const VKByte vk_stage16_tiles[VK_STAGE16_ROOM_COUNT][768];
extern const VKRoomLink vk_stage16_raw_links[VK_STAGE16_ROOM_COUNT];
extern const VKByte vk_stage16_raw_spawn_flags[VK_STAGE16_ROOM_COUNT];
#define VK_STAGE16_DOOR_RAW_FLAGS %d
#define VK_STAGE16_DOOR_ROOM %d
#define VK_STAGE16_DOOR_X %d
#define VK_STAGE16_DOOR_Y %d
#endif
'''%(door[0],door[0]&15,door[2],door[1])
    (out/'stage16_foundations.h').write_text(h)
    c=['/* MSX2 ROM static extraction only: NOT collision/objects/enemies. */', '#include "stage16_foundations.h"',
       'const VKByte vk_stage16_tiles[VK_STAGE16_ROOM_COUNT][768] = {']
    for tiles in maps:
        c+=['    {']+['        '+','.join(str(n) for n in tiles[i:i+32])+',' for i in range(0,768,32)]+['    },']
    c+=['};','const VKRoomLink vk_stage16_raw_links[VK_STAGE16_ROOM_COUNT] = {']
    for row in g['rooms']:
        c.append('    {'+','.join('VK_ROOM_NONE' if row[d] is None else str(row[d]) for d in ('UP','DOWN','LEFT','RIGHT'))+',0},')
    c+=['};','const VKByte vk_stage16_raw_spawn_flags[VK_STAGE16_ROOM_COUNT] = {'+','.join(str(n) for n in flags)+'};']
    (out/'stage16_foundations.c').write_text('\n'.join(c)+'\n')
    report={'stage':STAGE,'status':'ROM_raw_static_NOT_playable','rom_sha256':hashlib.sha256(data).hexdigest(),
       'room_count':COUNT,'navigation_pointer_cpu':f'0x{g["pointer"]:04X}',
       'links':g['rooms'],'asymmetries':ex,'rooms':meta,
       'spawn_flags':{'pointer_table_file_offset':f'0x{pof:05X}','pointer_cpu':f'0x{ptr:04X}',
           'data_file_offset':f'0x{fof:05X}','bytes':flags,'note':'Channel flags do not specify enemy type or count'},
       'door':{'offset':f'0x{dof:05X}','raw_hex':bytes(door).hex(), 'flags':door[0],
           'room':door[0]&15,'y':door[1],'x':door[2], 'scope':'door table entry only, not key/opening rules'},
       'stage_group':{'offset':f'0x{gof:05X}','value':group},
       'limitations':['Z80 4F83 collision maps not executed','Z80 5A49/5B1B objects and key not extracted',
                      'Z80 6154 fixed enemy list not executed','Enemy behavior and audio not ported',
                      'No SGDK gameplay integration','No compiled Mega Drive runtime verification']}
    (ROOT/'research/stage16-foundations.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    return report

if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: build_stage16_foundations.py path/to/reference.rom')
    report=generate(load_rom(Path(sys.argv[1])))
    print('Stage 16:',report['room_count'],'static maps; flags',report['spawn_flags']['bytes'], 'door',report['door'])
