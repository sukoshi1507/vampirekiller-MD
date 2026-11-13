#!/usr/bin/env python3
"""Stage 18 C800 objects 1Eh flame, 2Ch/2Dh/2Eh shots (2 entries each) versus the real 5F1D / 99F6 / 99E5 / 6445.

openMSX never shows them at SAT Y E0 while alive (research/sat-owners-lot15.md): here the real 6445 is run on every object Y below the kill threshold of
99E5 (E4) for every pose the fight gives them, and no entry ever lands on E0 (a body or flame entry at object Y - 15/16 reaches E0 at object Y 239/240
only), so the references stay reserved from 5F1D to the kill, which is what actors.c dracula_sat_sync holds through Enemies_boss15_reserve (slots
0, 9, 10..14 = C800[0..6]).  Allocation order, release by 99F6 and reuse of the references are compared with the port's allocator."""
import ctypes as C,sys
from pathlib import Path
from probe import load_rom,reference_rom
from verify_item_trace import setup

def zmask(m):return sum((m[0xD638+4*j]!=224)<<j for j in range(18))
SLOT=(0,9,10,11,12,13,14)
POSES={0x2C:(2,),0x2D:(0x57,0x59),0x2E:(0x5A,),0x1E:(0x85,0x86)}

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom())
 # 1. no entry at E0 while the object lives (Y < E4, the 99E5 limit), every pose
 for t,poses in POSES.items():
  for pose in poses:
   h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);h.cpu.c=t;h.cpu.de=0x8060;h.call(0x5F1D);assert m[0xC800]==t and m[0xC820]==2
   for y in range(228):
    m[0xC803]=y;m[0xC80B]=pose;h.cpu.ix=0xC800;h.call(0x6445)
    assert all(m[0xC822+5*i]!=0xE0 for i in range(2)),('entry at E0 while alive',hex(t),hex(pose),y)
   m[0xC800]=t;m[0xC803]=227;h.cpu.ix=0xC800;h.call(0x99E5);assert m[0xC800]==t
   m[0xC803]=228;h.cpu.ix=0xC800;h.call(0x99E5);assert m[0xC800]==0,('99E5 kill threshold',hex(t),m[0xC800])
  print('stage 18 object %02X poses %s: no E0 for object Y 0..227, 99E5 kills at E4 PASS'%(t,[hex(p) for p in poses]),flush=True)
 # 2. allocation, kill, rebuild as a flame, reuse
 h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
 types=(0x11,0x2C,0x2D,0x2E)
 for k,t in enumerate(types):
  h.cpu.c=t;h.cpu.de=0x8060;h.call(0x5F1D);assert m[0xC800+128*k]==t,(k,t)
  assert lib.boss15_reserve(SLOT[k],m[0xC820+128*k])==1
  assert lib.pools_mask()==zmask(m),('alloc',k,hex(t),hex(lib.pools_mask()),hex(zmask(m)))
 # 2Dh in block 2 is killed: 99F6 (kill), port releases; a flame 1Eh (death_objects) then takes the lowest free references
 h.cpu.ix=0xC800+128*2;h.call(0x99F6);assert lib.boss15_reserve(SLOT[2],0)==1
 assert lib.pools_mask()==zmask(m),('killed',hex(lib.pools_mask()),hex(zmask(m)))
 h.cpu.c=0x1E;h.cpu.de=0x8040;h.call(0x5F1D);assert m[0xC800+128*2]==0x1E
 assert lib.boss15_reserve(SLOT[2],2)==1
 assert lib.pools_mask()==zmask(m),('flame',hex(lib.pools_mask()),hex(zmask(m)))
 print('stage 18 objects 11h/2Ch/2Dh/2Eh + flame 1Eh: 5F1D order, 99F6, reuse of the references PASS',flush=True)
if __name__=='__main__':main()
