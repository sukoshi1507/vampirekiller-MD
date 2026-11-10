#!/usr/bin/env python3
"""BBF3/BC6D/9F22 stage15: actual ROM object, slots and sprites versus C."""
import ctypes as C,json,sys
from pathlib import Path
from probe import load_rom,reference_rom,ROOT
from intro_harness import IntroHarness
from extract_rooms import decode_room

def build(rom):
 h=IntroHarness(rom);m=h.cpu;mem=m.memory
 mem[0xD000]=15;mem[0xD001]=9;mem[0xD002]=4;mem[0xCE00]=5;mem[0xC418]=128;mem[0xF0F1:0xF0F4]=bytes([1,2,3])
 tilemap=bytes(decode_room(rom,15,9)[2]);m.set_memory_block(0xD100,tilemap)
 for a in range(0xD600,0xD700,4):mem[a]=0xE0
 return h,tilemap

def compare_objects(lib,h,label,p,orb=False,sprites=True):
 mem=h.cpu.memory;out=(C.c_ubyte*32)()
 if orb:lib.b15_orb_record(out)
 else:lib.b15_record(out)
 raw=bytes(mem[0xC800:0xC820]);assert bytes(out)==raw,(label,p,'body',[(hex(i),a,b) for i,(a,b) in enumerate(zip(raw,out)) if a!=b])
 shots=[]
 for k in range(8):
  a=0xD700+128*k;out=(C.c_ubyte*32)();alive=lib.b15_shot(k,out)
  assert bool(alive)==bool(mem[a]),(label,p,k,'slot presence')
  if alive:
   raw=bytes(mem[a:a+32]);assert bytes(out)==raw,(label,p,k,'projectile',raw.hex(),bytes(out).hex());shots.append(raw.hex())
 if sprites:
  expected=[]
  for a in [0xC800]+[0xD700+128*k for k in range(8)]:
   if mem[a]:
    for k in range(0,mem[a+32],2):expected.append(list(mem[a+34+5*k:a+37+5*k]))
  out=(C.c_int*60)();n=lib.b15_sprites(out)
  assert [list(out[3*i:3*i+3]) for i in range(n)]==expected,(label,p,'sprites',list(out)[:n*3],expected)
 return shots

def lifecycle(lib,rom):
 reports=[]
 for weapon,take in [(0,False),(1,True)]:
  h=IntroHarness(rom);m=h.cpu;mem=m.memory
  mem[0xD000]=15;mem[0xD001]=9;mem[0xD002]=4;mem[0xCE00]=5;mem[0xC418]=128;mem[0xC415]=8;mem[0xC002]=0x40
  mem[0xF0F1:0xF0F4]=bytes((1,2,3))
  tilemap=bytes(decode_room(rom,15,9)[2]);m.set_memory_block(0xD100,tilemap)
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.b15_init(cmap)
  requests=[];h.hooks[0x509F]=lambda:requests.append(m.a)
  trace=[];killed=False;health=8;exit_seen=False;pickup=False;hits=0
  for p in range(2200):
   sx,sy=64,160;mem[0xC003]=(p+1)&255;mem[0xC427]=sx;mem[0xC425]=sy
   if p>=150 and p%12==0 and not killed:
    requests.clear();mem[0xC416]=weapon;m.ix=0xC800;h.call(0x7E0C)
    was_killed=lib.b15_hit(weapon);hits+=1
    assert requests==([0x1C] if was_killed else []),('death sound',weapon,p,requests)
    assert lib.b15_health()==mem[0xC418],('health',weapon,p,lib.b15_health(),mem[0xC418])
    assert bool(was_killed)==bool(mem[0xCE15]);killed=bool(was_killed)
    score=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(mem[0xC405:0xC408]))
    assert score==(lib.b15_score()*100 if killed else 0),('boss score',p,score)
   frozen=int(weapon==1 and 600<=p<700);mem[0xD010]=frozen
   requests.clear()
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
   events=lib.b15_step(sx,sy,0,frozen,health)
   commands=([0] if events&128 else [])+([0x8C] if events&2 else [])+([1] if events&4 else [])
   assert commands==requests,('sound requests',weapon,p,commands,requests)
   for k in range(8):
    out=(C.c_ubyte*32)();alive=lib.b15_shot(k,out);a=0xD700+128*k
    assert bool(alive)==bool(mem[a]),('end projectile presence',weapon,p,k)
    if alive:assert bytes(out)==bytes(mem[a:a+32]),('end projectile',weapon,p,k,bytes(out).hex(),bytes(mem[a:a+32]).hex())
   values=(C.c_int*16)();lib.b15_script(values)
   assert list(values)[:(3 if values[1] else 2)]==[mem[0xCE0B],mem[0xCE10],mem[0xCE02]][:(3 if values[1] else 2)],('script',weapon,p,list(values)[:3],list(mem[0xCE0B:0xCE11]),mem[0xCE02])
   if events&4:health+=1
   assert health==mem[0xC415],('heal',p,health,mem[0xC415])
   compare_objects(lib,h,'lifecycle',p,orb=values[1]>=3,sprites=values[1]<3)
   if take and mem[0xCE14] and not pickup:
    mem[0xC427]=120;mem[0xC425]=mem[0xC803];mem[0xC420]=0;h.call(0x7E67)
    assert mem[0xCE11]==1
    assert lib.b15_orb_touch(120,values[4],0)==1
    pickup=True
   trace.append({'frame':2*p,'ending':list(values)[:3],'energy':health,'enemy_energy':mem[0xC418],'events':events,'orb':bytes(mem[0xC800:0xC820]).hex()})
   if mem[0xC409]:
    assert events&8;assert mem[0xC425]==0xE0;exit_seen=True;break
   assert not events&8
  assert killed and exit_seen
  reports.append({'weapon':weapon,'pickup':take,'hits':hits,'frames':2*len(trace),'trace':trace})
 print('boss15 lifecycle:',[(r['weapon'],r['hits'],r['frames']) for r in reports],'PASS')
 return reports


