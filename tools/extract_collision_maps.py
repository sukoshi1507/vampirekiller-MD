#!/usr/bin/env python3
"""Compare/regenerate full 768-byte maps using original Z80 4F83 (D100:D400).
Usage: extract_collision_maps.py ROM [--write]. Requires z80==1.2.0.
"""
import argparse,json,re,hashlib
from pathlib import Path
from castle_room_objects import make_machine,call
from probe import load_rom
from extract_room_links import COUNTS
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('rom',type=Path);p.add_argument('--write',action='store_true');a=p.parse_args()
data=load_rom(a.rom);report=[]
limits=(ROOT/'sgdk/src/generated/player_assets.c').read_text()
body=re.search(r'vk_collision_limits\[19\] = \{(.*?)\};',limits,re.S).group(1)
values=[int(v,0) for v in re.findall(r'0x[0-9a-fA-F]+|\b\d+\b',body)]
assert values==list(data[0x3c78:0x3c78+19]), 'Solid-tile limits differ from ROM'
print('19 solid-tile limits match ROM 7C78')
for stage in range(1,12):
    path=ROOT/'sgdk/src/generated'/('stage1_collision.c' if stage==1 else f'stage{stage}_maps.c')
    text=path.read_text();match=re.search(r'(const VKByte vk_stage\d+_collision\[[^;]+?= \{)(.*?)(\n\};)',text,re.S);assert match
    old=[[int(v) for v in re.findall(r'\b\d+\b',row)] for row in re.findall(r'\{([^{}]+)\}',match[2])]
    maps=[]
    for room in range(COUNTS[stage]):
        m=make_machine(data);m.memory[0xd000]=stage;m.memory[0xd001]=room;call(m,0x4f83)
        cells=bytes(m.memory[0xd100:0xd400]);maps.append(cells)
        diff=[i for i,(x,y) in enumerate(zip(cells,old[room])) if x!=y]
        assert len(old[room])==768
        report.append(dict(stage=stage,room=room,changed_cells=len(diff),visible_changed=sum(i>=64 for i in diff),sha256=hashlib.sha256(cells).hexdigest()))
        if not a.write:assert not diff,(stage,room,diff)
    if a.write:
        body='\n'+''.join('    {\n'+''.join('        '+','.join(map(str,cells[i:i+32]))+',\n' for i in range(0,768,32))+'    },\n' for cells in maps)
        path.write_text(text[:match.start(2)]+body.rstrip('\n')+text[match.end(2):])
if a.write:(ROOT/'research/stages-1-10/MSX2_COLLISION_MAPS.json').write_text(json.dumps(report,indent=2)+'\n')
print('73 full maps / 56064 cells compared with original 4F83; changed cells:',sum(r['changed_cells'] for r in report))
