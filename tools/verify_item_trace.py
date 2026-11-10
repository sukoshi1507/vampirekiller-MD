#!/usr/bin/env python3
"""LOT10 inventory, timers, projectile shield, revelation and F2 versus executed ROM."""
import ctypes as C,json,sys,os
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_trade_trace import TradeHarness
from build_secret_data import collect

class ItemHarness(TradeHarness):
 def __init__(self,rom):
  super().__init__(rom);self.death_r=None;self.cpu.set_breakpoint(0x9AE7)
 def call(self,address,budget=5000000):
  c=self.cpu;c.sp=0xF100;c.memory[0xF100:0xF102]=bytes([0,1]);c.pc=address;c.ticks_to_stop=budget
  for _ in range(100000):
   c.run()
   if c.pc==0x0100 or c.pc==self.stop:return
   if c.pc==0x9AE7:self.death_r=c.a;c.step_over_breakpoint()
   elif c.pc==0x92EA:self.random.append(c.a);c.step_over_breakpoint()
   elif c.pc==0x925E:
    k=(c.ix-0xC5B5)//16
    if 0<=k<2:self.frames[k]=c.a
    c.step_over_breakpoint()
   elif c.pc in self.hooks:
    self.hooks[c.pc]();c.pc=c.memory[c.sp]|c.memory[c.sp+1]<<8;c.sp=(c.sp+2)&65535
   elif not c.ticks_to_stop:raise RuntimeError(f'budget {c.pc:04X} calling {address:04X}')
  raise RuntimeError('too many stops')

def setup(rom,stage=12,room=0):
 h=ItemHarness(rom);mem=h.cpu.memory;mem[0xF0F1:0xF0F4]=bytes([1,2,3]);mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage]
 for a in (0x548D,0x53B6,0x564C,0x570D,0x5768,0x5780,0x4F83,0x4F91,0x5A49,0x5B1B,0x8671):h.call(a)
 mem[0xC415]=32;mem[0xC417]=0x50
 return h

def val(lib):
 o=(C.c_int*16)();lib.item_values(o);return list(o)

def compare_inventory(lib,h,label):
 m=h.cpu.memory;expected=[m[0xC419],m[0xC431],m[0xC701],m[0xC702],(m[0xC417]>>4)*10+(m[0xC417]&15),m[0xC700],m[0xC416],m[0xC70F],m[0xC415],m[0xC441],m[0xC440],m[0xC43A],m[0xC434],m[0xC43E]]
 assert val(lib)[:14]==expected,(label,val(lib)[:14],expected)

