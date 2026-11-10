#!/usr/bin/env python3
"""Bounded capacity probe: full Z80 object/channel loops, 2400 images per late room."""
import sys,json
from pathlib import Path
sys.path.insert(0,'tools')
from probe import load_rom,reference_rom,ROOT
from castle_room_objects import make_machine,call
from extract_rooms import decode_room
r=load_rom(reference_rom());overall=[]
for stage in range(12,19):
 count=json.load(open(f'research/stage{stage}-foundations.json'))['room_count']
 for room in range(count):
  m=make_machine(r);mem=m.memory;mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=r[0x1E6A+stage];call(m,0x6154)
  m.set_memory_block(0xD100,bytes(decode_room(r,stage,room)[2]));mem[0xC427]=110;mem[0xC425]=160
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  call(m,0x9CA9)
  base=0xDB00+(stage-1-3*mem[0xD002])*256
  for slot in range(4):
   raw=bytes(mem[base+16*room+4*slot:base+16*room+4*slot+4])
   if raw[0]:m.c=raw[0]&127;m.d=(raw[1]&15)*16;m.e=raw[1]&240;call(m,0x5F1D)
  peak=(0,0,0)
  for p in range(1200):
   mem[0xC003]=p&255;mem[0xC427]=32+(p*3)%192
   call(m,0x98E5)
   e=sum(bool(mem[0xC800+128*k]) for k in range(7));q=sum(bool(mem[0xD700+128*k]) for k in range(8))
   if e+q>sum(peak[:2]):peak=(e,q,p)
   call(m,0x9E31)
   e=sum(bool(mem[0xC800+128*k]) for k in range(7));q=sum(bool(mem[0xD700+128*k]) for k in range(8))
   if e+q>sum(peak[:2]):peak=(e,q,p)
  overall.append((stage,room,*peak));print(stage,room,peak,flush=True)
(ROOT/'research/late-pools-trace.json').write_text(json.dumps({'images_per_room':2400,'samples':overall,'scope':'No contact or deaths; Simon triangle X=32..223, Y=160'},indent=2)+'\n')
assert len(overall)==74
assert max(e+q for stage,room,e,q,p in overall)==7
print('74 real rooms, 177600 video images, peak combined live objects 7: PASS; no capacity excess observed in this scenario')