def boxes(lib,rom):
 checks=0
 for projectile in (False,True):
  h,tilemap=build(rom);m=h.cpu;mem=m.memory;cmap=(C.c_ubyte*768).from_buffer_copy(tilemap)
  lib.b15_init(cmap);h.call(0xB6AB);lib.b15_step(64,160,0,0,32)
  a=0xD700 if projectile else 0xC800;mem[a+3]=160;mem[a+5]=128
  if projectile:lib.b15_place_shot()
  else:lib.b15_place(128,160)
  for state in range(5):
   mem[0xC420]=state
   for sx in range(96,161,2):
    mem[0xC427]=sx
    for sy in range(96,225,2):
     mem[0xC425]=sy;m.ix=a;h.call(0x824C if projectile else 0x8290)
     got=(lib.b15_shot_contact if projectile else lib.b15_contact)(sx,sy,state)
     assert got==(m.f&1),('contact',projectile,sx,sy,state);checks+=1
  for left in (0,1):
   mem[0xC42C]=left
   for chain in (0,1):
    mem[0xC416]=chain
    for state in (0,2):
     mem[0xC420]=state
     for sx in range(72,185,2):
      mem[0xC427]=sx
      for sy in range(96,209,2):
       mem[0xC425]=sy;m.ix=a;h.call(0x8270 if projectile else 0x827C)
       got=(lib.b15_shot_whip if projectile else lib.b15_whip)(sx,sy,left,state,chain)
       assert got==(m.f&1),('whip',projectile,sx,sy,left,state,chain);checks+=1
  if not projectile:
   for sx in (123,133):
    for facing in (0,1):
     for armor in (0,1):
      mem[0xC415]=32;mem[0xC441]=16;mem[0xC701]=16 if armor else 0;mem[0xC427]=sx;mem[0xC425]=160;mem[0xC42C]=facing
      m.ix=a;m.b=3;h.call(0x8179);out=(C.c_int*2)()
      assert lib.b15_actor_contact(sx,160,0,facing,armor,out)
      assert list(out)==[mem[0xC415],mem[0xC441]];checks+=1
 print('boss15 collision and armor:',checks,'PASS')
 return checks

