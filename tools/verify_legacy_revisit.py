#!/usr/bin/env python3
"""Real98E5 IX rewind across panther/fish3; full C800 prefixes and sprites."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup,val

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom());rows=[]
 for kind,stage,room,x,y in ((5,1,2,160,176),(33,10,6,192,176)):
  for moving in ((0,1) if kind==5 else (0,1,2,3)):
   h=setup(rom,stage,room);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
   cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap);lib.candle_busy(1)
   h.cpu.c=30;h.cpu.de=0x1840;h.call(0x5F1D);m[0xC80C]=255;lib.item_enemy_metadata(0,(C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]))
   sx=120 if moving else 0;sy=176 if moving else 32;m[0xC427]=sx;m[0xC425]=sy
   h.cpu.c=kind;h.cpu.de=(x<<8)|y;h.call(0x5F1D)
   if kind==33:m[0xC890]=64 #common walking timer input, no shot before freeze24
   if moving>=2:
    # Common active03 shooting-state fixture from the fixed constructor.
    m[0xC880]=3;m[0xC881]=2;m[0xC886]=0;m[0xC889:0xC88B]=int((-384 if moving==2 else 384)&65535).to_bytes(2,'little');m[0xC891]=24
   lib.legacy_import(kind,1,(C.c_ubyte*32).from_buffer_copy(m[0xC880:0xC8A0]),stage,room)
   h.cpu.b=1;h.cpu.de=0x6830;h.call(0x898F);lib.candle_direct(1,104,48);lib.item_enemy_metadata(2,(C.c_ubyte*32).from_buffer_copy(m[0xC900:0xC920]))
   for p in range(180):
    if p==8:h.cpu.ix=0xC800;h.call(0x99F6);lib.candle_free(0)
    frozen=int(p>=24);m[0xD010]=frozen;m[0xC440]=255;m[0xC43A]=255;m[0xC003]=(p+1)&255;m[0xC427]=sx;m[0xC425]=sy;h.sounds.clear()
    h.call(0x98E5);h.call(0x9E31);lib.legacy_step(frozen,sx,sy)
    for i in range(7):
     raw=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.legacy_record(i,raw,sp);a=0xC800+128*i
     assert bytes(raw)==bytes(m[a:a+32]),('legacy32',kind,moving,p,i,bytes(raw).hex(),bytes(m[a:a+32]).hex())
     if i==1 and m[a]:
      h.cpu.ix=a;h.call(0x6445)
      assert sp[0]>0 and tuple(sp[1:3])==tuple(m[a+34:a+36]),('legacy sprite6445',kind,moving,p,list(sp)[:4])
    for i in range(8):
     raw=(C.c_ubyte*32)();sp=(C.c_int*25)()
     if moving>=2:lib.legacy_shot_record(i,raw)
     else:lib.pools_record(i,raw,sp)
     assert bytes(raw)==bytes(m[0xD700+128*i:0xD720+128*i]),('D70032',kind,moving,p,i)
    assert val(lib)[8]==m[0xC415] and not h.sounds and lib.legacy_sounds()==0
   rows.append({'kind':kind,'moving':moving,'images':360})
   print('Legacy IX revisit',kind,moving,'360 images/7C800+8D700 raw32/SAT positions/Simon/sounds PASS',flush=True)
 #One actual A492 emission in each direction, then explicit time stop.
 for direction in (-1,1):
  h=setup(rom,10,6);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
  h.cpu.c=33;h.cpu.de=0xC0B0;h.call(0x5F1D);h.call(0x98E5);m[0xC801]=2;m[0xC806]=0;m[0xC809:0xC80B]=int(direction*384&65535).to_bytes(2,'little');m[0xC811]=9
  lib.legacy_import(33,0,(C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]),10,6)
  for p in range(180):
   frozen=int(p>=1);m[0xD010]=frozen;m[0xC440]=255;m[0xC43A]=255;m[0xC427]=0;m[0xC425]=32;h.sounds.clear()
   h.call(0x98E5);h.call(0x9E31);lib.legacy_step(frozen,0,32)
   for i in range(7):
    raw=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.legacy_record(i,raw,sp)
    assert bytes(raw)==bytes(m[0xC800+128*i:0xC820+128*i]),('shoot C80032',direction,p,i,bytes(raw).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
   for i in range(8):
    raw=(C.c_ubyte*32)();lib.legacy_shot_record(i,raw)
    assert bytes(raw)==bytes(m[0xD700+128*i:0xD720+128*i]),('shoot D70032',direction,p,i,bytes(raw).hex(),bytes(m[0xD700+128*i:0xD720+128*i]).hex())
   assert val(lib)[8]==m[0xC415] and not h.sounds and lib.legacy_sounds()==0
  rows.append({'shoot_direction':direction,'images':360,'freeze_after_pass':1})
  print('Fish3 full emission',direction,'360 images/7C800+8D700 raw32/Simon/sounds PASS',flush=True)
 (ROOT/'research/legacy-revisit-trace.json').write_text(json.dumps(rows,indent=2)+'\n')
if __name__=='__main__':main()
