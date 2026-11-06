#!/usr/bin/env python3
"""Rebuild stages 1-10 room links by executing the original B99A Z80 routine.
Usage: python tools/extract_room_links.py ROM [--write]
Requires z80==1.2.0. One-way links in the ROM are preserved.
"""
import argparse,json,re
from pathlib import Path
from castle_room_objects import make_machine,call
from probe import load_rom
ROOT=Path(__file__).resolve().parents[1]
COUNTS=[3,8,6,6,6,6,6,9,8,9,9]
def extract(data):
    result=[]
    for stage,n in enumerate(COUNTS):
        ptr=int.from_bytes(data[0x1b9d3+stage*2:0x1b9d5+stage*2],'little')
        links=[]
        for room in range(n):
            off=0x1a000+ptr-0xa000+2*room
            raw=data[off:off+2]
            direct=[v if v!=15 else 255 for b in raw for v in (b>>4,b&15)]
            m=make_machine(data);m.set_memory_block(0xa000,data[13*8192:14*8192])
            m.memory[0xd000]=stage;m.memory[0xd001]=room;call(m,0xb99a)
            decoded=list(m.memory[0xc41c:0xc420]);assert decoded==direct
            assert all(v==255 or v<n for v in decoded)
            links.append(decoded)
        result.append(links)
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('rom',type=Path);p.add_argument('--write',action='store_true');a=p.parse_args()
    links=extract(load_rom(a.rom));changes={}
    for stage in range(1,11):
        file=ROOT/('sgdk/src/room_manager.c' if stage==1 else f'sgdk/src/generated/stage{stage}_maps.c')
        s=file.read_text();pattern=rf'(const VKRoomLink vk_stage{stage}_links\[(?:VK_STAGE{stage}_ROOM_COUNT|\d+)\] = \{{)(.*?)(\n\}};)'
        match=re.search(pattern,s,re.S);assert match
        previous=[[int(v.strip()) if v.strip()!='VK_ROOM_NONE' else 255 for v in row.split(',')][:4] for row in re.findall(r'\{([^}]+)\}',match[2])]
        changes[stage]=sum(x!=y for row,old in zip(links[stage],previous) for x,y in zip(row,old))
        if a.write:
            body='\n'+''.join('    {'+','.join('VK_ROOM_NONE' if v==255 else str(v) for v in row)+',0},\n' for row in links[stage]).rstrip('\n')
            file.write_text(s[:match.start(2)]+body+s[match.end(2):])
        else:assert previous==links[stage],(stage,'differs from ROM')
    if a.write:
        (ROOT/'research/stages-1-10/MSX2_ROOM_LINKS.json').write_text(json.dumps({'order':['up','down','left','right'],'none':255,'links':links,'changed_direction_entries':changes},indent=2)+'\n')
    print('ROM B99A and packed table agree; 73 castle rooms; changes:',changes)
if __name__=='__main__':main()
