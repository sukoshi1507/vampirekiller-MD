#!/usr/bin/env python3
"""Poses of the legacy species (ROM types 03, 05, 07) that the openMSX room traces never showed but the ROM handlers reach.

The real 5F1D constructor and the real 98E5/9E31 frame loop run on a C800 object with Simon at several places; the poses (+0B) seen must
contain build_c800_sat.LEGACY_EXTRA, so that the SAT rows of those poses are facts and not guesses.  (The panther constructor itself takes
43h when Simon is to its right: 5F1D with C427 above the X.)"""
from probe import load_rom,reference_rom
from verify_item_trace import setup
from build_c800_sat import LEGACY_EXTRA

def reached(rom,raw):
 seen=set()
 for sx in (20,60,100,128,160,200,240):
  for x,y in ((128,160),(60,160),(200,160),(128,96)):
   h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xC427]=sx;m[0xC425]=y
   h.cpu.c=raw;h.cpu.de=(x<<8)|y;h.call(0x5F1D)
   if m[0xC800]!=raw:continue
   seen.add(m[0xC80B])
   for p in range(300):
    m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xC43A]=255
    h.call(0x98E5);h.call(0x9E31)
    if m[0xC800]!=raw:break
    seen.add(m[0xC80B])
 return seen

def main():
 rom=load_rom(reference_rom())
 for raw,poses in LEGACY_EXTRA.items():
  seen=reached(rom,raw);missing=sorted(set(poses)-seen)
  assert not missing,('poses not reached by the ROM',raw,[hex(p) for p in missing],sorted(seen))
  print('legacy poses ROM type %02X: %s reached (all seen: %s) PASS'%(raw,[hex(p) for p in poses],[hex(p) for p in sorted(seen)]),flush=True)
if __name__=='__main__':main()
