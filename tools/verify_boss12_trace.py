#!/usr/bin/env python3
"""Stage12 BA26/BA64/BAF6: executed Z80 versus production boss.c."""
import ctypes as C,json,sys
from pathlib import Path
from probe import load_rom,reference_rom,ROOT
from intro_harness import IntroHarness
from extract_rooms import decode_room

def lifecycle(lib,rom):
 reports=[]
 for weapon,take in [(0,False),(1,True)]:
  h=IntroHarness(rom);m=h.cpu;mem=m.memory
  mem[0xD000]=12;mem[0xD001]=6;mem[0xD002]=3;mem[0xCE00]=4;mem[0xC418]=128;mem[0xC415]=8;mem[0xC002]=0x40
  mem[0xF0F1:0xF0F4]=bytes((1,2,3))
  tilemap=bytes(decode_room(rom,12,6)[2]);m.set_memory_block(0xD100,tilemap)
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.b12_init(cmap)
  requests=[];h.hooks[0x509F]=lambda:requests.append(m.a)
  trace=[];killed=False;health=8;exit_seen=False;pickup=False;hits=0
  for p in range(2200):
   sx,sy=64,160;mem[0xC003]=(p+1)&255;mem[0xC427]=sx;mem[0xC425]=sy
   if p>=150 and p%12==0 and not killed:
    requests.clear();mem[0xC416]=weapon;m.ix=0xC800;h.call(0x7E0C)
    was_killed=lib.b12_hit(weapon);hits+=1
    assert requests==([0x1C] if was_killed else []),('death sound',weapon,p,requests)
    assert lib.b12_health()==mem[0xC418],('health',weapon,p,lib.b12_health(),mem[0xC418])
    assert bool(was_killed)==bool(mem[0xCE15]);killed=bool(was_killed)
    score=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(mem[0xC405:0xC408]))
    assert score==(lib.b12_score()*100 if killed else 0),('boss score',p,score)
   frozen=int(weapon==1 and 600<=p<700);mem[0xD010]=frozen
   requests.clear()
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
   events=lib.b12_step(sx,sy,0,0,frozen,health)
   commands=([0] if events&128 else [])+([0x8C] if events&2 else [])+([1] if events&4 else [])
   assert commands==requests,('sound requests',weapon,p,commands,requests)
   for k in range(8):
    out=(C.c_ubyte*32)();alive=lib.b12_shot(k,out);a=0xD700+128*k
    assert bool(alive)==bool(mem[a]),('end projectile presence',weapon,p,k)
    if alive:assert bytes(out)==bytes(mem[a:a+32]),('end projectile',weapon,p,k,bytes(out).hex(),bytes(mem[a:a+32]).hex())
   values=(C.c_int*16)();lib.b12_script(values)
   assert list(values)[:3]==[mem[0xCE0B],mem[0xCE10],mem[0xCE02]],('script',weapon,p,list(values)[:3],list(mem[0xCE0B:0xCE11]),mem[0xCE02])
   if events&4:health+=1
   assert health==mem[0xC415],('heal',p,health,mem[0xC415])
   for k in range(2):
    a=0xC800+128*k;raw=bytes(mem[a:a+32]);out=(C.c_ubyte*32)()
    if k==0 and values[1]>=3:lib.b12_orb_record(out)
    else:lib.b12_record(k,out)
    assert bytes(out)==raw,('end record',weapon,p,k,[(hex(i),a,b) for i,(a,b) in enumerate(zip(raw,out)) if a!=b])
   if values[1]<3:
    expected_sprites=[]
    for a in [0xC800,0xC880]+[0xD700+128*k for k in range(8)]:
     if mem[a]:
      for i in range(0,mem[a+32],2):expected_sprites.append(list(mem[a+34+5*i:a+37+5*i]))
    sat=(C.c_int*48)();n=lib.b12_sprites(sat)
    assert [list(sat[3*i:3*i+3]) for i in range(n)]==expected_sprites,('end sprites',weapon,p,list(sat)[:3*n],expected_sprites)
   if take and mem[0xCE14] and not pickup:
    mem[0xC427]=120;mem[0xC425]=mem[0xC803];mem[0xC420]=0;h.call(0x7E67)
    assert mem[0xCE11]==1
    assert lib.b12_orb_touch(120,values[8],0)==1
    pickup=True
   trace.append({'frame':2*p,'ending':list(values)[:3],'energy':health,'enemy_energy':mem[0xC418],'events':events,'orb':bytes(mem[0xC800:0xC820]).hex()})
   if mem[0xC409]:
    assert events&8;assert mem[0xC425]==0xE0;exit_seen=True;break
   assert not events&8
  assert killed and exit_seen
  reports.append({'weapon':weapon,'pickup':take,'hits':hits,'frames':2*len(trace),'trace':trace})
 print('boss12 lifecycle:',[(r['weapon'],r['hits'],r['frames']) for r in reports],'PASS')
 return reports