def inventory(lib,rom):
 images=0
 for kind in (3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,22,26,27,28,29,30):
  for flags in (0,4):
   h=setup(rom);m=h.cpu.memory;m[0xC431]=flags;lib.item_init(12,0,flags);h.cpu.a=kind;h.call(0x8D30);sound=lib.item_give(kind)
   assert h.cpu.a==sound,(kind,'returned sound',h.cpu.a,sound)
   compare_inventory(lib,h,('pickup',kind,flags))
   # Only timer/color routines run here: no invented idle physics or enemies.
   for p in range(260):
    m[0xC003]=(p+1)&255;h.sounds.clear()
    for a in (0x75C0,0x75E2,0x75F2,0x7604):h.call(a)
    lib.item_timers();compare_inventory(lib,h,('timers',kind,flags,p))
    tail_before=bytes(m[0xD400:0xD480]);h.call(0x790C);colour=val(lib)[14];size=64 if m[0xC416]>=2 else 128
    expected=bytes([colour])*128 if colour else bytes([1 if (i//16)%2==0 else 0x42 for i in range(128)])
    assert bytes(m[0xD400:0xD400+size])==expected[:size] and bytes(m[0xD400+size:0xD480])==tail_before[size:],(kind,flags,p,'Simon colors')
    assert h.sounds==([val(lib)[15]] if val(lib)[15] else []),(kind,flags,p,'timer sounds',h.sounds,val(lib)[15])
    images+=2
 print('Items inventory/timers',images,'images,44 scenarios PASS',flush=True)
 return images

def shield_boxes(lib,rom):
 h=TradeHarness(rom);m=h.cpu.memory;checks=0
 for kind in range(1,12):
  m[0xD700]=kind;h.cpu.ix=0xD700;m[0xD705]=88;m[0xD703]=144
  for left in (0,1):
   m[0xC42C]=left
   for x in range(256):
    for y in (0,100,131,132,143,144,145,175,176,177,240,255):
     m[0xC427]=x;m[0xC425]=y;h.call(0x8264)
     assert lib.item_box(kind,x,y,left,88,144)==bool(h.cpu.f&1),(kind,x,y,left,'8264 geometry')
     checks+=1
 print('Items shield geometry',checks,'executed8264 comparisons PASS',flush=True)
 return checks

def shield_scenarios(lib,rom):
 images=0
 for kind in (3,4,9):
  for left,charges,inventory in [(0,16,32),(1,1,32),(0,0,32),(1,16,0)]:
   h=setup(rom);m=h.cpu.memory;lib.item_init(12,0,0);h.call(0x63C5)
   h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(96<<8)|144;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
   assert m[0xD700]==kind
   lib.item_shot(kind,96,144,0);meta=(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]);lib.item_shot_metadata(0,meta)
   m[0xC701]=inventory;m[0xC441]=charges;m[0xD010]=1;lib.item_freeze(1)
   # Preserve the whole raw prefix, including unused retained bytes of9F6D.
   for p in range(180):
    sx=88 if not left else 104;sy=172
    if p<20:sx=32
    m[0xC427]=sx;m[0xC425]=sy;m[0xC42C]=left;h.sounds.clear()
    native_charges=charges if p==0 else val(lib)[9]
    lib.item_shield_config(sx,sy,left,native_charges,inventory if p==0 else val(lib)[2])
    h.call(0x8610);n=lib.item_shield()
    assert h.sounds==[11]*n,(kind,left,p,'shield sounds',h.sounds,n)
    assert val(lib)[9]==m[0xC441] and val(lib)[2]==m[0xC701],(kind,left,p,'shield charges',val(lib)[9],m[0xC441])
    out=(C.c_ubyte*32)();sat=(C.c_int*13)();lib.item_shot_record(0,out,sat)
    assert bytes(out)==bytes(m[0xD700:0xD720]),(kind,left,p,'shield raw32',bytes(out).hex(),bytes(m[0xD700:0xD720]).hex())
    h.call(0x9E31);lib.item_timers();lib.item_shot_record(0,out,sat)
    assert bytes(out)==bytes(m[0xD700:0xD720]),(kind,left,p,'projectile update32',bytes(out).hex(),bytes(m[0xD700:0xD720]).hex())
    assert not any(m[0xC405:0xC408]),'shield gives no score'
    images+=2
   print('Items8610',kind,left,charges,inventory,'360 images PASS',flush=True)
 return images

def shield_relocation(lib,rom):
 for slots in ((2,),(0,3),(1,2,3)):
  h=setup(rom);m=h.cpu.memory;lib.item_init(12,0,0);h.call(0x63C5)
  for slot in range(max(slots)+1):
   h.cpu.a=10;h.cpu.bc=(96<<8)|144;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
  for slot in range(8):
   if slot not in slots:m[0xD700+128*slot]=0
   else:lib.item_shot(3,96,144,slot)
   raw=(C.c_ubyte*32).from_buffer_copy(m[0xD700+128*slot:0xD720+128*slot]);lib.item_pool_metadata(slot,raw)
  m[0xC701]=32;m[0xC441]=1;m[0xD010]=1;lib.item_freeze(1)
  for p in range(180):
   m[0xC427]=88 if p>=20 else 32;m[0xC425]=172;m[0xC42C]=0;h.sounds.clear()
   lib.item_shield_config(m[0xC427],172,0,1 if p==0 else val(lib)[9],32 if p==0 else val(lib)[2])
   h.call(0x8610);n=lib.item_shield()
   assert h.sounds==[11]*n and val(lib)[9]==m[0xC441] and val(lib)[2]==m[0xC701],(slots,p,'charge/sound')
   for slot in range(8):
    out=(C.c_ubyte*32)();lib.item_pool_record(slot,out);expected=bytes(m[0xD700+128*slot:0xD720+128*slot])
    assert bytes(out)==expected,(slots,p,slot,'relocation raw32',bytes(out).hex(),expected.hex())
   h.call(0x9E31);lib.item_timers()
  print('Items8610 relocation',slots,'360 images,all8 physical prefixes PASS',flush=True)

def boss_shield(lib,rom):
 for kind in range(5):
  h=setup(rom);m=h.cpu.memory;h.call(0x63C5);h.cpu.a=10;h.cpu.bc=(96<<8)|144;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
  meta=(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]);lib.item_boss_init(kind,meta)
  m[0xC701]=32;m[0xC441]=16;m[0xC427]=88;m[0xC425]=172;m[0xD010]=1;h.call(0x8610);lib.item_boss_shield()
  for p in range(17):
   out=(C.c_ubyte*32)();lib.item_boss_record(out)
   assert bytes(out)==bytes(m[0xD700:0xD720]),('boss shield flame32',kind,p,bytes(out).hex(),bytes(m[0xD700:0xD720]).hex())
   h.call(0x9E31);lib.item_boss_step()
  print('Items boss shield',kind,'full disappearance prefix PASS',flush=True)

def recoil(lib,rom):
 images=0
 for flags in (0,8):
  for direction in (-1,1):
   h=setup(rom,1,0);m=h.cpu.memory;m[0xC420]=5;m[0xC423]=0;m[0xC425]=192;m[0xC427]=96;m[0xC426]=0;m[0xC431]=flags;m[0xC43C]=int(direction>0);m[0xC007]=0;m[0xC006]=0
   lib.item_recoil_init(flags,direction)
   for p in range(180):
    h.call(0x6B39);out=(C.c_int*5)();lib.item_recoil_step(out)
    assert list(out)[:4]==[m[0xC427],m[0xC425],m[0xC426],m[0xC42A]],('boots recoil',flags,direction,p,list(out),[m[0xC427],m[0xC425],m[0xC426],m[0xC42A]])
    images+=2
   print('Items0C recoil',flags,direction,'360 images PASS',flush=True)
 return images

def late_contacts(lib,rom):
 cases=0
 for stage,kind in ((13,9),(16,18),(12,11),(13,13),(17,14),(18,16),(12,15),(13,35)):
  for item in (8,9,16,17):
   h=setup(rom,stage,0);m=h.cpu.memory;m[0xC427]=88;m[0xC425]=172;m[0xC002]=64;m[0xCE00]=0;m[0xCF40]=0
   h.call(0x63C5);h.cpu.c=kind;h.cpu.d=88;h.cpu.e=172;h.call(0x5F1D)
   lib.item_init(stage,0,0);lib.item_enemy(kind,88,172)
   if kind==9:
    # Execute AF5F's real wake-up; this is a valid end-of-dormancy input state.
    m[0xC811]=1;h.call(0xAF5F);lib.item_enemy_active09(m[0xC805],m[0xC804])
   initial=[bytes(m[0xC800+128*i:0xC820+128*i]) for i in range(7)]
   h.cpu.a=item;h.call(0x8D30);lib.item_give(item)
   m[0xD010]=1;h.sounds.clear();h.call(0x7E67)
   lib.item_contact_once(item,h.death_r if h.death_r is not None else 1)
   out=(C.c_int*45)();lib.item_enemy_result(out)
   assert out[0]==m[0xC415] and (item not in (8,9) or out[0]==32),(stage,kind,item,'energy',out[0],m[0xC415])
   score=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(m[0xC405:0xC408]))
   assert out[1]*100==score and h.sounds==([out[2]] if out[2] else []),(kind,item,'score/sounds',out[1],score,h.sounds,out[2])
   if item in (8,16,17):
    assert all(bytes(m[0xC800+128*i:0xC820+128*i])==initial[i] for i in range(7)),(kind,item,'inventory must not kill/mutate enemies')
   elif kind==9:
    assert m[0xC800]==out[3]==9 and m[0xC801]==out[8]==0 and m[0xC811]==out[7]==64 and m[0xC80B]==out[6]==0x26
   else:
    for i in range(7):
     a=0xC800+128*i
     if m[a]==30:
      assert list(out[3+6*i:9+6*i])==[30,m[a+5],m[a+3],m[a+11],m[a+12],m[a+31]],(kind,item,'death',i,list(out[3+6*i:9+6*i]),bytes(m[a:a+32]).hex())
     elif m[a]==38:
      drop=(C.c_int*4)();lib.item_drop(drop);assert list(drop)==[1,m[a+31],m[a+5],m[a+3]],(kind,item,'item flame',list(drop),bytes(m[a:a+32]).hex())
   cases+=1
  print('Items08/09/books late type',kind,'contact outcomes PASS',flush=True)
 return cases

