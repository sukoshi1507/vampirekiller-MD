#!/usr/bin/env python3
"""Production separate7 C800 /8 D700 capacity and per-frame states vs real ROM."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());images=0;cases=[]
 lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)]
 for boss in (0,1):
  for kind in (3,4,9):
   for shield in (False,True):
    h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
    cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap);lib.pools_boss(boss);m[0xCE03]=boss;lib.candle_busy(7)
    for i in range(7):
     h.cpu.c=30;h.cpu.d=24+24*i;h.cpu.e=64;h.call(0x5F1D);m[0xC80C+128*i]=255;lib.item_enemy_metadata(i,(C.c_ubyte*32).from_buffer_copy(m[0xC800+128*i:0xC820+128*i]))
    for i in range(8):
     h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(96<<8)|144;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
     raw=(C.c_ubyte*32).from_buffer_copy(m[0xD700+128*i:0xD720+128*i]);result=lib.pools_spawn(kind,raw);assert result==(i if m[0xD700+128*i] else -1),(kind,i,result,m[0xD700+128*i])
    assert lib.pools_count(0)==7 and lib.pools_count(1)==sum(bool(m[0xD700+128*i]) for i in range(8))
    assert lib.pools_spawn(kind,raw)==-1,'ninth D700 must be rejected'
    for p in range(180):
     m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;m[0xC427]=0;m[0xC425]=32;h.sounds.clear()
     if shield and p==4:
      m[0xC701]=32;m[0xC441]=1;m[0xC427]=88;m[0xC425]=172;h.call(0x8610);n=lib.pools_shield(1);assert n==len(h.sounds) and n>0,('shield count',n,len(h.sounds),lib.pools_count(1))
      assert lib.pools_armor()==m[0xC441] and lib.pools_inventory()==m[0xC701] and h.sounds==[11]*n
     h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
     for i in range(7):
      raw=(C.c_ubyte*32)();lib.item_enemy_prefix(i,raw);assert bytes(raw)==bytes(m[0xC800+128*i:0xC820+128*i]),('C80032',kind,shield,p,i)
     for i in range(8):
      raw=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.pools_record(i,raw,sp)
      assert bytes(raw)==bytes(m[0xD700+128*i:0xD720+128*i]),('D70032',kind,shield,p,i,bytes(raw).hex(),bytes(m[0xD700+128*i:0xD720+128*i]).hex())
      if m[0xD700+128*i]:
       h.cpu.ix=0xD700+128*i;h.call(0x6445)
       visible=any(m[0xD725+128*i+5*j]&15 for j in range(m[0xD720+128*i]))
       assert sp[0]==int(visible),('D700 visible sprite count',kind,shield,p,i,sp[0],visible)
       if visible:assert tuple(sp[1:3])==tuple(m[0xD722+128*i:0xD724+128*i]),('D700 SAT position',kind,shield,p,i,list(sp)[:4],bytes(m[0xD722+128*i:0xD727+128*i]).hex())
 
     assert lib.pools_count(0)==7
     images+=2
    print('Mixed pools',kind,shield,boss,'7 C800 +8 D700 prefixes,360 images/shared SAT PASS',flush=True);cases.append((kind,shield,boss))
 # C800 pickup occurs before D700 contact, including an immediately active guard.
 for kind in (1,2,3,8):
  h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
  cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap);lib.candle_direct(kind if kind!=1 else 2,80,128)
  #26 can carry an arbitrary object;1 is a declared constructor-input fixture.
  h.cpu.c=38;h.cpu.de=0x5890;h.call(0x5F1D);m[0xC81F]=kind
  if kind==1:
   # Update the carrier input of the native test object, not ROM code.
   lib.pools_set_kind(1)
  h.cpu.a=rom[0x608E:0x60BD].index(3);h.cpu.bc=0x5878;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
  lib.pools_spawn(3,(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]));lib.pools_contact_enable()
  for p in range(180):
   m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC427]=88;m[0xC425]=144;h.sounds.clear()
   for a in (0x75C0,0x75E2,0x98E5,0x9E31,0x7E67,0x7EE4):h.call(a)
   lib.pools_contact_step()
   from verify_item_trace import val
   values=val(lib);assert values[4]==(m[0xC417]>>4)*10+(m[0xC417]&15) and values[8]==m[0xC415] and values[9]==m[0xC441],('two-pool contact Simon',kind,p,values[4:10],m[0xC415])
   raw=(C.c_ubyte*32)();lib.item_enemy_prefix(0,raw);assert bytes(raw)==bytes(m[0xC800:0xC820]),('two-pool contact C800',kind,p)
   sp=(C.c_int*25)();lib.pools_record(0,raw,sp);assert bytes(raw)==bytes(m[0xD700:0xD720]),('two-pool contact D700',kind,p,bytes(raw).hex(),bytes(m[0xD700:0xD720]).hex())
   sounds=(C.c_ubyte*16)();n=lib.pools_sounds(sounds);assert list(sounds)[:n]==h.sounds,('contact sounds',kind,p,list(sounds)[:n],h.sounds);images+=2
  print('C800 pickup then D700 contact',kind,'360 images raw32/Simon/inventory/sounds PASS',flush=True);cases.append(('two-pool contact',kind))
 # Cross7806: collect airborne loot, leave death flames, clear D700; no CF40/R roll.
 h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
 cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
 h.cpu.c=11;h.cpu.de=0x5890;h.call(0x5F1D);lib.pools_hopper(0,(C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]))
 h.cpu.c=38;h.cpu.de=0x8850;h.call(0x5F1D);m[0xC89F]=2;lib.candle_direct(2,128,64);lib.item_enemy_metadata(1,(C.c_ubyte*32).from_buffer_copy(m[0xC880:0xC8A0]))
 h.cpu.a=rom[0x608E:0x60BD].index(3);h.cpu.bc=0x5878;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D);lib.pools_spawn(3,(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]))
 lib.pools_contact_enable();h.sounds.clear();h.cpu.a=5;h.call(0x8D2C);lib.pools_cross()
 sounds=(C.c_ubyte*16)();n=lib.pools_sounds(sounds);assert list(sounds)[:n]==h.sounds,('cross sounds',list(sounds)[:n],h.sounds)
 from verify_item_trace import val
 assert val(lib)[4]==(m[0xC417]>>4)*10+(m[0xC417]&15) and lib.pools_kills()==m[0xCF40]
 for p in range(180):
  for i in range(7):
   raw=(C.c_ubyte*32)();lib.pools_c800_record(i,raw);assert bytes(raw)==bytes(m[0xC800+128*i:0xC820+128*i]),('cross C80032',p,i,bytes(raw).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
  sp=(C.c_int*25)();lib.pools_record(0,raw,sp);assert bytes(raw)==bytes(m[0xD700:0xD720]),('cross D70032',p,bytes(raw).hex(),bytes(m[0xD700:0xD720]).hex())
  m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC427]=0;m[0xC425]=32;h.call(0x98E5);h.call(0x9E31);lib.pools_step(1);images+=2
 print('Cross:loot pickup/death flame/D700 clear,360 images all prefixes and sounds PASS',flush=True);cases.append(('cross',))
 # Ordinary C800 AI must be revisited when9B90 moves IX to an earlier hole.
 h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
 cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap);lib.candle_busy(1)
 h.cpu.c=30;h.cpu.de=0x1840;h.call(0x5F1D);m[0xC80C]=255;lib.item_enemy_metadata(0,(C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]))
 h.cpu.c=11;h.cpu.de=0x5890;h.call(0x5F1D);m[0xC881]=2;m[0xC886]=0;m[0xC887:0xC88B]=bytes(4);m[0xC890]=64
 lib.pools_hopper(1,(C.c_ubyte*32).from_buffer_copy(m[0xC880:0xC8A0]))
 h.cpu.b=1;h.cpu.de=0x6830;h.call(0x898F);lib.candle_direct(1,104,48);lib.item_enemy_metadata(2,(C.c_ubyte*32).from_buffer_copy(m[0xC900:0xC920]))
 for p in range(180):
  if p==8:h.cpu.ix=0xC800;h.call(0x99F6);lib.candle_free(0)
  m[0xD010]=int(p>=24);m[0xC440]=255;m[0xC003]=(p+1)&255;m[0xC427]=0;m[0xC425]=32
  h.call(0x98E5);h.call(0x9E31);lib.pools_step(int(p>=24))
  for i in range(7):
   raw=(C.c_ubyte*32)();lib.pools_c800_record(i,raw)
   assert bytes(raw)==bytes(m[0xC800+128*i:0xC820+128*i]),('ordinary AI revisit32',p,i,bytes(raw).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
  images+=2
 print('Ordinary hopper AI revisit after heart reallocation,360 images all C80032 PASS',flush=True);cases.append(('AI-revisit',))
 # Failed8-entry allocation after a dragon leaves six SAT reservations behind.
 h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5)
 cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
 for n in range(2):
  h.cpu.c=14;h.cpu.de=0x5840;h.call(0x5F1D);result=lib.pools_sat_alloc(14,88,64)
  assert bool(m[0xCF31])==(result>=0),('SAT dragon allocation',n,result,m[0xCF31])
 assert lib.pools_orphans()==6
 h.cpu.c=30;h.cpu.de=0x5840;h.call(0x5F1D);assert not m[0xCF31] and lib.pools_sat_alloc(30,88,64)==-1
 for p in range(180):
  if p==40:
   h.cpu.ix=0xC800;h.call(0x99F6);lib.candle_free(0)
   for j in range(5):
    h.cpu.c=30;h.cpu.de=0x5840;h.call(0x5F1D);result=lib.pools_sat_alloc(30,88,64)
    assert bool(m[0xCF31])==(result>=0),('orphan capacity after free',j,result,m[0xCF31])
    if result>=0:m[0xC80C+128*result]=255
  m[0xD010]=1;m[0xC440]=255;m[0xC003]=(p+1)&255;h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
  assert lib.pools_count(0)==sum(bool(m[0xC800+128*i]) for i in range(7)),('SAT orphan lifecycle',p)
  images+=2
 print('Partial SAT reservations/release/lifecycle,360 images PASS',flush=True);cases.append(('SAT-partial',))
 (ROOT/'research/mixed-pools-trace.json').write_text(json.dumps({'images':images,'cases':cases},indent=2)+'\n')
if __name__=='__main__':main()
