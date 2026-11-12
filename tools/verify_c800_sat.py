#!/usr/bin/env python3
"""C800 generated layouts vs real full renderer and frozen99E5."""
import ctypes as C,json,os,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup,val
from build_c800_sat import collect,SPECS

def unused(lib,m,label):
 for i in range(2,7):
  b=(C.c_ubyte*32)();lib.pools_c800_record(i,b)
  assert bytes(b)==bytes(m[0xC800+128*i:0xC820+128*i]),('inactive C80032',label,i)
 for i in range(8):
  b=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.pools_record(i,b,sp)
  assert bytes(b)==bytes(m[0xD700+128*i:0xD720+128*i]),('inactive D70032',label,i)

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom());cases=[]
 only={int(x) for x in os.environ['SAT_RAWS'].split(',')} if os.environ.get('SAT_RAWS') else None   # debugging aid: restrict to some ROM types
 for raw,internal,pose,n,visible,dy in collect():
  if only and raw not in only:continue
  ys=sorted({96}|{(224-dy[i])&255 for i in range(n) if visible&(1<<i)})
  for y in ys:
   # Species outside SPECS (legacy rows): the port renders them from its own pose set and runs its own AI, proven elsewhere; here only the
   # allocation (18-entry mask, references of the next constructor, IX+00..1F round trip) is compared, with the AI frozen.
   legacy=raw not in {x[0] for x in SPECS}
   for ai in ((0,) if legacy else (0,1)):
    h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
    h.cpu.c=raw;h.cpu.de=(128<<8)|y;h.call(0x5F1D)
    if raw==9 and 0x21<=pose<0x25:
     m[0xC811]=1;m[0xC427]=200;h.cpu.ix=0xC800;h.call(0xAF5F)
    m[0xC803]=y;m[0xC80B]=pose
    #Eagle constructor has a real nested flea; remove it explicitly for this parent-only fixture.
    if raw==15:h.cpu.ix=0xC880;h.call(0x99F6)
    #Legacy constructors may also build children (type 02/03 splashes): remove them for this parent-only fixture.
    if legacy:
     for k in range(1,7):
      if m[0xC800+128*k]:
       h.cpu.hl=0xC800+128*k;h.call(0x9A00)   # real 9A00: the child's SAT Y back to E0, then the block is wiped
       m[0xC800+128*k:0xC880+128*k]=bytes(128)
      h.sounds.clear()
    assert lib.c800_layout_import(internal,(C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]))==0
    for p in range(180):
     if m[0xC800]:h.cpu.ix=0xC800;h.call(0x6445)
     if m[0xC880]:h.cpu.ix=0xC880;h.call(0x6445)
     h.call(0x64EC);h.call(0x64E5);lib.pools_render()
     expected=sum((m[0xD638+4*j]!=224)<<j for j in range(18))
     assert lib.pools_mask()==expected,('SAT',raw,pose,y,ai,p,hex(lib.pools_mask()),hex(expected))
     if p==0:
      h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==1
      m[0xC88C]=255;lib.item_enemy_metadata(1,(C.c_ubyte*32).from_buffer_copy(m[0xC880:0xC8A0]))
      refs=(m[0xC8A1],m[0xC8A6]);assert lib.pools_owner(1)==sum(1<<r for r in refs),('reallocated refs',raw,pose,y,refs)
     b=(C.c_ubyte*32)();lib.c800_layout_record(b)
     assert bytes(b)==bytes(m[0xC800:0xC820]),('C80032',raw,pose,y,ai,p,bytes(b).hex(),bytes(m[0xC800:0xC820]).hex())
     lib.item_enemy_prefix(1,b);assert bytes(b)==bytes(m[0xC880:0xC8A0]),('secondary IX32',raw,pose,y,ai,p)
     if m[0xC800] and not legacy:
      sp=(C.c_int*25)();lib.c800_layout_sprites(sp)
      entries=[tuple(m[0xC822+5*k:0xC824+5*k]) for k in range(m[0xC820]) if m[0xC825+5*k]&15]
      stride=2 if len(entries)==2*sp[0] else 1
      assert len(entries)==stride*sp[0],('sprite count',raw,pose,y,ai,p,sp[0],entries)
      for k in range(sp[0]):assert tuple(sp[1+2*k:3+2*k])==entries[stride*k],('sprite6445',raw,pose,y,ai,p,k,list(sp)[:9],entries)
     unused(lib,m,(raw,pose,y,ai,p))
     assert val(lib)[8]==m[0xC415] and not h.sounds and lib.pools_step_sounds()==0
     if ai:
      m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;m[0xC425]=32;m[0xC427]=0;h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
    cases.append({'raw':raw,'pose':pose,'y':y,'AI_loop':bool(ai),'images':360 if ai else None})
    print('C800 SAT',raw,hex(pose),y,ai,'180 renders/frozen AI360 images/SAT/IX32/Simon/sounds PASS',flush=True)
 #Dragon dynamic chain bypasses6445 (RET for raw0E);6501 copies its entries.
 for pattern in range(16):
  for ai in (0,1):
   h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
   h.cpu.c=14;h.cpu.de=0x8060;h.call(0x5F1D);assert lib.c800_dragon_import((C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]))==0
   for i in range(8):m[0xC822+5*i]=224 if pattern&(1<<(i//2)) else 96
   lib.c800_dragon_pattern(pattern)
   for p in range(180):
    if m[0xC880]:h.cpu.ix=0xC880;h.call(0x6445)
    h.call(0x64EC);lib.pools_render();expected=sum((m[0xD638+4*j]!=224)<<j for j in range(18))
    assert lib.pools_mask()==expected,('dragon SAT',pattern,ai,p,hex(lib.pools_mask()),hex(expected))
    if p==0:
     h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==1;m[0xC88C]=255;lib.item_enemy_metadata(1,(C.c_ubyte*32).from_buffer_copy(m[0xC880:0xC8A0]))
     assert lib.pools_owner(1)==(1<<m[0xC8A1])|(1<<m[0xC8A6])
    b=(C.c_ubyte*32)();sp=(C.c_int*25)();lib.c800_dragon_record(b,sp);assert bytes(b)==bytes(m[0xC800:0xC820]),('dragon32',pattern,ai,p,bytes(b).hex(),bytes(m[0xC800:0xC820]).hex())
    assert sp[0]==4
    for i in range(4):assert tuple(sp[1+2*i:3+2*i])==tuple(m[0xC822+10*i:0xC824+10*i]),('dragon sprite',pattern,ai,p,i)
    lib.item_enemy_prefix(1,b);assert bytes(b)==bytes(m[0xC880:0xC8A0])
    unused(lib,m,('dragon',pattern,ai,p))
    assert val(lib)[8]==m[0xC415] and not h.sounds and lib.pools_step_sounds()==0
    if ai:m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
   cases.append({'raw':14,'pattern':pattern,'AI_loop':bool(ai),'images':360 if ai else None})
   print('Dragon dynamic SAT',pattern,ai,'180 renders/frozen AI360 images/IX32/sprites/Simon/sounds PASS',flush=True)
 (ROOT/'research/c800-sat-trace.json').write_text(json.dumps(cases,indent=2)+'\n')
if __name__=='__main__':main()