def long_contacts(lib,rom):
 images=0
 for stage,kind in ((13,9),(16,18),(12,11),(13,13),(17,14),(18,16),(12,15),(13,35)):
  for item in (8,9,16,17):
   h=setup(rom,stage,0);m=h.cpu.memory;m[0xC427]=88;m[0xC425]=172;m[0xC002]=64;m[0xCE00]=0;m[0xCF40]=0
   h.call(0x63C5);h.cpu.c=kind;h.cpu.d=88;h.cpu.e=172;h.call(0x5F1D)
   lib.item_init(stage,0,0);lib.item_enemy(kind,88,172)
   if kind==9:m[0xC811]=1;h.call(0xAF5F);lib.item_enemy_active09(m[0xC805],m[0xC804])
   for i in range(7):lib.item_enemy_metadata(i,(C.c_ubyte*32).from_buffer_copy(m[0xC800+128*i:0xC820+128*i]))
   h.cpu.a=item;h.call(0x8D30);lib.item_give(item);m[0xD010]=1;m[0xC440]=255
   cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.item_contact_map(cmap)
   # First contact is isolated; subsequent passes keep D010 but advance the real timers/death loop.
   h.call(0x75C0);h.call(0x75E2);h.call(0x7E67);lib.item_contact_once(item,h.death_r if h.death_r is not None else 1)
   for p in range(180):
    for i in range(7):
     addr=0xC800+128*i
     out=(C.c_ubyte*32)();lib.item_enemy_prefix(i,out)
     assert bytes(out)==bytes(m[addr:addr+32]),('long contact32',kind,item,p,i,bytes(out).hex(),bytes(m[addr:addr+32]).hex())
    assert val(lib)[8]==m[0xC415],('long energy',kind,item,p,val(lib)[8],m[0xC415])
    # Fixed Simon and cleared recoil/invulnerability are deliberate external inputs, not ROM patches.
    m[0xC42D]=0;m[0xC420]=0;m[0xC003]=(p+1)&255;m[0xC440]=255
    for call in (0x75C0,0x75E2,0x98E5,0x7E67):h.call(call)
    lib.item_long_step();images+=2
   print('Items extended contact',kind,item,'360 images, all C800 prefixes/energy PASS',flush=True)
 print('Extended contact: all active and inactive C800 prefixes compared, including24/26 reservations',flush=True)
 return images

