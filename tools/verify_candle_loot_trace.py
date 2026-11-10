#!/usr/bin/env python3
"""LOT13 connected real candle whip -> C800 reservations -> air pickup / C500."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup,val
from castle_room_objects import decode_room
from decode_room_links import decode

def inventory_prefix(lib,m,label):
 expected=[m[0xC419],m[0xC431],m[0xC701],m[0xC702],(m[0xC417]>>4)*10+(m[0xC417]&15),m[0xC700],m[0xC416],m[0xC70F],m[0xC415],m[0xC441]]
 assert val(lib)[:10]==expected,('candle inventory/Simon',label,val(lib)[:10],expected)

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());cases=[];images=0
 lib.candle_begin.argtypes=[C.POINTER(C.c_ubyte),C.c_int,C.c_int,C.c_int]
 for stage in range(19):
  for room in range(len(decode(rom)[stage]['rooms'])):
   objects=decode_room(rom,stage,room)
   for index,ob in enumerate(objects['candles']):
    b=bytes.fromhex(ob['raw']);kind=b[5]
    if b[4]>=3 or kind in (0,21) or not 0<kind<31:continue
    # All actual positive late candles, plus already-owned weapon conversion.
    for weapon in ((0,kind-25) if kind>=26 else (0,)):
     for mode in ('floor','air'):
      h=setup(rom,stage,room);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128);m[0xC416]=weapon
      cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.candle_begin(cmap,stage,room,weapon)
      flag=m[int(ob['slot'],16)+7]*256+m[int(ob['slot'],16)+8]
      # Real7D68 candle hit; Simon/whip coordinates are declared test inputs.
      m[0xC427]=(b[2]+28 if b[2]<12 else b[2]-12)&255;m[0xC425]=(b[1]+18)&255;m[0xC422]=3;m[0xC42C]=int(b[2]<12);m[0xC420]=0
      h.call(0x7D68);h.call(0x8671);lib.candle_hit(index);assert not m[int(ob['slot'],16)],('hit target',stage,room,index)
      for p in range(181):
       raw=(C.c_ubyte*32)();o=(C.c_int*6)();lib.candle_record(0,raw,o)
       assert bytes(raw)==bytes(m[0xC800:0xC820]),('candle32',stage,room,index,weapon,mode,p,bytes(raw).hex(),bytes(m[0xC800:0xC820]).hex())
       assert bool(o[1]&(1<<index))==(m[flag]==0),('stream flag',stage,room,index,p)
       if p==0:assert o[2]==0 and not any(m[0xC500:0xC580]),'successful C800 creation must not allocate C500'
       if m[0xC800] in (30,36,38):
        h.cpu.ix=0xC800;h.call(0x6445);sp=(C.c_int*25)();lib.loot_actor_sprites(sp)
        assert sp[0]==1 and tuple(sp[1:3])==tuple(m[0xC822:0xC824]),('candle SAT',stage,room,index,mode,p,list(sp)[:4],bytes(m[0xC822:0xC827]).hex())
        if m[0xC800]==38:assert (8,8,1,14)[sp[3]]==m[0xC825]&15,('flame colour',stage,room,index,p,list(sp)[:4],m[0xC825])

       assert o[4]==(m[0xC417]>>4)*10+(m[0xC417]&15),('hearts',stage,room,index,mode,p,o[4],m[0xC417])
       inventory_prefix(lib,m,(stage,room,index,weapon,mode,p))
       floor_raw=(C.c_ubyte*32)();drops=(C.c_int*44)();lib.loot_actor_record(floor_raw,drops)
       for j in range(8):
        a=0xC500+16*j
        assert bool(drops[4+5*j])==bool(m[a]),('candle C500 allocation',stage,room,index,mode,p,j,drops[4+5*j],m[a])
        if m[a]:assert list(drops[5+5*j:9+5*j])==[m[a+2],m[a+1],m[a+4],m[a+6]],('candle C500 fields',stage,room,index,mode,p,j,list(drops[5+5*j:9+5*j]),bytes(m[a:a+16]).hex())
       if p==180:break
       sx,sy=(m[0xC805],m[0xC803]) if mode=='air' and p>=4 and m[0xC800] in (36,38) else (0,32)
       m[0xC427]=sx;m[0xC425]=sy;m[0xC420]=0;m[0xC42D]=0;m[0xC422]=0;m[0xC43A]=0;m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xD010]=1;h.sounds.clear()
       for a in (0x98E5,0x7E67,0x7FE2,0x8A4A):h.call(a)
       lib.candle_step(sx,sy,0,0,0,0)
       lib.candle_record(0,raw,o);assert h.sounds==([o[5]] if o[5] else []),('sounds',stage,room,index,mode,p,h.sounds,o[5]);images+=2
      cases.append((stage,room,index,kind,weapon,mode));print('Candle loot',cases[-1],'360 images IX32/persistence/hearts/sounds PASS',flush=True)
 # Guarded air contacts and global-clock offsets from real89BF births.
 for kind in (1,2):
  for mode in ('guard','recoil','invuln','attack'):
   for base in (15,31):
    h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128);m[0xC003]=base
    cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.candle_begin(cmap,12,0,0);lib.candle_clock(base)
    h.cpu.b=kind;h.cpu.de=0x5840;h.call(0x898F);lib.candle_direct(kind,88,64)
    for p in range(180):
     sx,sy=(m[0xC805],m[0xC803]) if p>=4 and m[0xC800] in (36,38) else (0,32)
     state=5 if mode=='recoil' and p<40 else 0;guard=20 if mode=='guard' and p<40 else 0;invuln=20 if mode=='invuln' and p<40 else 0;attack=3 if mode=='attack' and p<40 else 0
     m[0xC427]=sx;m[0xC425]=sy;m[0xC420]=state;m[0xC43A]=guard;m[0xC42D]=invuln;m[0xC422]=attack;m[0xC003]=(base+p+1)&255;m[0xC440]=255;m[0xD010]=1;h.sounds.clear()
     for a in (0x98E5,0x7E67,0x7FE2,0x8A4A):h.call(a)
     lib.candle_step(sx,sy,state,guard,invuln,attack)
     q=(C.c_ubyte*32)();o=(C.c_int*6)();lib.candle_record(0,q,o)
     assert bytes(q)==bytes(m[0xC800:0xC820]),('candle guards32',kind,mode,base,p,bytes(q).hex(),bytes(m[0xC800:0xC820]).hex())
     inventory_prefix(lib,m,('guard',kind,mode,base,p));assert h.sounds==([o[5]] if o[5] else []),('guard sound',kind,mode,base,p,h.sounds,o[5]);images+=2
    print('Candle guards',kind,mode,base,'360 images IX32/Simon/inventory/sound PASS',flush=True);cases.append(('guard',kind,mode,base))
 # 9B90 allocates the first free C800 slot, not necessarily the old flame.
 h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
 cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.candle_begin(cmap,12,0,0);lib.candle_busy(3)
 for i in range(3):
  h.cpu.c=30;h.cpu.d=24+24*i;h.cpu.e=64;h.call(0x5F1D);m[0xC80C+128*i]=255
  lib.item_enemy_metadata(i,(C.c_ubyte*32).from_buffer_copy(m[0xC800+128*i:0xC820+128*i]))
 h.cpu.b=1;h.cpu.de=0x5840;h.call(0x8992);lib.candle_drop(1,88,64)
 moved=False
 for p in range(180):
  if p==8:h.cpu.ix=0xC800;h.call(0x99F6);lib.candle_free(0)
  m[0xC427]=0;m[0xC425]=32;m[0xC420]=0;m[0xC422]=0;m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xD010]=1
  for a in (0x98E5,0x7E67,0x7FE2,0x8A4A):h.call(a)
  lib.candle_step(0,32,0,0,0,0)
  for i in range(7):
   q=(C.c_ubyte*32)();lib.item_enemy_prefix(i,q)
   assert bytes(q)==bytes(m[0xC800+128*i:0xC820+128*i]),('reallocated heart32',p,i,bytes(q).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
  moved=moved or m[0xC800]==36;images+=2
 assert moved,'heart did not reuse earlier hole'
 print('Candle heart reallocation into earlier hole,360 images/all7 C80032 PASS',flush=True);cases.append(('first-free',))
 # Real allocation failure: seven live C8001E records, no C800 slot available.
 for failure in ('pool','SAT'):
  for kind in (1,2,3,19,26):
   for mode in ('floor','air'):
    h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
    cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.candle_begin(cmap,12,0,0);lib.candle_busy(7) if failure=='pool' else lib.candle_sat_busy()
    for i in range(7 if failure=='pool' else 3):
     h.cpu.c=30 if failure=='pool' else (14,11,30)[i];h.cpu.d=24+24*i if failure=='pool' else (24,88,144)[i];h.cpu.e=64;h.call(0x5F1D)
     if failure=='pool' or i==2:m[0xC80C+128*i]=255
     lib.item_enemy_metadata(i,(C.c_ubyte*32).from_buffer_copy(m[0xC800+128*i:0xC820+128*i]))
    h.cpu.b=kind;h.cpu.d=88;h.cpu.e=64;h.call(0x8992);lib.candle_drop(kind,88,64)
    assert m[0xC500]==1,'failed C800 allocation must create C500 fallback'
    for p in range(181):
     raw=(C.c_ubyte*16)();lib.candle_fallback(raw)
     assert bytes(raw)==bytes(m[0xC500:0xC510]),('fallback16',failure,kind,mode,p,bytes(raw).hex(),bytes(m[0xC500:0xC510]).hex())
     for i in range(7):
      q=(C.c_ubyte*32)();lib.item_enemy_prefix(i,q)
      assert bytes(q)==bytes(m[0xC800+128*i:0xC820+128*i]),('busy32',kind,mode,p,i,bytes(q).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
     if p==180:break
     sx,sy=(m[0xC502]+8,m[0xC501]+18) if mode=='air' and p>=20 else (0,32)
     m[0xC43A]=255 if failure=='SAT' else 0
     m[0xC427]=sx&255;m[0xC425]=sy&255;m[0xC420]=0;m[0xC42D]=0;m[0xC422]=0;m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xD010]=1;h.sounds.clear();m[0xD628]=208
     for a in (0x98E5,0x7E67,0x7FE2,0x8A4A):h.call(a)
     lib.candle_step(sx&255,sy&255,0,255 if failure=='SAT' else 0,0,0)
     inventory_prefix(lib,m,('fallback',failure,kind,mode,p))
     q=(C.c_ubyte*32)();ev=(C.c_int*6)();lib.candle_record(0,q,ev)
     assert h.sounds==([ev[5]] if ev[5] else []),('fallback sound',failure,kind,mode,p,h.sounds,ev[5])
     sp=(C.c_ubyte*4)();visible=lib.candle_fallback_sprite(sp)
     if visible:assert bytes(sp)==bytes([m[0xD629],m[0xD628],m[0xD62A],m[0xD4A0]]),('fallback sprite',kind,mode,p,list(sp),bytes(m[0xD628:0xD62B]).hex(),m[0xD4A0])
     elif m[0xC500] and m[0xC500]&15!=4:assert m[0xD628]>=208,('unexpected visible fallback',kind,mode,p,m[0xD628])
     images+=2
    print('Fallback',failure,kind,mode,'360 images C50016/all7 C80032/SAT PASS',flush=True);cases.append(('fallback',failure,kind,mode))
 # Both pools full: the candle is lost, no invented replacement object.
 h=setup(rom,12,0);m=h.cpu.memory;h.call(0x63C5);m[0xC500:0xC580]=bytes(128)
 cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.candle_begin(cmap,12,0,0);lib.candle_busy(7);lib.candle_fill_floor()
 for i in range(7):
  h.cpu.c=30;h.cpu.d=24+24*i;h.cpu.e=64;h.call(0x5F1D);m[0xC80C+128*i]=255
  lib.item_enemy_metadata(i,(C.c_ubyte*32).from_buffer_copy(m[0xC800+128*i:0xC820+128*i]))
 for i in range(8):m[0xC500+16*i:0xC510+16*i]=bytes((0x84,176,24*i,0,23,255,255,0,0,0,0,0,0,0,0,0))
 h.cpu.b=2;h.cpu.de=0x5840;h.call(0x898F);lib.candle_drop(2,88,64)
 for p in range(180):
  m[0xC427]=0;m[0xC425]=32;m[0xC420]=0;m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xD010]=1
  for a in (0x98E5,0x7E67,0x7FE2,0x8A4A):h.call(a)
  lib.candle_step(0,32,0,0,0,0);q=(C.c_ubyte*32)();o=(C.c_int*44)();lib.loot_actor_record(q,o)
  for i in range(8):assert list(o[4+5*i:9+5*i])==[1,24*i,176,23,255],('full C500',p,i,list(o[4+5*i:9+5*i]))
  for i in range(7):
   lib.item_enemy_prefix(i,q);assert bytes(q)==bytes(m[0xC800+128*i:0xC820+128*i]),('both pools full C800',p,i)
  images+=2
 print('Both C800/C500 full,360 images,unchanged reservations/no invented drop PASS',flush=True);cases.append(('both-full',))
 lib.candle_end()
 (ROOT/'research/candle-loot-trace.json').write_text(json.dumps({'images':images,'cases':cases},indent=2)+'\n')
 print('Candle loot',len(cases),'scenarios',images,'images PASS')
if __name__=='__main__':main()
