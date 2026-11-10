#!/usr/bin/env python3
"""Executed renderer/allocator alias cases; controlled constructor positions."""
import ctypes as C,sys,json
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)]
 rom=load_rom(reference_rom());cases=[]
 for mode,pose,y in ((0,0x85,240),(0,0x86,239),(1,0x85,240),(1,0x86,239)):
  h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);lib.pools_begin((C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]))
  h.cpu.c=30;h.cpu.de=(88<<8)|y;h.call(0x5F1D);m[0xC80B]=pose;m[0xC80C]=255
  assert lib.pools_sat_alloc(30,88,y)==0;lib.pools_flame_pose(0,y,pose)
  lib.item_enemy_metadata(0,(C.c_ubyte*32).from_buffer_copy(m[0xC800:0xC820]))
  # Native constructor input counters/pose match the actual5F1D outputs.
  for p in range(360):
   h.cpu.ix=0xC800;h.call(0x6445)
   if m[0xC880]:h.cpu.ix=0xC880;h.call(0x6445)
   h.call(0x64EC);h.call(0x64E5);lib.pools_render()
   expected=sum((m[0xD638+4*j]!=0xE0)<<j for j in range(18))
   assert lib.pools_mask()==expected,('render mask',pose,p,hex(lib.pools_mask()),hex(expected),list(m[0xC800:0xC830]),list(m[0xC880:0xC8B0]))
   if p==0:
    assert expected==2,('visible layer free, colour-zero layer E1',expected)
    h.cpu.c=30;h.cpu.de=0x5840;h.call(0x5F1D);assert lib.pools_sat_alloc(30,88,64)==1
    refs=[m[0xC8A1],m[0xC8A6]];assert refs==[0,2],refs
    assert lib.pools_owner(1)==5,'second object reuses active first SAT'
    m[0xC88C]=255;lib.item_enemy_metadata(1,(C.c_ubyte*32).from_buffer_copy(m[0xC880:0xC8A0]))
   # Hold constructors still: render/allocator proof, not an AI trajectory.
   for i in range(2):
    raw=(C.c_ubyte*32)();lib.item_enemy_prefix(i,raw)
    assert bytes(raw)==bytes(m[0xC800+128*i:0xC820+128*i]),('IX32',pose,p,i,bytes(raw).hex(),bytes(m[0xC800+128*i:0xC820+128*i]).hex())
   if m[0xC800]:
    sp=(C.c_int*25)();lib.loot_actor_sprites(sp)
    assert sp[0]==1 and tuple(sp[1:3])==tuple(m[0xC822:0xC824]) and sp[3]+0xF4==m[0xC824],('sprite6445',mode,pose,p,list(sp)[:4])
   from verify_item_trace import val
   assert val(lib)[8]==m[0xC415],('Simon energy',mode,pose,p)
   assert not h.sounds
   if mode:
    m[0xC003]=(p+1)&255;m[0xD010]=1;m[0xC440]=255;m[0xC43A]=255;m[0xC427]=0;m[0xC425]=32
    h.call(0x98E5);h.call(0x9E31);lib.pools_step(1)
  cases.append({'pose':pose,'AI':bool(mode),'render_passes':360,'scope':'constructor position inputs; actual98E5/9E31 and720 images when AI=true, static renderer when false'})
  print('SAT alias',hex(pose),'AI',mode,'360 render passes, all IX32, active-entry reuse PASS',flush=True)
 (ROOT/'research/sat-alias-trace.json').write_text(json.dumps(cases,indent=2)+'\n')
if __name__=='__main__':main()
