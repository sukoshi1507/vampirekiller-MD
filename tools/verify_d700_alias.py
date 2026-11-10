#!/usr/bin/env python3
"""D7000C alias/saturation, actual9F6D/6445/64E5/9E31 vs production."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup,val

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)]
 rom=load_rom(reference_rom());cases=[]
 for boss in (0,1):
  for kind,pose,y in ((12,0x85,240),(12,0x86,239),(4,3,236),(9,3,236)):
   for ai in (0,1):
    h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xCE03]=boss
    lib.pools_begin((C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]));lib.pools_boss(boss)
    # Repeated actual constructors and renders until the shared SAT denies one.
    made=0
    for i in range(8):
     h.cpu.a=255 if kind==12 else rom[0x608E:0x60BD].index(kind);h.cpu.bc=(88<<8)|y;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
     if m[0xD700+128*i]:m[0xD70B+128*i]=pose;m[0xD70C+128*i]=255
     raw=(C.c_ubyte*32).from_buffer_copy(m[0xD700+128*i:0xD720+128*i]);n=lib.pools_spawn(kind,raw)
     assert (n==i)==bool(m[0xD700+128*i]),('allocation',boss,pose,i,n)
     if n<0:break
     made+=1
     for j in range(made):h.cpu.ix=0xD700+128*j;h.call(0x6445)
     h.call(0x64E5);lib.pools_render()
     expected=sum((m[0xD638+4*j]!=0xE0)<<j for j in range(18))
     assert lib.pools_mask()==expected,('allocation SAT',boss,pose,i,hex(lib.pools_mask()),hex(expected))
    assert made==((1 if boss else 7) if kind==12 else 8),('saturated objects',kind,made)
    if made==8:
     before=[bytes(m[0xD700+128*i:0xD720+128*i]) for i in range(8)]
     h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(88<<8)|y;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
     assert lib.pools_spawn(kind,raw)==-1
     assert before==[bytes(m[0xD700+128*i:0xD720+128*i]) for i in range(8)]
    for p in range(180):
     for i in range(8):
      a=0xD700+128*i
      if m[a]:h.cpu.ix=a;h.call(0x6445)
     h.call(0x64E5);lib.pools_render()
     expected=sum((m[0xD638+4*j]!=0xE0)<<j for j in range(18))
     assert lib.pools_mask()==expected,('SAT',boss,pose,ai,p,hex(lib.pools_mask()),hex(expected))
     for i in range(8):
      raw=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.pools_record(i,raw,sp);a=0xD700+128*i
      assert bytes(raw)==bytes(m[a:a+32]),('D70032',boss,pose,ai,p,i,bytes(raw).hex(),bytes(m[a:a+32]).hex())
      if m[a]:
       assert sp[0]==1 and tuple(sp[1:3])==tuple(m[a+0x27:a+0x29]) and (sp[3]+0xF4 if kind==12 else sp[3])==m[a+0x29],('sprite6445',boss,pose,ai,p,i,list(sp)[:4])
     assert val(lib)[8]==m[0xC415] and not h.sounds
     if ai:
      m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;m[0xC427]=0;m[0xC425]=32
      h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
    cases.append({'boss':boss,'type':kind,'pose':pose,'AI':bool(ai),'objects':made,'passes':180,'images':360 if ai else None})
    print('D700 alias',boss,kind,hex(pose),ai,'objects',made,'180passes / AI360 images,8 raw32/SAT/sprites/Simon/sounds PASS',flush=True)
 #992F freezes AI, but9E49/9E4C still renders and applies99E5.
 for kind in (3,4,9):
  for x,y in ((6,144),(7,144),(240,144),(241,144),(96,227),(96,228)):
   h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);lib.pools_begin((C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]))
   h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(x<<8)|y;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
   assert lib.pools_spawn(kind,(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]))==0
   for p in range(180):
    m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;m[0xC427]=0;m[0xC425]=32
    h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
    for i in range(8):
     raw=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.pools_record(i,raw,sp);a=0xD700+128*i
     assert bytes(raw)==bytes(m[a:a+32]),('frozen boundary32',kind,x,y,p,i,bytes(raw).hex(),bytes(m[a:a+32]).hex())
     if m[a]:
      h.cpu.ix=a;h.call(0x6445)
      assert sp[0]>=1 and tuple(sp[1:3])==tuple(m[a+0x22:a+0x24]),('frozen boundary sprite',kind,x,y,p)
    assert bool(m[0xD700])==(x>=7 and x<241 and y<228)
    assert val(lib)[8]==m[0xC415] and not h.sounds
   cases.append({'type':kind,'x':x,'y':y,'frozen':True,'images':360})
   print('D700 frozen bounds',kind,x,y,'360 images/8 raw32/sprites/Simon/sounds PASS',flush=True)
 #All rendered D700 projectile poses: colour-zero vs two coloured layers.
 for kind,poses in ((3,(3,)),(4,(3,0x4B,0x4C,0x4D)),(9,(3,0x63,0x64,0x65,0x66))):
  for pose in poses:
   for ai in (0,1):
    y=236 if pose==3 else 239
    h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);lib.pools_begin((C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]))
    h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(96<<8)|y;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D);m[0xD70B]=pose
    assert lib.pools_spawn(kind,(C.c_ubyte*32).from_buffer_copy(m[0xD700:0xD720]))==0
    for p in range(180):
     if m[0xD700]:h.cpu.ix=0xD700;h.call(0x6445)
     h.call(0x64E5);lib.pools_render()
     expected=sum((m[0xD638+4*j]!=0xE0)<<j for j in range(18))
     assert lib.pools_mask()==expected,('pose SAT',kind,pose,ai,p,hex(lib.pools_mask()),hex(expected))
     for i in range(8):
      raw=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.pools_record(i,raw,sp);a=0xD700+128*i
      assert bytes(raw)==bytes(m[a:a+32]),('pose32',kind,pose,ai,p,i)
      if m[a]:assert sp[0]>=1 and tuple(sp[1:3])==tuple(m[a+0x22:a+0x24]),('pose sprite',kind,pose,ai,p)
     assert val(lib)[8]==m[0xC415] and not h.sounds
     if ai:
      m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;m[0xC427]=0;m[0xC425]=32
      h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
    cases.append({'type':kind,'pose':pose,'AI':bool(ai),'images':360 if ai else None,'render_passes':180})
    print('D700 pose alias',kind,hex(pose),ai,'180passes/AI360 images/8 raw32/SAT/sprites PASS',flush=True)
 (ROOT/'research/d700-alias-trace.json').write_text(json.dumps(cases,indent=2)+'\n')
if __name__=='__main__':main()