def reentry(lib,rom):
 h,tilemap=build(rom);mem=h.cpu.memory;cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.b15_init(cmap)
 for entry in range(3):
  if entry:
   mem[0xCE01]=0;mem[0xCE02]=0
   h.cpu.set_memory_block(0xC800,bytes(256));h.cpu.set_memory_block(0xD700,bytes(1024))
   for a in range(0xD600,0xD700,4):mem[a]=0xE0
   lib.b15_reenter(cmap)
  for p in range(170):
   mem[0xC003]=(p+1)&255;mem[0xC427]=64;mem[0xC425]=160
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918);lib.b15_step(64,160,0,0,32)
   compare_objects(lib,h,'reentry',170*entry+p)
   out=(C.c_int*3)();lib.b15_room(out);assert out[2]==mem[0xCF20]
 print('boss15 reentry: 1020 images, persistent CF20 PASS')


def bursts(lib,rom):
 frames=0
 for slot in (0,2):
  h,tilemap=build(rom);m=h.cpu;mem=m.memory;cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.b15_init(cmap)
  for p in range(60):
   mem[0xC003]=(p+1)&255;mem[0xC427]=64;mem[0xC425]=160
   if p==20:
    m.ix=0xD700+128*slot;assert mem[m.ix]==8;h.call(0x9A1A);lib.b15_shot_kill(slot)
   frozen=int(22<=p<35);mem[0xD010]=frozen
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918);lib.b15_step(64,160,0,frozen,32)
   compare_objects(lib,h,'burst',p);frames+=2
 print('boss15 projectile destruction:',frames,'images, slots 0/2 and freeze PASS')
 return frames

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.b15_init.argtypes=[C.POINTER(C.c_ubyte)]
 rom=load_rom(reference_rom());cases=[]
 navigation=(C.c_int*5)();lib.b15_navigation(navigation)
 h=IntroHarness(rom);mem=h.cpu.memory;mem[0xD000]=15;mem[0xC41E]=9;mem[0xC41F]=7;mem[0xC427]=16;mem[0xC425]=176
 requests=[];h.hooks[0x509F]=lambda:requests.append(h.cpu.a);h.call(0x77D1)
 assert list(navigation)==[1,9,1,246,-1]
 assert (mem[0xC427],mem[0xC425])==(246,176) and requests==[0x87]
 print('boss15 entry: door8 to room9, 77D1 X/Y and sound87 PASS')
 for name,sx,sy,difficulty in [('left',32,160,0),('right',224,160,0),('near',128,112,0),('hard',80,160,2),('frozen',80,160,0),('frozen_birth',80,160,0)]:
  h,tilemap=build(rom);mem=h.cpu.memory;cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.b15_init(cmap);trace=[];peak=0
  requests=[];h.hooks[0x509F]=lambda:requests.append(h.cpu.a)
  for p in range(800):
   frozen=int((name=='frozen' and 50<=p<200) or (name=='frozen_birth' and p<80));mem[0xC003]=(p+1)&255;mem[0xC427]=sx;mem[0xC425]=sy;mem[0xD012]=difficulty;mem[0xD010]=frozen
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
   events=lib.b15_step(sx,sy,difficulty,frozen,32);assert not events&0x20;assert requests==[]
   shots=compare_objects(lib,h,name,p)
   room=(C.c_int*3)();lib.b15_room(room);assert list(room)==[mem[0xCE01],mem[0xCE02],mem[0xCF20]],(name,p,'room',list(room),mem[0xCE02],mem[0xCF20])
   assert (mem[0xC427],mem[0xC425])==(sx,sy);peak=max(peak,len(shots))
   trace.append({'frame':2*p,'body':bytes(mem[0xC800:0xC820]).hex(),'shots':shots,'room':list(room),'simon':[sx,sy]})
  cases.append({'name':name,'frames':1600,'peak_shots':peak,'trace':trace})
 print('boss15 motion:',len(cases),'scenarios',sum(c['frames'] for c in cases),'images, peaks',[c['peak_shots'] for c in cases],'PASS')
 burst_frames=bursts(lib,rom)
 box_checks=boxes(lib,rom)
 reentry(lib,rom)
 endings=lifecycle(lib,rom)
 (ROOT/'research/boss15-trace.json').write_text(json.dumps({'all_equal':True,'cases':cases,'lifecycle':endings,'box_checks':box_checks,'burst_frames':burst_frames},indent=1)+'\n')
if __name__=='__main__':main()
