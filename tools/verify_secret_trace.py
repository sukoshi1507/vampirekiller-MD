#!/usr/bin/env python3
"""ExecutedBB9A/85F4/6DA9/70FB/75C0/B98C vs production C; no patched ROM."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_trade_trace import TradeHarness
from build_secret_data import collect

def bank(h,n):h.mapper_write(0xA000,n);h.cpu.memory[0xF0F3]=n

def setup(rom,stage,room,x,y,state=2,tick=0,invuln=0,platform=17,left=0,graphics=False):
 h=TradeHarness(rom);mem=h.cpu.memory
 mem[0xF0F1:0xF0F4]=bytes([1,2,3]);mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage]
 if graphics:
  for a in (0x548D,0x53B6,0x564C,0x570D,0x5768,0x5780):h.call(a)
 bank(h,13);h.call(0xBB9A);bank(h,3)
 mem[0xC420:0xC440]=bytes(32);mem[0xC420]=state;mem[0xC421]=19;mem[0xC425]=y;mem[0xC427]=x;mem[0xC42D]=invuln;mem[0xC43D]=platform
 mem[0xC42C]=left;mem[0xC42E]=16 if left else 6;mem[0xC42F]=15 if left else 0
 mem[0xC003]=tick;mem[0xC007]=2
 if graphics:h.call(0x56E1);h.call(0x63C5)
 return h

def values(lib):
 out=(C.c_int*8)();lib.secret_values(out);return list(out)

def compare(lib,h,label,step,tick_offset=0):
 # Existing native C003 advances inside Enemies_frame, while ROM414D advances
 # before Simon. Pre-timer reads therefore expect exactly -1; after the full
 # pass clocks MUST match without offset. No shift of player/timer/sprite data.
 mem=h.cpu.memory;out=(C.c_ubyte*32)();lib.secret_record(out)
 assert bytes(out)==bytes(mem[0xC420:0xC440]),(label,step,'Simon32',bytes(out).hex(),bytes(mem[0xC420:0xC440]).hex())
 v=values(lib);expected=list(mem[0xC5B1:0xC5B5])+[mem[0xC41B],mem[0xD001],mem[0xC003]]
 assert v[:7]==expected[:6]+[(expected[6]+tick_offset)&255],(label,step,'point/route/room/tick',v[:7],expected)
 return v

def sprite_compare(lib,h,left,label,step):
 mem=h.cpu.memory;h.call(0x7837)
 hidden=bool(mem[0xC42D] and mem[0xC003]&2);v=values(lib)
 assert v[7]==hidden,(label,step,'blink',v[7],hidden)
 # Mirror the produced SAT/colour buffers into their VRAM destinations; this
 # adapter does not claim interrupt/VDP transfer timing.
 h.vram[0xF600:0xF680]=bytes(mem[0xD600:0xD680]);h.vram[0xF400:0xF480]=bytes(mem[0xD400:0xD480])
 x0=mem[0xC427]+(-39 if left else -24);y0=mem[0xC425]-25
 expected=bytearray(2048)
 for group in h.sprite_groups():
  for y in range(16):
   for x in range(16):
    gx=group['x']+x-x0;gy=group['y']+y-y0;color=group['pixels'][16*y+x]
    if color:
     assert 0<=gx<64 and 0<=gy<32,(label,step,'sprite escaped canvas',gx,gy)
     expected[gy*64+gx]=color
 pixels=(C.c_ubyte*2048)();lib.secret_image(left,pixels)
 assert bytes(pixels)==expected,(label,step,'sprite pixels',next((i for i,(a,b) in enumerate(zip(pixels,expected)) if a!=b),None))
 return 2048

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());counts,points,raw=collect();checks=0;images=0;pixels=0;reports=[]
 # Full stage/room matrix, including stages13..18 with NO secret points.
 for stage in range(19):
  for room in range(counts[stage]):
   h=setup(rom,stage,room,88,172);lib.secret_init(stage,room,88,172,2,0,0,17);compare(lib,h,'room init',stage*12+room)
   assert list(h.cpu.memory[0xC5B1:0xC5B5])==list(points[stage][room])
 # Execute6382's real object clear between a populated stage12 point and
 # EVERY late room, then the fullBB31 entry (includingBB9A).
 h=setup(rom,12,0,88,172)
 for stage in range(13,19):
  for room in range(counts[stage]):
   h.cpu.memory[0xD000]=stage;h.cpu.memory[0xD001]=room;h.clear_room_objects()
   bank(h,13);h.call(0xBB31);bank(h,3)
   assert bytes(h.cpu.memory[0xC5B1:0xC5B5])==bytes(4)
 print('Secret6382/BB31:62 late rooms clear stale passage data PASS',flush=True)
 # C007 contains held UP but C006 has no new UP edge: stop at the failure
 # continuation6DC8, before unrelated stairs/release-crouch movement executes.
 for stage,room,dest,x,y in raw:
  h=setup(rom,stage,room,x,y+4);mem=h.cpu.memory;mem[0xC007]=32;mem[0xC006]=0
  h.stop=0x6DC8;h.cpu.set_breakpoint(h.stop);h.call(0x6DA9)
  h.cpu.clear_breakpoint(h.stop);h.stop=None
  assert mem[0xC420]==2 and mem[0xC42D]==0 and not h.sounds
 print('Secret6DA9:held-UP without new edge rejected at all10 points PASS',flush=True)
 # Exhaust the two independent8-bit differences for every valid point and absence.
 for stage,room,dest,x,y in raw+[(s,0,0,88,168) for s in range(13,19)]:
  h=setup(rom,stage,room,x,(y+4)&255);lib.secret_init(stage,room,x,(y+4)&255,2,0,0,17)
  for axis in (0,1):
   for coord in range(256):
    sx=coord if axis==0 else x;sy=coord if axis==1 else (y+4)&255
    h.cpu.memory[0xC427]=sx;h.cpu.memory[0xC425]=sy;h.call(0x85F4)
    assert lib.secret_near(sx,sy)==bool(h.cpu.f&1),(stage,room,axis,coord,'85F4 bounds')
    checks+=1
 #7E77/7EF4 block contact for states>=5, even after C42D reaches0.
 # Use an overlapping live enemy sentinel; the real Z80 returns before touching IX.
 for state in (5,6,7,8):
  h=setup(rom,12,0,88,172,state=state);mem=h.cpu.memory
  enemy=bytes([1])+bytes(range(1,32));mem[0xC800:0xC820]=enemy;mem[0xC415]=32
  h.cpu.ix=0xC800;h.call(0x7E67);h.call(0x7EE4)
  assert mem[0xC415]==32 and bytes(mem[0xC800:0xC820])==enemy and not h.sounds
  lib.secret_init(12,0,88,172,state,0,0,17)
  assert lib.secret_frame_contact(state)==0 and lib.secret_energy()==32
 assert lib.secret_frame_contact(0)==1 and lib.secret_energy()==30,'overlapping enemy control must hurt standing Simon'
 # Crusher85A6 has a DIFFERENT guard (only state6), so do not add the
 # ordinary C420>=5 guard to the shared low-level damage helper.
 h=setup(rom,12,0,88,172,state=7);mem=h.cpu.memory;mem[0xC415]=32
 mem[0xC580:0xC588]=bytes([2,144,80,0,0,0,0,0]);h.call(0x85A6)
 assert mem[0xC420]==5 and mem[0xC415]==24
 lib.secret_init(12,0,88,172,2,0,0,17);assert lib.secret_begin(1)==21
 assert lib.secret_crusher()==1 and lib.secret_energy()==24
 assert lib.secret_poll()==0 and lib.secret_route()==0
 print('Secret85A6:crusher interrupts state7 at zero C42D;8 energy lost,route cancelled PASS',flush=True)
 print('Secret7E77/7EF4:states5..8 contact rejected with zero invulnerability PASS',flush=True)
 # Positive and negative controls, both faces, multiple free-running timer phases.
 for stage,room,dest,x,y in raw+[(s,0,0,88,168) for s in range(13,19)]:
  for name,state,up,dx,dy,invuln,left,tick in [('enter',2,1,0,0,0,0,0),('left',2,1,15,15,0,1,253),('no-up',2,0,0,0,0,0,7),('standing',0,1,0,0,0,0,127),('outside',2,1,16,0,0,0,255),('existing-invuln',2,1,0,0,90,0,5)]:
   sx=(x+dx)&255;sy=(y+4+dy)&255;label=(stage,room,name)
   h=setup(rom,stage,room,sx,sy,state,tick,invuln,17,left,True);mem=h.cpu.memory
   lib.secret_init(stage,room,sx,sy,state,tick,invuln,17);lib.secret_face(left)
   # Use real6DA9 only when its caller state is2. Other player states never call it.
   mem[0xC006]=32 if up else 0;h.sounds.clear()
   if state==2:h.call(0x6DA9)
   sound=lib.secret_begin(up);assert ([sound] if sound else [])==h.sounds,(label,'sound',sound,h.sounds)
   compare(lib,h,label,'begin');accepted=mem[0xC420]==7;arrival=None
   # Standalone negative controls intentionally do not execute unrelated walking.
   for p in range(180):
    mem[0xC003]=(mem[0xC003]+1)&255
    if p and mem[0xC420]==7:h.call(0x70FB)
    event=lib.secret_poll() if p else 0
    assert bool(event)==bool(mem[0xC41B]),(label,p,'transition request')
    compare(lib,h,label,p,-1)
    h.call(0x75C0);lib.secret_timers();compare(lib,h,label,('timers',p));images+=2
    if state==2:pixels+=sprite_compare(lib,h,left,label,p)
    if mem[0xC41B]:
     assert accepted and p==64,(label,'arrival pass',p)
     bank(h,13);h.call(0xB963);assert h.cpu.f&1;bank(h,3)
     assert lib.secret_route()==1;compare(lib,h,label,'B98C');assert mem[0xD001]==dest
     arrival=p
   assert (arrival is not None)==accepted,(label,'missing arrival')
   assert mem[0xD000]==stage and h.sounds==([sound] if sound else []),(label,'stage or extra sound')
   reports.append({'stage':stage,'room':room,'scenario':name,'accepted':accepted,'arrival_pass':arrival,'images':360})
  print('Secret',stage,room,'6 scenarios x360 images PASS',flush=True)
 print('Secret',checks,'bounds;',images,'images;',pixels,'sprite pixels;10 passages,none13..18 PASS')
 (ROOT/'research/secret-trace.json').write_text(json.dumps({'bounds':checks,'images':images,'sprite_pixels':pixels,'cases':reports},indent=2)+'\n')
if __name__=='__main__':main()