def loot_actor(lib,rom):
 from verify_z80_maps import execute_map
 lib.loot_actor_begin.argtypes=[C.POINTER(C.c_ubyte),C.POINTER(C.c_ubyte)]+[C.c_int]*5
 images=0
 for kind in (36,38):
  for mode in ('floor','air','guard','invuln','recoil'):
   h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128);tilemap=execute_map(rom,12,0)[0];m[0xD100:0xD400]=tilemap
   meta=bytes([0]+[(i*7+1)&255 for i in range(1,32)]);m[0xC800:0xC820]=meta;h.cpu.c=kind;h.cpu.de=0x5840;h.call(0x5F1D);m[0xC81F]=2 if kind==38 else 0
   cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.loot_actor_begin(cmap,(C.c_ubyte*32).from_buffer_copy(meta),12,kind,88,64,2)
   for p in range(180):
    m[0xC003]=(p+1)&255;h.sounds.clear()
    sx,sy=(m[0xC805],m[0xC803]) if mode!='floor' and p>=4 and m[0xC800] else (16,32)
    state=5 if mode=='recoil' and p<40 else 0;guard=20 if mode=='guard' and p<40 else 0;invuln=20 if mode=='invuln' and p<40 else 0
    m[0xC427]=sx;m[0xC425]=sy;m[0xC420]=state;m[0xC43A]=guard;m[0xC42D]=invuln
    if m[0xC800]:h.cpu.ix=0xC800;h.call(0x9BD9 if kind==36 else 0x9C1E);h.call(0x99B9);h.call(0x99E5)
    h.call(0x7E67);h.call(0x7FE2);h.call(0x8A4A);lib.loot_actor_step(sx,sy,state,guard,invuln,0)
    out=(C.c_ubyte*32)();o=(C.c_int*44)();lib.loot_actor_record(out,o)
    assert bytes(out)==bytes(m[0xC800:0xC820]),('integrated loot32',kind,mode,p,bytes(out).hex(),bytes(m[0xC800:0xC820]).hex())
    assert o[0]==(m[0xC417]>>4)*10+(m[0xC417]&15),('loot hearts',kind,mode,p,o[0],m[0xC417])
    if o[2] and kind==38:assert o[3]==m[0xC825]&15,('loot colour',mode,p,o[3],m[0xC825])
    for i in range(8):
     b=0xC500+16*i;assert bool(o[4+i*5])==bool(m[b]),('C500 existence',kind,mode,p,i,o[4+i*5],m[b])
     if m[b]:assert list(o[5+i*5:9+i*5])==[m[b+2],m[b+1],m[b+4],m[b+6]],('C500 fields',kind,mode,p,i,list(o[5+i*5:9+i*5]),bytes(m[b:b+16]).hex())
    if m[0xC800]:
     h.cpu.ix=0xC800;h.call(0x6445);sp=(C.c_int*25)();lib.loot_actor_sprites(sp)
     assert sp[0]==1 and tuple(sp[1:3])==tuple(m[0xC822:0xC824]),('loot sprite placement',kind,mode,p,list(sp)[:4],bytes(m[0xC822:0xC827]).hex())
    assert h.sounds==([o[1]] if o[1] else []),('loot sounds',kind,mode,p,h.sounds,o[1]);images+=2
   print('Integrated loot',kind,mode,'360 images,IX32/C500/pickup/guards/sounds PASS',flush=True)
 # Resource proof: seven C800 hearts use14 SAT entries; probe allocation every pass.
 h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5);tilemap=execute_map(rom,12,0)[0];m[0xD100:0xD400]=tilemap;m[0xC427]=0;m[0xC425]=0
 for i in range(7):h.cpu.c=36;h.cpu.d=24+32*i;h.cpu.e=64;h.call(0x5F1D)
 cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.loot_capacity_begin(cmap)
 accepted=0
 for p in range(180):
  h.cpu.c=1;h.cpu.de=0x80A0;h.call(0x5F1D);found=[i for i in range(7) if m[0xC800+128*i]==1]
  assert lib.loot_capacity_probe()==bool(found),('loot capacity',p,found)
  if found:h.cpu.ix=0xC800+128*found[0];h.call(0x99F6);accepted+=1
  h.call(0x98E5);assert lib.loot_capacity_step()==sum(bool(m[0xC800+128*i]) for i in range(7)),('C800 reservation counts',p)
 assert accepted>0
 print('Integrated loot seven-slot capacity/SAT gating,360 images PASS',flush=True)
 return images

