#!/usr/bin/env python3
"""Actual24/26 constructors/9BD9/9C1E, full IX32, lifetime and floor transfers."""
import ctypes as C,sys,json
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup
from verify_z80_maps import execute_map

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());images=0;cases=[]
 for stage,room in ((12,0),(13,0),(16,0),(17,6),(18,8)):
  tilemap=execute_map(rom,stage,room)[0]
  for kind in (36,38):
   for x,y in ((88,64),(128,128),(232,160)):
    h=setup(rom,stage,room);m=h.cpu.memory;h.call(0x63C5);retained=bytes([0]+[(i*17+3)&255 for i in range(1,32)]);m[0xC800:0xC820]=retained;m[0xD100:0xD400]=tilemap
    h.cpu.c=kind;h.cpu.d=x;h.cpu.e=y;h.call(0x5F1D);m[0xC81F]=2 if kind==38 else 0
    data=(C.c_ubyte*768).from_buffer_copy(tilemap);meta=(C.c_ubyte*32).from_buffer_copy(retained);lib.loot_begin(meta,data,stage,kind,x,y,2)
    lifetime=0
    for p in range(180):
     out=(C.c_ubyte*32)();rest=(C.c_int*5)();lib.loot_record(out,rest)
     assert bytes(out)==bytes(m[0xC800:0xC820]),(stage,room,kind,x,y,p,bytes(out).hex(),bytes(m[0xC800:0xC820]).hex())
     if m[0xC800]:
      lifetime+=1;m[0xC003]=p&255;h.cpu.ix=0xC800;h.call(0x9BD9 if kind==36 else 0x9C1E);h.call(0x99B9);h.call(0x99E5)
     lib.loot_step(p&255);images+=2
    cases.append((stage,room,kind,x,y,lifetime));print('Loot',cases[-1],'360 images IX32 PASS',flush=True)
 (ROOT/'research/loot-trace.json').write_text(json.dumps({'images':images,'cases':cases},indent=2)+'\n')
if __name__=='__main__':main()