def boxes_and_rider(lib,rom):
 checks={'contact':0,'whip':0,'armor':0,'rider_frames':0}
 tilemap=bytes(decode_room(rom,12,6)[2]);cmap=(C.c_ubyte*768).from_buffer_copy(tilemap)
 for part in (0,1):
  h=IntroHarness(rom);m=h.cpu;mem=m.memory;mem[0xD000]=12;mem[0xCE00]=4
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  h.call(0xB6AB);lib.b12_init(cmap);lib.b12_step(64,160,0,0,0,32)
  for k in (0,1):
   x,y=(128,160) if k==part else (0,0);a=0xC800+128*k;mem[a+3]=y;mem[a+5]=x;lib.b12_place(k,x,y)
  for state in (0,1,2,3,4):
   mem[0xC420]=state
   for sx in range(80,177,2):
    mem[0xC427]=sx
    for sy in range(80,209,2):
     mem[0xC425]=sy;m.ix=0xC800+128*part;h.call(0x8290)
     assert lib.b12_contact(sx,sy,state)==(m.f&1),('contact box',part,sx,sy,state)
     checks['contact']+=1
  for left in (0,1):
   mem[0xC42C]=left
   for chain in (0,1):
    mem[0xC416]=chain
    for state in (0,2):
     mem[0xC420]=state
     for sx in range(72,185,2):
      mem[0xC427]=sx
      for sy in range(80,193,2):
       mem[0xC425]=sy;m.ix=0xC800+128*part
       h.call(0x82B8) # class coordinates B,C
       h.call(0x8428 if part==0 else 0x8398)
       got=(lib.b12_rider_whip if part else lib.b12_whip)(sx,sy,left,state,chain)
       assert got==(m.f&1),('whip box',part,sx,sy,left,state,chain)
       checks['whip']+=1
  for sx in (123,133):
   for facing in (0,1):
    for armor in (0,1):
     mem[0xC415]=32;mem[0xC441]=16;mem[0xC701]=0x10 if armor else 0;mem[0xC427]=sx;mem[0xC42C]=facing
     m.ix=0xC800+128*part;m.b=1 if part else 3;h.call(0x8179)
     out=(C.c_int*2)();assert lib.b12_actor_contact(sx,160,0,facing,armor,out)
     assert list(out)==[mem[0xC415],mem[0xC441]],('damage/armor',part,sx,facing,armor,list(out),mem[0xC415],mem[0xC441])
     checks['armor']+=1
 # Hit the live rider through the actual post-box damage path, then compare all object bytes for 400 images.
 for chain in (0,1):
  h=IntroHarness(rom);m=h.cpu;mem=m.memory;mem[0xD000]=12;mem[0xCE00]=4;mem[0xC418]=128;m.set_memory_block(0xD100,tilemap)
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  lib.b12_init(cmap);mem[0xC002]=0x40;score=0
  for p in range(200):
   mem[0xC003]=(p+1)&255;mem[0xC427]=128;mem[0xC425]=160
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);lib.b12_step(128,160,0,0,0,32)
   if p in (60,100,140) and mem[0xC880]:
    m.ix=0xC880;mem[0xC416]=chain;h.call(0x7DD4);score+=lib.b12_rider_hit(chain+1)*100
    actual_score=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(mem[0xC405:0xC408]))
    assert score==actual_score,('rider score',chain,p,score,actual_score)
   for k in (0,1):
    out=(C.c_ubyte*32)();lib.b12_record(k,out);raw=bytes(mem[0xC800+128*k:0xC820+128*k])
    assert bytes(out)==raw,('rider hit',chain,p,k,raw.hex(),bytes(out).hex())
   checks['rider_frames']+=2
 # Projectile0Ah uses 830D class0, not the class1 of the stage6 fireball.
 h=IntroHarness(rom);m=h.cpu;mem=m.memory;mem[0xD700]=10;mem[0xD703]=160;mem[0xD705]=128
 lib.b12_init(cmap);lib.b12_place_shot();checks['shot_contact']=0;checks['shot_whip']=0
 for state in (0,1,2,3,4):
  mem[0xC420]=state
  for sx in range(112,145,2):
   mem[0xC427]=sx
   for sy in range(128,193,2):
    mem[0xC425]=sy;m.ix=0xD700;h.call(0x824C)
    assert lib.b12_shot_contact(sx,sy,state)==(m.f&1)
    checks['shot_contact']+=1
 for left in (0,1):
  mem[0xC42C]=left
  for chain in (0,1):
   mem[0xC416]=chain
   for state in (0,2):
    mem[0xC420]=state
    for sx in range(88,169,2):
     mem[0xC427]=sx
     for sy in range(128,193,2):
      mem[0xC425]=sy;m.ix=0xD700;h.call(0x8270)
      assert lib.b12_shot_whip(sx,sy,left,state,chain)==(m.f&1)
      checks['shot_whip']+=1
 checks['burst_frames']=0
 for slot in (0,1):
  h=IntroHarness(rom);m=h.cpu;mem=m.memory;mem[0xD000]=12;mem[0xCE00]=4;mem[0xC418]=128;m.set_memory_block(0xD100,tilemap)
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  m.a=0x18;m.bc=0x80A0;m.hl=0;m.de=0;h.call(0x9F6D)
  if slot:
   mem[0xD780:0xD800]=bytes(mem[0xD700:0xD780]);mem[0xD700]=0
  m.ix=0xD700+128*slot;h.call(0x9A1A)
  lib.b12_init(cmap);lib.b12_place_shot_slot(slot);lib.b12_shot_kill(slot)
  for p in range(20):
   frozen=int(3<=p<10);mem[0xD010]=frozen;mem[0xC003]=(p+1)&255;mem[0xC427]=64;mem[0xC425]=160
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);lib.b12_step(64,160,0,0,frozen,32)
   for k in range(8):
    a=0xD700+128*k;out=(C.c_ubyte*32)();alive=lib.b12_shot(k,out)
    assert bool(alive)==bool(mem[a]),('burst slot',slot,p,k)
    if alive:assert bytes(out)==bytes(mem[a:a+32]),('burst fields',slot,p,k,bytes(out).hex(),bytes(mem[a:a+32]).hex())
   checks['burst_frames']+=2
 print('boss12 boxes, armor and rider hits:',checks,'PASS')
 return checks

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.b12_init.argtypes=[C.POINTER(C.c_ubyte)]
 rom=load_rom(reference_rom());cases=[]
 navigation=(C.c_int*5)();lib.b12_navigation(navigation)
 h=IntroHarness(rom);mem=h.cpu.memory;mem[0xD000]=12;mem[0xC41E]=4;mem[0xC41F]=6;mem[0xC427]=236;mem[0xC425]=176
 requests=[];h.hooks[0x509F]=lambda:requests.append(h.cpu.a);h.call(0x77D1)
 assert list(navigation)[:3]==[1,6,1]
 assert (navigation[3],176 if navigation[4]<0 else navigation[4])==(mem[0xC427],mem[0xC425])
 assert requests==[0x87]
 print('boss12 entry: door5 to room6, 77D1 X/Y and sound87 PASS')
 for name,sx,sy,r,difficulty in [('left',32,160,0,0),('right',224,160,64,0),('near',128,112,127,0),('hard',80,160,32,2),('frozen',80,160,32,0)]:
  data=rom # Original LD A,R is executed; its result does not reach DE/A55D.
  h=IntroHarness(bytes(data));m=h.cpu;mem=m.memory
  mem[0xD000]=12;mem[0xD001]=6;mem[0xD002]=3;mem[0xCE00]=4;mem[0xC418]=128;mem[0xD012]=difficulty
  tilemap=bytes(decode_room(rom,12,6)[2]);m.set_memory_block(0xD100,tilemap)
  for a in range(0xD600,0xD700,4):mem[a]=0xE0
  cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.b12_init(cmap);trace=[];peak=0
  for p in range(600):
   mem[0xC003]=(p+1)&255;mem[0xC427]=sx;mem[0xC425]=sy
   frozen=int(name=='frozen' and 50<=p<150);mem[0xD010]=frozen
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
   lib.b12_step(sx,sy,difficulty,r,frozen,32)
   objects=[]
   for k in range(2):
    a=0xC800+128*k;out=(C.c_ubyte*32)();lib.b12_record(k,out);actual=bytes(mem[a:a+32])
    assert bytes(out)==actual,(name,p,k,[(hex(i),v,c) for i,(v,c) in enumerate(zip(actual,out)) if v!=c])
    objects.append(actual.hex())
   shots=[]
   for k in range(8):
    a=0xD700+128*k;out=(C.c_ubyte*32)();alive=lib.b12_shot(k,out)
    assert bool(alive)==bool(mem[a]),(name,p,k,'shot presence')
    if alive:
     actual=bytes(mem[a:a+32]);assert bytes(out)==actual,(name,p,k,'shot fields',actual.hex(),bytes(out).hex());shots.append(actual.hex())
   expected_sprites=[]
   for a in [0xC800,0xC880]+[0xD700+128*k for k in range(8)]:
    if not mem[a]:continue
    for i in range(0,mem[a+32],2):expected_sprites.append(list(mem[a+34+5*i:a+37+5*i]))
   sat=(C.c_int*48)();n=lib.b12_sprites(sat)
   assert [list(sat[3*i:3*i+3]) for i in range(n)]==expected_sprites,(name,p,'sprites',list(sat)[:3*n],expected_sprites)
   peak=max(peak,len(shots));assert lib.b12_signal()==mem[0xCE07];assert lib.b12_direction()==mem[0xCE09]+256*mem[0xCE0A]
   assert (mem[0xC427],mem[0xC425])==(sx,sy)
   trace.append({'frame':2*p,'objects':objects,'shots':shots,'simon':[sx,sy]})
  cases.append({'name':name,'frames':1200,'peak_shots':peak,'trace':trace})
 box_checks=boxes_and_rider(lib,rom)
 endings=lifecycle(lib,rom)
 print('boss12 motion:',len(cases),'scenarios',sum(c['frames'] for c in cases),'images PASS')
 (ROOT/'research/boss12-trace.json').write_text(json.dumps({'all_equal':True,'random_seam':None,'cases':cases,'lifecycle':endings,'box_checks':box_checks},indent=1)+'\n')
if __name__=='__main__':main()
