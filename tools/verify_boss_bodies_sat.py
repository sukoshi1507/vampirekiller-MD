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

def rider():
 """Stage 12, room 6: rider 15h (C800[0], references 0..5) + 18h (C800[1], references 6..7), openMSX 12:6."""
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom())
 h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
 h.cpu.c=0x15;h.cpu.de=0x8060;h.call(0x5F1D);h.cpu.c=0x18;h.cpu.de=0x8060;h.call(0x5F1D)
 assert (m[0xC800],m[0xC820],m[0xC880],m[0xC8A0])==(0x15,6,0x18,2),(m[0xC800],m[0xC820],m[0xC880],m[0xC8A0])
 assert [m[0xC821+5*k] for k in range(6)]==list(range(6)) and [m[0xC8A1],m[0xC8A6]]==[6,7]
 assert lib.boss15_reserve(0,6)==1 and lib.boss15_reserve(9,2)==1
 assert lib.pools_mask()==zmask(m)==0xFF,(hex(lib.pools_mask()),hex(zmask(m)))
 h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==2
 refs=(m[0xC921],m[0xC926]);assert lib.pools_owner(2)==sum(1<<r for r in refs)==0x300,(refs,hex(lib.pools_owner(2)))
 assert lib.pools_mask()==zmask(m)
 # The 18h rider disappears first (99F6), then the 15h body.
 h.cpu.ix=0xC880;h.call(0x99F6);assert lib.boss15_reserve(9,0)==1;assert lib.pools_mask()==zmask(m),('rider released',hex(lib.pools_mask()),hex(zmask(m)))
 h.cpu.ix=0xC800;h.call(0x99F6);assert lib.boss15_reserve(0,0)==1;assert lib.pools_mask()==zmask(m),('body released',hex(lib.pools_mask()),hex(zmask(m)))
 print('stage 12 rider 15h(6)+18h(2): constructor/flame/99F6/release PASS',flush=True)

def shots():
 """D700 shots of the bosses of stages 6 (type 6), 9 (type 7), 12 (0Ah) and 18 (05h): 2 entries, references 17 16, D700[k] = port slot k + 1."""
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom())
 for kind in (6,7,10,5):
  h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
  for k in range(3):
   h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(88<<8)|96;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
   assert m[0xD700+128*k]==kind and m[0xD720+128*k]==2,(kind,k,m[0xD700+128*k],m[0xD720+128*k])
   assert (m[0xD721+128*k],m[0xD726+128*k])==(17-2*k,16-2*k),(kind,k)
   assert lib.boss15_reserve(k+1,2)==1
   for j in range(k+1):h.cpu.ix=0xD700+128*j;h.call(0x6445)
   h.call(0x64E5)
   assert lib.pools_mask()==zmask(m),('shot mask',kind,k,hex(lib.pools_mask()),hex(zmask(m)))
  h.cpu.ix=0xD700;h.call(0x99F6);assert lib.boss15_reserve(1,0)==1
  assert lib.pools_mask()==zmask(m),('shot released',kind,hex(lib.pools_mask()),hex(zmask(m)))
  h.cpu.a=rom[0x608E:0x60BD].index(kind);h.cpu.bc=(88<<8)|96;h.cpu.hl=0;h.cpu.de=0;h.call(0x9F6D)
  assert m[0xD700]==kind and (m[0xD721],m[0xD726])==(17,16),('reuse',kind,m[0xD721],m[0xD726])
  print('D700 shot type',kind,'(2 entries, 3 slots): constructor/mask/99F6/reuse PASS',flush=True)

def flames():
 """Death of a boss body (kill 7E17 -> 81AB, 7806: every C800 block goes through 9A3A: 99F6 then, with CE00 != 0, a 1E flame of 2 entries built by the
 real 5F1D in the first free block, on the first two free references).  Stage 12 (15h 6 + 18h 2, CE0B != 0: both turn into flames; CE0B = 0: the
 rider only gets its 9AB3 delay and keeps its entries) and stage 15 (16h, 12 entries).  The port holds 2 entries for a walker with alive = 2 (actors.c:
 boss_bodies_sync, boss15_pool_sync), in the order of the blocks; they are released when the flame ends (16 passes, 9B71)."""
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.pools_begin.argtypes=[C.POINTER(C.c_ubyte)];rom=load_rom(reference_rom())
 for label,bodies,ce0b in (('stage 12 rider+flames',(0x15,0x18),1),('stage 12 rider delayed',(0x15,0x18),0),('stage 15 body 16h',(0x16,),0)):
  h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);cmap=(C.c_ubyte*768).from_buffer_copy(m[0xD100:0xD400]);lib.pools_begin(cmap)
  m[0xCE00]=1;m[0xCE0B]=ce0b
  for t in bodies:h.cpu.c=t;h.cpu.de=0x8060;h.call(0x5F1D)
  counts=[m[0xC820],m[0xC8A0]] if len(bodies)==2 else [m[0xC820]]
  assert lib.boss15_reserve(0,counts[0])==1 and (len(bodies)==1 or lib.boss15_reserve(9,counts[1])==1)
  assert lib.pools_mask()==zmask(m),(label,'body',hex(lib.pools_mask()),hex(zmask(m)))
  h.call(0x7806)
  assert m[0xC800]==0x1E and m[0xC820]==2,(label,hex(m[0xC800]))
  # the port: the flame of walker 0 first, then walker 1 (a delayed rider keeps its 2 entries)
  assert lib.boss15_reserve(0,0)==1 and lib.boss15_reserve(0,2)==1
  if len(bodies)==2:
   flame=m[0xC880]==0x1E;assert flame==bool(ce0b),(label,hex(m[0xC880]))
   if ce0b:assert lib.boss15_reserve(9,0)==1 and lib.boss15_reserve(9,2)==1   # body -> flame: released first even when the count is the same (the ROM frees 6,7 and takes the first two free references)
  assert lib.pools_mask()==zmask(m),(label,'flames',hex(lib.pools_mask()),hex(zmask(m)))
  refs=[(m[0xC821],m[0xC826])]+([(m[0xC8A1],m[0xC8A6])] if len(bodies)==2 else [])
  assert refs[0]==(0,1) and (len(refs)==1 or refs[1]==((2,3) if ce0b else (6,7))),(label,refs)
  # the flames end: 16 real passes, then 9B71 frees the blocks (rider delayed: its block stays, only the flame of walker 0 ends)
  for p in range(22):
   m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xC43A]=255;m[0xD010]=0;h.call(0x98E5);h.call(0x9E31)
   for k in range(2):
    if m[0xC800+128*k]:h.cpu.ix=0xC800+128*k;h.call(0x6445)
   h.call(0x64E5);h.call(0x64EC)
  assert m[0xC800]==0 and (ce0b==0 or m[0xC880]==0) or len(bodies)==1 and m[0xC800]==0,(label,'flames did not end')
  assert lib.boss15_reserve(0,0)==1
  if len(bodies)==2 and ce0b:assert lib.boss15_reserve(9,0)==1
  assert lib.pools_mask()==zmask(m),(label,'ended',hex(lib.pools_mask()),hex(zmask(m)))
  h.cpu.c=30;h.cpu.de=0x8040;h.call(0x5F1D);assert lib.pools_sat_alloc(30,128,64)==0,label
  assert lib.pools_owner(0)==(1<<m[0xC821])|(1<<m[0xC826]),(label,'reuse')
  print(label,': kill 7806 / flame holds / end of the flames / reuse PASS',flush=True)

if __name__=='__main__':main();walkers();rider();shots();flames()
