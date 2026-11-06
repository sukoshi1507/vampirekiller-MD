#!/usr/bin/env python3
"""Generate only ROM-proven stage 17/18 static maps, navigation, spawn flags, door bytes.

This does NOT execute Z80 routines or create gameplay/objects/boss behaviors.
"""
from pathlib import Path
import sys, json, hashlib
from probe import load_rom
from extract_rooms import decode_room, render
from decode_room_links import decode,check

ROOT=Path(__file__).resolve().parents[1]
COUNTS={17:12,18:10}

def extract(data,s):
    assert s in COUNTS
    g=decode(data)[s]
    count=COUNTS[s]
    assert len(g['rooms'])==count
    problems=check([g])
    maps=[]; meta=[]; images=[]
    for r in range(count):
        manifest, gfx, tiles=decode_room(data,s,r)
        assert len(tiles)==768
        maps.append(tiles)
        meta.append({'room':r,'map_cpu':manifest['map_cpu'],'map_file_offset':manifest['map_file_offset'],
            'map_sha256':hashlib.sha256(bytes(tiles)).hexdigest(),
            'graphics_sha256':hashlib.sha256(gfx).hexdigest(),
            'palette_rgb333':manifest['palette_rgb333'],
            'unresolved_tile_ids':manifest['unresolved_tile_ids'],
            'status':manifest['status']})
        images.append((gfx,tiles,manifest['palette_rgb333']))
    flag_ptr_offset=0x1C5A6+2*s
    flag_ptr=int.from_bytes(data[flag_ptr_offset:flag_ptr_offset+2],'little')
    assert 0x8000<=flag_ptr<0xA000
    flag_offset=0x1C000+flag_ptr-0x8000
    flags=list(data[flag_offset:flag_offset+count])
    door_offset=0x1BB61+3*s
    door=list(data[door_offset:door_offset+3])
    assert door[0]&0x80 and door[0]&15<count
    group_offset=0x1E6A+s
    return {'stage':s,'count':count,'links':g['rooms'],'navigation_pointer':g['pointer'],
            'asymmetries':problems,'maps':maps,'rooms':meta,'images':images,
            'flag_ptr_offset':flag_ptr_offset,'flag_ptr':flag_ptr,'flag_offset':flag_offset,'flags':flags,
            'door_offset':door_offset,'door':door,'group_offset':group_offset,'group':data[group_offset]}

def generate(data):
    out=ROOT/'sgdk/src/generated';out.mkdir(parents=True,exist_ok=True)
    previews=ROOT/'research/stages17-18-previews';previews.mkdir(parents=True,exist_ok=True)
    all_reports=[]
    for s in COUNTS:
        x=extract(data,s)
        count=x['count']; door=x['door']
        macro=f'VK_STAGE{s}'
        h=f'''/* ROM MSX2 raw stage {s} static data. NOT a playable stage. */
#ifndef {macro}_FOUNDATIONS_H
#define {macro}_FOUNDATIONS_H
#include "../room_manager.h"
#define {macro}_ROOM_COUNT {count}
extern const VKByte vk_stage{s}_tiles[{macro}_ROOM_COUNT][768];
extern const VKRoomLink vk_stage{s}_raw_links[{macro}_ROOM_COUNT];
extern const VKByte vk_stage{s}_raw_spawn_flags[{macro}_ROOM_COUNT];
#define {macro}_DOOR_RAW_FLAGS {door[0]}
#define {macro}_DOOR_ROOM {door[0]&15}
#define {macro}_DOOR_X {door[2]}
#define {macro}_DOOR_Y {door[1]}
#endif
'''
        (out/f'stage{s}_foundations.h').write_text(h)
        c=[f'/* Exact ROM stage {s} raw tile indices; NO gameplay semantics. */',f'#include "stage{s}_foundations.h"',
           f'const VKByte vk_stage{s}_tiles[{macro}_ROOM_COUNT][768] = {{']
        for tiles in x['maps']:
            c+=['    {']+['        '+','.join(str(v) for v in tiles[i:i+32])+',' for i in range(0,768,32)]+['    },']
        c+=['};',f'const VKRoomLink vk_stage{s}_raw_links[{macro}_ROOM_COUNT] = {{']
        for row in x['links']:
            c.append('    {'+','.join('VK_ROOM_NONE' if row[d] is None else str(row[d]) for d in ('UP','DOWN','LEFT','RIGHT'))+',0},')
        c+=['};',f'const VKByte vk_stage{s}_raw_spawn_flags[{macro}_ROOM_COUNT] = {{'+','.join(str(n) for n in x['flags'])+'};']
        (out/f'stage{s}_foundations.c').write_text('\n'.join(c)+'\n')
        # Generate preview only as illustrative static background; not emulator validation.
        for r,(gfx,tiles,pal) in enumerate(x['images']):
            img=render(gfx,tiles,pal)
            img.save(previews/f'stage{s}_room{r:02d}.png')
        rep={'stage':s,'status':'RAW_STATIC_ONLY_NOT_PLAYABLE',
             'rom_sha256':hashlib.sha256(data).hexdigest(),'room_count':count,
             'navigation_pointer_cpu':f'0x{x["navigation_pointer"]:04X}',
             'links':x['links'],'asymmetries':x['asymmetries'],'rooms':x['rooms'],
             'spawn_flags':{'pointer_table_file_offset':f'0x{x["flag_ptr_offset"]:05X}',
                'pointer_cpu':f'0x{x["flag_ptr"]:04X}','data_file_offset':f'0x{x["flag_offset"]:05X}',
                'bytes':x['flags'],'note':'Raw flags cannot identify enemy type or count'},
             'door':{'offset':f'0x{x["door_offset"]:05X}','raw_hex':bytes(door).hex(),
                'flags':door[0],'room':door[0]&15,'y':door[1],'x':door[2],
                'scope':'ROM door-table entry only; activation/key/final-ending not established'},
             'stage_group':{'offset':f'0x{x["group_offset"]:05X}','value':x['group']},
             'limitations':['Z80 collision generation 4F83 not executed',
                'Z80 objects and keys 5A49/5B1B not executed','Z80 fixed enemies 6154 not executed',
                'No enemy behaviors, attacks, bosses or ending implemented',
                'No gameplay engine integration or SGDK runtime tested',
                'Stage 18 room 9 has tiles >=192 unsupported by fixed 192-tile graphics decoder']}
        (ROOT/f'research/stage{s}-foundations.json').write_text(json.dumps(rep,ensure_ascii=False,indent=2)+'\n')
        all_reports.append(rep)
    return all_reports

if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: build_stages17_18_foundations.py /path/to/reference.rom')
    for x in generate(load_rom(Path(sys.argv[1]))):
        print(f'stage {x["stage"]}: {x["room_count"]} maps, flags={x["spawn_flags"]["bytes"]}, door={x["door"]["raw_hex"]}, unresolved rooms={[r["room"] for r in x["rooms"] if r["unresolved_tile_ids"]]}')
