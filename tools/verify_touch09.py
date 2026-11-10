#!/usr/bin/env python3
"""Object09 simultaneous pool contacts, score, sound and Simon colours vs ROM."""
import ctypes as C,json,sys,itertools
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup,val

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom());rows=[]
 cases=[(1,3,0)]+[x for x in itertools.product((0,1,2),(0,3,4,9),(0,17)) if x!=(1,3,0)]
 if len(sys.argv)>2:cases=[tuple(json.loads(sys.argv[2]))]
 for bodies,kind,guard in cases:
  h=setup(rom,13,3);m=h.cpu.memory;m[0xC002]=64;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap);m[0xC427]=88;m[0xC425]=172
  for i in range(bodies):
   h.cpu.c=9;h.cpu.de=0x58AC;h.call(0x5F1D);a=0xC800+128*i;m[a+17]=1;h.cpu.ix=a;h.call(0xAF5F);lib.touch09_body((C.c_ubyte*32).from_buffer_copy(m[a:a+32]))
  if kind:
   h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=0x5894;h.cpu.de=0;h.cpu.hl=0;h.call(0x9F6D);assert lib.pools_spawn(kind,(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]))==0
  h.cpu.a=9;h.call(0x8D30);m[0xC43A]=guard;lib.touch09_begin(guard)
  for frame in range(360):
   h.sounds.clear();m[0xD010]=1;m[0xC440]=255
   if frame%2==0:
    m[0xC003]=(frame//2+1)&255
    for a in (0x75C0,0x75E2,0x98E5,0x9E31):h.call(a)
   h.call(0x7E67);h.call(0x7EE4);lib.touch09_step()
   score=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(m[0xC405:0xC408]))
   assert lib.touch09_points()==score,('points',bodies,kind,guard,frame,lib.touch09_points(),score)
   sounds=(C.c_ubyte*8)();n=lib.touch09_sounds(sounds);assert list(sounds)[:n]==h.sounds,('sounds',bodies,kind,guard,frame,list(sounds)[:n],h.sounds)
   for i in range(7):
    b=(C.c_ubyte*32)();lib.pools_c800_record(i,b);assert bytes(b)==bytes(m[0xC800+128*i:0xC820+128*i]),('C80032',bodies,kind,guard,frame,i)
   for i in range(8):
    b=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.pools_record(i,b,sp);assert bytes(b)==bytes(m[0xD700+128*i:0xD720+128*i]),('D70032',bodies,kind,guard,frame,i)
   for i in range(bodies):
    a=0xC800+128*i;h.cpu.ix=a;h.call(0x6445);sp=(C.c_int*25)();lib.touch09_sprites(i,sp)
    visible=[tuple(m[a+34+5*k:a+37+5*k]) for k in range(m[a+32]) if m[a+37+5*k]&15]
    assert [tuple(sp[1+3*k:4+3*k]) for k in range(sp[0])]==visible,('body sprites6445',bodies,kind,guard,frame,i)
   assert lib.pools_kills()==m[0xCF40]==0,('CF40 retained for enemy09',bodies,kind,guard,frame)
   assert val(lib)[8]==m[0xC415] and val(lib)[12]==m[0xC434] and val(lib)[11]==m[0xC43A],('Simon/timers',bodies,kind,guard,frame,val(lib)[8:14],m[0xC415],m[0xC434],m[0xC43A])
   if frame%2==0:
    h.call(0x790C);colour=val(lib)[14];expected=bytes([colour])*128 if colour else bytes([1 if (i//16)%2==0 else 0x42 for i in range(128)])
    assert bytes(m[0xD400:0xD480])==expected,('Simon colours',bodies,kind,guard,frame,colour)
  rows.append({'bodies':bodies,'projectile':kind,'guard':guard,'images':360,'points':score})
  print('Touch09',bodies,kind,guard,'360 images/7C800+8D700 prefixes/points/sounds/Simon colours PASS',flush=True)
 (ROOT/'research/touch09-trace.json').write_text(json.dumps(rows,indent=2)+'\n')
if __name__=='__main__':main()
