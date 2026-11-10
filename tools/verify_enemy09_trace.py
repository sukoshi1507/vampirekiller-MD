#!/usr/bin/env python3
"""AF4A/AF5F real-room Z80 versus C, including dormant SAT layers."""
import ctypes as C
import json,sys
from pathlib import Path
from probe import load_rom,reference_rom,ROOT
from castle_room_objects import make_machine,call
from extract_rooms import decode_room

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()))
 lib.nine_init.argtypes=[C.POINTER(C.c_ubyte)]+[C.c_int]*6
 rom=load_rom(reference_rom());cases=[]
 for stage in (13,14):
  table=make_machine(rom);table.memory[0xD000]=stage;table.memory[0xD002]=rom[0x1E6A+stage];call(table,0x6154)
  base=0xDB00+(stage-1-3*table.memory[0xD002])*256
  count=json.loads((ROOT/'research'/f'stage{stage}-foundations.json').read_text())['room_count']
  for room in range(count):
   for slot in range(4):
    raw=bytes(table.memory[base+room*16+slot*4:base+room*16+slot*4+4])
    if raw[0]&127!=9:continue
    x,y=(raw[1]&15)*16,raw[1]&240
    for label,sx,diff in [('left',max(8,x-70),0),('right',min(240,x+70),0),('near',min(240,x+20),0),('hard',max(8,x-20),2)]:
     m=make_machine(rom);mem=m.memory;mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage];call(m,0x6154)
     tilemap=bytes(decode_room(rom,stage,room)[2]);m.set_memory_block(0xD100,tilemap)
     mem[0xD012]=diff;mem[0xC427]=sx;mem[0xC425]=160
     for a in range(0xD600,0xD700,4):mem[a]=0xE0
     m.c=9;m.d=x;m.e=y;call(m,0x5F1D)
     cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.nine_init(cmap,stage,room,x,y,sx,diff)
     trace=[]
     for frame in range(400):
      if frame%2==0:
       mem[0xC003]=(frame//2)&255
       if mem[0xC800]:
        m.ix=0xC800;call(m,0xAF5F);call(m,0x99B9);call(m,0x99E5);call(m,0x6445)
       lib.nine_step(sx)
      b=(C.c_ubyte*32)();sat=(C.c_int*24)();n=lib.nine_record(b,sat);expected=bytes(mem[0xC800:0xC820])
      assert bytes(b)==expected,(stage,room,slot,label,frame,[(hex(i),a,c) for i,(a,c) in enumerate(zip(expected,b)) if a!=c])
      visible=[]
      if expected[0] and frame%2==0:
       for k in range(mem[0xC820]):
        a=0xC822+5*k
        if mem[a+3]&15:visible.append(list(mem[a:a+3]))
       assert n==len(visible),(stage,room,label,frame,n,visible)
       assert [list(sat[3*k:3*k+3]) for k in range(n)]==visible,(stage,room,label,frame,list(sat)[:n*3],visible)
      assert (mem[0xC427],mem[0xC425])==(sx,160)
      assert not any(mem[0xD700+k*128] for k in range(8))
      trace.append({'frame':frame,'raw':expected.hex(),'sprites':visible,'simon':[sx,160],'projectiles':[]})
     cases.append({'stage':stage,'room':room,'slot':slot,'case':label,'frames':400,'trace':trace})
 print('09h:',len(cases),'real-room scenarios,',sum(c['frames'] for c in cases),'frames: PASS')
 (ROOT/'research/enemy09-trace.json').write_text(json.dumps({'all_equal':True,'cases':cases},indent=1)+'\n')
if __name__=='__main__':main()
