#!/usr/bin/env python3
"""Extract exits, keys and entry poses for MD stages 1-18 from the supplied ROM.
Requires z80==1.2.0. No ROM is distributed with the project.
"""
import json
from pathlib import Path
from probe import load_rom,reference_rom
from decode_room_links import decode
from castle_room_objects import decode_room, make_machine, call

CHECK=False
ROOMS = [3,9,6,6,6,6,6,9,8,9,9,6]
def extract(data):
    result=[]
    for stage in range(1,19):
        keys=[]
        for room in range(len(decode(data)[stage]['rooms'])):
            decoded=decode_room(data,stage,room)
            for e in decoded['candles']+decoded['other_objects']:
                b=bytes.fromhex(e['raw'])
                hidden=b[0]==1 and b[4]==3 and b[5]==0x18
                visible=b[0]==0x83 and b[4]==0x18
                if hidden or visible:
                    keys.append(dict(room=room,x=b[2],y=b[1],hidden=int(hidden),raw=e['raw']))
        assert keys and (stage>=12 or len(keys)==1),(stage,keys)
        off=0x1bb61+3*stage
        flags,y,x=data[off:off+3]
        assert flags&0x80 and (flags&15)<len(decode(data)[stage]['rooms']) and x in (12,236)
        # Execute the original door loader, not just its table interpretation.
        m=make_machine(data)
        m.set_memory_block(0xa000,data[13*8192:14*8192])
        m.memory[0xd000]=stage; m.memory[0xd001]=flags&15
        call(m,0xbb37)
        assert bytes(m.memory[0xc5ac:0xc5af])==bytes((255,y,x))
        m=make_machine(data);m.memory[0xd000]=stage;call(m,0x6402)
        entry=dict(x=m.memory[0xc427],y=m.memory[0xc425],facing=m.memory[0xc42c])
        result.append(dict(stage=stage,key=keys[0],keys=keys,door=dict(room=flags&15,x=x,y=y,raw=data[off:off+3].hex(),offset=hex(off)),entry=entry))
    return result

def generate(data,root):
    rows=extract(data)
    h='/* Generated from the reference ROM by extract_stage_exits.py. */\n#ifndef VK_STAGE_EXITS_H\n#define VK_STAGE_EXITS_H\n#include "../player.h"\n'
    h+='typedef struct { VKByte key_room,key_x,key_y,key_hidden,door_room,door_x,door_y; } VKStageItems;\n'
    h+='static const VKStageItems kStageItems[18] = {\n'
    for r in rows:
        k,d=r['key'],r['door'];h+='    { '+', '.join(map(str,[k['room'],k['x'],k['y'],k['hidden'],d['room'],d['x'],d['y']]))+' }, /* stage '+str(r['stage'])+' */\n'
    h+='};\ntypedef struct { VKByte x,y,facing; } VKStageEntry;\nstatic const VKStageEntry kStageEntries[18] = {\n'
    for r in rows:h+='    { '+', '.join(map(str,r['entry'].values()))+' },\n'
    h+='};\nstatic const VKStageItems kLateRoomItems[7][12] = {\n'
    for r in rows[11:]:
        h+='{\n'
        for room in range(12):
            ks=[k for k in r['keys'] if k['room']==room];assert len(ks)<=1
            k=ks[0] if ks else dict(room=255,x=0,y=0,hidden=0);d=r['door']
            h+='{'+','.join(map(str,[k['room'],k['x'],k['y'],k['hidden'],d['room'],d['x'],d['y']]))+'},\n'
        h+='},\n'
    h+='};\n#endif\n'
    p=root/'sgdk/src/generated/stage_exits.h'
    if CHECK:assert p.read_text()==h
    else:p.write_text(h)
    # Keep historical evidence files unchanged; new runtime keys are checked against execution.

    return rows
if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('rom',nargs='?',type=Path,default=reference_rom());p.add_argument('--check',action='store_true');a=p.parse_args();CHECK=a.check
    for r in generate(load_rom(a.rom),Path(__file__).resolve().parents[1]):print(r)