def late_ghosts(lib,rom):
 from build_candles_md import collect as candle_collect
 from extract_rooms import decode_room
 defs=candle_collect(rom)[0];images=0
 lib.item_ghost_init.argtypes=[C.POINTER(C.c_ubyte)]+[C.c_int]*6
 for x,y,mode,kind,content,stage,room in defs:
  if stage<12 or kind!=21:continue
  for sx,sy in ((32,160),(224,80)):
   h=setup(rom,stage,room);m=h.cpu.memory;h.call(0x63C5)
   for a in range(0xD600,0xD700,4):m[a]=224
   m[0xC427]=sx;m[0xC425]=sy;h.cpu.c=27;h.cpu.d=x;h.cpu.e=y+16;h.call(0x5F1D)
   tilemap=bytes(decode_room(rom,stage,room)[2]);m[0xD100:0xD400]=tilemap;cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.item_ghost_init(cmap,stage,room,x,y,sx,sy)
   for p in range(200):
    if m[0xC800]:h.cpu.ix=0xC800;h.call(0xB4EA);h.call(0x99B9);h.call(0x99E5);h.call(0x6445)
    lib.item_ghost_step();out=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.item_ghost_prefix(out,sp)
    assert bytes(out)==bytes(m[0xC800:0xC820]),('late1B prefix32',stage,room,x,y,p,bytes(out).hex(),bytes(m[0xC800:0xC820]).hex())
    if out[0]:
     a=0xC822;assert sp[24]==1 and tuple(sp[:2])==tuple(m[a:a+2]),('late1B SAT placement',stage,room,p,sp[:3],bytes(m[a:a+5]).hex())
    images+=2
   print('Late1B',stage,room,x,y,sx,'400 images,32-byte prefix/SAT PASS',flush=True)
 return images

