#!/usr/bin/env python3
"""Extract exits, keys and entry poses for MD stages 1-10 from the supplied ROM.
Requires z80==1.2.0. No ROM is distributed with the project.
"""
import json
from pathlib import Path
from probe import load_rom
from castle_room_objects import decode_room, make_machine, call

ROOMS = [3,9,6,6,6,6,6,9,8,9,9]
def extract(data):
    result=[]
    for stage in range(1,11):
        keys=[]
        for room in range(ROOMS[stage]):
            decoded=decode_room(data,stage,room)
            for e in decoded['candles']+decoded['other_objects']:
                b=bytes.fromhex(e['raw'])
                hidden=b[0]==1 and b[4]==3 and b[5]==0x18
                visible=b[0]==0x83 and b[4]==0x18
                if hidden or visible:
                    keys.append(dict(room=room,x=b[2],y=b[1],hidden=int(hidden),raw=e['raw']))
        assert len(keys)==1,(stage,keys)
        off=0x1bb61+3*stage
        flags,y,x=data[off:off+3]
        assert flags&0x80 and (flags&15)<ROOMS[stage] and x in (12,236)
        # Execute the original door loader, not just its table interpretation.
        m=make_machine(data)
        m.set_memory_block(0xa000,data[13*8192:14*8192])
        m.memory[0xd000]=stage; m.memory[0xd001]=flags&15
        call(m,0xbb37)
        assert bytes(m.memory[0xc5ac:0xc5af])==bytes((255,y,x))
        m=make_machine(data);m.memory[0xd000]=stage;call(m,0x6402)
        entry=dict(x=m.memory[0xc427],y=m.memory[0xc425],facing=m.memory[0xc42c])
        result.append(dict(stage=stage,key=keys[0],door=dict(room=flags&15,x=x,y=y,raw=data[off:off+3].hex(),offset=hex(off)),entry=entry))
    return result

def generate(data,root):
    rows=extract(data)
    h='/* Generated from the reference ROM by extract_stage_exits.py. */\n#ifndef VK_STAGE_EXITS_H\n#define VK_STAGE_EXITS_H\n#include "../player.h"\n'
    h+='typedef struct { VKByte key_room,key_x,key_y,key_hidden,door_room,door_x,door_y; } VKStageItems;\n'
    h+='static const VKStageItems kStageItems[10] = {\n'
    for r in rows:
        k,d=r['key'],r['door'];h+='    { '+', '.join(map(str,[k['room'],k['x'],k['y'],k['hidden'],d['room'],d['x'],d['y']]))+' }, /* stage '+str(r['stage'])+' */\n'
    h+='};\ntypedef struct { VKByte x,y,facing; } VKStageEntry;\nstatic const VKStageEntry kStageEntries[10] = {\n'
    for r in rows:h+='    { '+', '.join(map(str,r['entry'].values()))+' },\n'
    h+='};\n#endif\n'
    (root/'sgdk/src/generated/stage_exits.h').write_text(h)
    (root/'research/stages-1-10/MSX2_STAGE_EXITS.json').write_text(json.dumps(rows,indent=2)+'\n')
    return rows
if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('rom',type=Path);a=p.parse_args()
    for r in generate(load_rom(a.rom),Path(__file__).resolve().parents[1]):print(r)
