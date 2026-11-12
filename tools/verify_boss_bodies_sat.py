#!/usr/bin/env python3
"""SAT entries held by a boss body (C800[0], 8 entries: 12h stage 3, 13h stage 6; 6057 table) versus the real 5F1D / 99F6 / 9A00.

openMSX (tools/sat_snapshot_extra.tcl, research/sat-owners-lot15.md): 3:5 holds 12h with 8 references 0..7 and 6:5 holds 13h with 8 references
0..7 for the whole fight; CE03 stays 0 in every room of stages 1-18.  Here the real constructor is executed, then a candle flame (1Eh), then the
body is destroyed (99F6) and a second flame is built; the port (Enemies_boss15_reserve(0, n) as the generic body hold) must give the same
18-entry masks and the same references to the flames."""
import ctypes as C,sys
from pathlib import Path
from probe import load_rom,reference_rom
from verify_item_trace import setup

def zmask(m):return sum((m[0xD638+4*j]!=224)<<j for j in range(18))

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom())
 for raw,count in ((0x12,8),(0x13,8),(0x11,8)):
  h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
  h.cpu.c=raw;h.cpu.de=0x8060;h.call(0x5F1D);assert m[0xC800]==raw and m[0xC820]==count,(raw,m[0xC800],m[0xC820])
  assert lib.boss15_reserve(0,count)==1
  assert lib.pools_mask()==zmask(m)==(1<<count)-1,('body',hex(raw),hex(lib.pools_mask()),hex(zmask(m)))
  for rnd in (0,1):
   h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);k=1+rnd;assert m[0xC800+128*k]==30
   assert lib.pools_sat_alloc(30,128,64)==k,(raw,rnd)
   refs=(m[0xC800+128*k+0x21],m[0xC800+128*k+0x26])
   assert lib.pools_owner(k)==sum(1<<r for r in refs),('flame refs',hex(raw),rnd,refs,hex(lib.pools_owner(k)))
   assert lib.pools_mask()==zmask(m),('flame mask',hex(raw),rnd,hex(lib.pools_mask()),hex(zmask(m)))
  # Death of the body: the real 99F6 (entries back to Y=E0), then the port releases the hold.
  h.cpu.ix=0xC800;h.call(0x99F6);assert lib.boss15_reserve(0,0)==1
  assert lib.pools_mask()==zmask(m),('released',hex(raw),hex(lib.pools_mask()),hex(zmask(m)))
  h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==0,raw
  refs=(m[0xC821],m[0xC826]);assert lib.pools_owner(0)==sum(1<<r for r in refs),('reused refs',hex(raw),refs)
  print('boss body hold',hex(raw),count,'entries: constructor/flames/99F6/reuse PASS',flush=True)

def walkers():
 """Stage 9, room 7: two 14h walkers, C800[0] references 0..5 and C800[1] references 6..11 (openMSX 9:7)."""
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom())
 h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
 for k in (0,1):
  h.cpu.c=0x14;h.cpu.de=0x8060;h.call(0x5F1D);assert m[0xC800+128*k]==0x14 and m[0xC820+128*k]==6
 assert (m[0xC821],m[0xC821+128],m[0xC821+128+25])==(0,6,11),(m[0xC821],m[0xC8A1])
 assert lib.boss15_reserve(0,6)==1 and lib.boss15_reserve(9,6)==1
 assert lib.pools_mask()==zmask(m)==0xFFF,(hex(lib.pools_mask()),hex(zmask(m)))
 h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==2
 refs=(m[0xC921],m[0xC926]);assert lib.pools_owner(2)==sum(1<<r for r in refs)==0x3000,(refs,hex(lib.pools_owner(2)))
 h.cpu.ix=0xC800;h.call(0x99F6);assert lib.boss15_reserve(0,0)==1;assert lib.pools_mask()==zmask(m)
 h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==0   # first free block, references 0 and 1
 assert lib.pools_owner(0)==3==(1<<m[0xC821])|(1<<m[0xC826]),(m[0xC821],m[0xC826])
 print('boss 9 walkers hold 6+6 entries (C800[0], C800[1]): constructor/flame/99F6/reuse PASS',flush=True)

if __name__=='__main__':main();walkers()