def outlines(lib,rom,counts):
 checks=0;blocks=0
 for stage in range(19):
  for room in range(counts[stage]):
   h=setup(rom,stage,room);m=h.cpu.memory
   before=bytes(h.vram[:32768]);h.call(0x870C)
   # Compare the exact WHITE writes; background remains unchanged elsewhere.
   expected=bytearray(before)
   for address in range(0xC470,0xC4F0,16):
    if m[address]!=2 or m[address+4]<2:continue
    x,y,mode=m[address+2],m[address+1],m[address+4];size=16 if mode==2 else 32;blocks+=1
    for dy in range(size):
     for dx in range(size):
      colour=lib.item_outline(mode,dx,dy)
      if colour:
       pos=(y+dy)*128+(x+dx)//2;b=expected[pos];expected[pos]=(b&240)|colour if (x+dx)&1 else (b&15)|(colour<<4)
      checks+=1
   assert bytes(h.vram[:32768])==expected,('870C outline pixels',stage,room,next((i for i,(a,b) in enumerate(zip(h.vram,expected)) if a!=b),None))
 print('Items870C',blocks,'blocks;',checks,'outline pixels PASS',flush=True)
 return checks

def map_lifecycle(lib,rom):
 images=0
 for flags,boss,inventory,uses in ((64,0,128,3),(0,0,128,3),(64,1,128,3),(64,6,128,3),(64,0,0,3),(64,0,128,0)):
  h=setup(rom,1,0);m=h.cpu.memory;m[0xC002]=flags;m[0xCE00]=boss;m[0xC701]=inventory;m[0xC70F]=uses;m[0xC425]=172;m[0xC427]=88
  lib.item_init(1,0,0);lib.item_map_inventory(inventory,uses);player=bytes(m[0xC420:0xC440]);opens=0
  for p in range(180):
   press=int(p in (0,20,40,60,80,100,120,140));m[0xC00B]=2*press;h.sounds.clear();h.call(0x9552);event=lib.item_map_step(flags,boss,press)
   assert m[0xCF38]==lib.item_map_state() and val(lib)[7]==m[0xC70F] and val(lib)[2]==m[0xC701],('F2 lifecycle',flags,boss,inventory,uses,p)
   assert bytes(m[0xC420:0xC440])==player,('F2 freezes Simon',p)
   assert h.sounds==([25] if event==1 else []),('F2 sound',p,event,h.sounds)
   if event==1:opens+=1
   images+=2
  assert opens==(3 if flags==64 and boss==0 and inventory==128 and uses==3 else 0)
 print('Items F2 lifecycle',images,'images;third use displayed,blocked contexts PASS',flush=True)
 return images

