#!/usr/bin/env python3
"""Shared stage15 live-body reservations, controlled frozen boss constructors."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup
from verify_candle_loot_trace import inventory_prefix

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());cases=[]
 lib.boss15_pools_begin.argtypes=[C.POINTER(C.c_ubyte)]
 for freed in (-1,0,1,2):
  for pickup in (False,True):
   h=setup(rom,15,9);m=h.cpu.memory;m[0xCE00]=5;m[0xC418]=128;m[0xC002]=64;m[0xD010]=1;h.call(0x63C5)
   for tick in range(4):
    m[0xC003]=tick+1;m[0xC427]=88;m[0xC425]=176
    for fn in (0xB6AB,0x98E5,0x9E31,0x9918,0x64EC,0x64E5):h.call(fn)
   cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.boss15_pools_begin(cmap)
   assert lib.boss15_reserve(0,12)==1
   for i in range(3):assert lib.boss15_reserve(i+1,2)==1
   if freed>=0:
    h.cpu.ix=0xD700+128*freed;h.call(0x99F6);assert lib.boss15_reserve(freed+1,0)==1
   expected=sum((m[0xD638+4*j]!=224)<<j for j in range(18))
   assert lib.pools_mask()==expected,('initial ownership',freed,hex(lib.pools_mask()),hex(expected))
   m[0xC427]=20;m[0xC425]=114;m[0xC420]=0;m[0xC42C]=0;m[0xC416]=1;m[0xC422]=3
   for fn in (0x999F,0x7D68,0x8671):h.call(fn)
   lib.candle_hit(0)
   # Imported preserved bytes from the actual constructor, as other loot proofs.
   for i in range(1,7):lib.item_enemy_metadata(i,(C.c_ubyte*32).from_buffer_copy(m[0xC800+128*i:0xC820+128*i]))
   for p in range(181):
    for i in range(1,7):
     raw=(C.c_ubyte*32)();lib.item_enemy_prefix(i,raw)
     assert bytes(raw)==bytes(m[0xC800+128*i:0xC820+128*i]),('ordinary IX32',freed,p,i,bytes(raw).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
    raw=(C.c_ubyte*16)();lib.candle_fallback(raw)
    if freed!=2:assert bytes(raw)==bytes(m[0xC500:0xC510]),('fallback16',freed,p,bytes(raw).hex(),bytes(m[0xC500:0xC510]).hex())
    else:
     # Landed C500 is represented by VKDrop, unlike failed-allocation raw16.
     drops=(C.c_int*44)();unused=(C.c_ubyte*32)();lib.loot_actor_record(unused,drops)
     for j in range(8):
      a=0xC500+16*j
      assert bool(drops[4+5*j])==bool(m[a]),('floor allocation',freed,p,j)
      if m[a]:assert list(drops[5+5*j:9+5*j])==[m[a+2],m[a+1],m[a+4],m[a+6]],('floor fields',freed,p,j,list(drops[5+5*j:9+5*j]),bytes(m[a:a+16]).hex())
    inventory_prefix(lib,m,(freed,pickup,p))
    if m[0xC880] in (30,36,38):
     h.cpu.ix=0xC880;h.call(0x6445);sp=(C.c_int*25)();lib.boss15_sprites(1,sp)
     assert sp[0]==1 and tuple(sp[1:3])==tuple(m[0xC8A2:0xC8A4]),('6445',freed,p,list(sp)[:4])
    if p==180:break
    sx,sy=(0,32)
    if pickup and p>=20:
     if m[0xC880] in (36,38):sx,sy=m[0xC885],m[0xC883]
     elif m[0xC500]&128:sx,sy=m[0xC502]+8,m[0xC501]+18
    m[0xC427]=sx;m[0xC425]=sy;m[0xC420]=0;m[0xC42D]=0;m[0xC422]=0;m[0xC43A]=255;m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xD010]=1;h.sounds.clear()
    for fn in (0x98E5,0x9E31,0x7E67,0x7FE2,0x8A4A):h.call(fn)
    lib.candle_step(sx,sy,0,255,0,0)
    extra=(C.c_int*6)();raw32=(C.c_ubyte*32)();lib.candle_record(1,raw32,extra)
    assert h.sounds==([extra[5]] if extra[5] else []),('sounds',freed,p,h.sounds,extra[5])
   cases.append({'freed_shot':freed,'pickup':pickup,'images':360,'ordinary_C800_slots_compared':[1,2,3,4,5,6],'C500_comparison':'raw16 fallback' if freed!=2 else 'VKDrop allocation,x,y,kind,counter'})
   print('Boss15 shared pool',freed,pickup,'360 images PASS',flush=True)
 (ROOT/'research/boss15-pools-trace.json').write_text(json.dumps(cases,indent=2)+'\n')
if __name__=='__main__':main()