def water(lib,rom):
 images=0
 for state,hearts,inventory,left,right,whip in ((1,50,8,1,0,0),(1,50,8,0,1,0),(0,50,8,1,0,0),(1,4,8,0,1,0),(1,50,0,1,0,0)):
  h=setup(rom,1,0);m=h.cpu.memory;m[0xC420]=state;m[0xC425]=112;m[0xC427]=88;m[0xC701]=inventory;m[0xC417]=(hearts//10)*16+hearts%10;m[0xC006]=(4*left+8*right+16*whip)
  # Room loads leave empty shot Y atE0 (7513).
  h.call(0x7513);lib.item_init(1,0,0);lib.item_water_init(state,hearts,inventory)
  h.call(0x7116);lib.item_water_begin(left,right,whip)
  for p in range(180):
   out=(C.c_ubyte*32)();lib.item_water_record(out)
   assert bytes(out)==bytes(m[0xC450:0xC470]),('1E water raw32',state,hearts,inventory,left,p,bytes(out).hex(),bytes(m[0xC450:0xC470]).hex())
   assert val(lib)[4]==(m[0xC417]>>4)*10+(m[0xC417]&15)
   m[0xC003]=(p+1)&255;h.call(0x72B2);lib.item_water_step();images+=2
  print('Items1E holy water',state,hearts,inventory,left,right,'360 images PASS',flush=True)
 return images

def maps(lib,rom,counts):
 checks=0
 for stage in range(19):
  for room in range(counts[stage]):
   h=setup(rom,stage,room);m=h.cpu.memory;m[0xC002]=64;m[0xCE00]=0;m[0xC701]=128;m[0xC70F]=3;m[0xC00B]=2;m[0xC427]=88;m[0xC425]=172
   lib.item_init(stage,room,0);lib.item_give(15)
   h.call(0x9552);assert lib.item_map_step(64,0,1)==1;assert m[0xCF38]==lib.item_map_state()==1
   m[0xC00B]=0;h.call(0x9552);assert lib.item_map_step(64,0,0)==2;assert m[0xCF38]==lib.item_map_state()==2
   pix=(C.c_ubyte*13312)();lib.item_map(stage,room,88,172,pix)
   expected=bytes(h.pixel(24+x,48+y) for y in range(126) for x in range(208))
   actual=bytes((pix[((y//8)*26+x//8)*32+(y%8)*4+(x%8)//2]>>(0 if x%2 else 4))&15 for y in range(126) for x in range(208))
   assert actual==expected,('F2 map pixels',stage,room,next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),None))
   checks+=len(actual)
  print('Items F2 stage',stage,'all rooms pixel PASS',flush=True)
 print('Items F2',checks,'pixels PASS')
 return checks

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());counts,_,_=collect()
 if os.environ.get("VK_ITEM_ONLY_LOOT"):loot_actor(lib,rom);return
 if os.environ.get("VK_ITEM_ONLY_GHOST"):late_ghosts(lib,rom);return
 images=inventory(lib,rom);boxes=shield_boxes(lib,rom);shield_images=shield_scenarios(lib,rom);shield_relocation(lib,rom);boss_shield(lib,rom);recoil_images=recoil(lib,rom);loot_actor(lib,rom);contacts=late_contacts(lib,rom);long_contacts(lib,rom);late_ghosts(lib,rom);pixels=outlines(lib,rom,counts);map_pixels=maps(lib,rom,counts);map_images=map_lifecycle(lib,rom);water_images=water(lib,rom)
 (ROOT/'research/items-trace.json').write_text(json.dumps({'inventory_images':images,'box_checks':boxes,'shield_images':shield_images,'recoil_images':recoil_images,'late_contacts':contacts,'outline_pixels':pixels,'map_pixels':map_pixels,'map_images':map_images,'water_images':water_images},indent=2)+'\n')
if __name__=='__main__':main()
