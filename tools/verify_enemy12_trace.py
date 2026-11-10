#!/usr/bin/env python3
"""BE50/BE70 stage16 fixed entries: source, aimed shots and composed sprites."""
import ctypes as C,json,sys
from pathlib import Path
from probe import load_rom,reference_rom,ROOT
from castle_room_objects import make_machine,call
from extract_rooms import decode_room
from verify_late_channels_trace import Observed

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));lib.twelve_init.argtypes=[C.POINTER(C.c_ubyte)]+[C.c_int]*7
 original=load_rom(reference_rom());cases=[]
 table=make_machine(original);table.memory[0xD000]=16;table.memory[0xD002]=original[0x1E6A+16];call(table,0x6154)
 base=0xDB00+(15-3*table.memory[0xD002])*256
 for room in range(10):
  for slot in range(4):
   entry=bytes(table.memory[base+room*16+slot*4:base+room*16+slot*4+4])
   if entry[0]&127!=18:continue
   x,y=(entry[1]&15)*16,entry[1]&240
   for label,sx,sy,r in [('left',32,160,0),('right',232,160,64),('near',max(8,x-20),112,127)]:
    rom=original;assert rom[0x7F32:0x7F34]==bytes.fromhex('ed5f')
    h=Observed(rom,(0xBF34,));m=h.cpu;mem=m.memory;mem[0xD000]=16;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+16];h.call(0x6154)
    tilemap=bytes(decode_room(original,16,room)[2]);m.set_memory_block(0xD100,tilemap)
    mem[0xC427]=sx;mem[0xC425]=sy
    for a in range(0xD600,0xD700,4):mem[a]=0xE0
    m.c=18;m.d=x;m.e=y;h.call(0x5F1D)
    cmap=(C.c_ubyte*768).from_buffer_copy(tilemap);lib.twelve_init(cmap,16,room,x,y,sx,sy,r)
    trace=[];births=0;previous=set()
    for frame in range(400):
     if frame%2==0:
      h.random.clear()
      mem[0xC003]=(frame//2)&255
      if mem[0xC800]:m.ix=0xC800;h.call(0xBE70);h.call(0x99B9);h.call(0x99E5);h.call(0x6445)
      h.call(0x9E31);rv=(C.c_ubyte*len(h.random))(*h.random);lib.twelve_random(rv,len(h.random));lib.twelve_step();assert lib.twelve_random_used()==len(h.random)
     objects=[];live=set()
     for projectile in [-1]+list(range(8)):
      address=0xC800 if projectile<0 else 0xD700+128*projectile
      raw=bytes(mem[address:address+32]);b=(C.c_ubyte*32)();sat=(C.c_int*24)();n=lib.twelve_record(projectile,b,sat)
      if projectile>=0 and not raw[0]:assert not n;continue
      assert bytes(b)==raw,(room,label,frame,projectile,[(hex(i),a,c) for i,(a,c) in enumerate(zip(raw,b)) if a!=c])
      # EnemySprite composes pairs of MSX layers, like dragonSprites; compare the first layer of each cell.
      stride=2;indices=range(0,mem[address+0x20],stride)
      zsat=[list(mem[address+0x22+5*k:address+0x25+5*k]) for k in indices] if raw[0] and frame%2==0 else []
      if zsat:
       assert n==len(zsat),(room,label,frame,projectile,n,zsat)
       assert [list(sat[3*k:3*k+3]) for k in range(n)]==zsat,(room,label,frame,projectile,list(sat)[:n*3],zsat)
      if projectile>=0:live.add(projectile)
      objects.append({'slot':projectile,'raw':raw.hex(),'sprites':zsat})
     births+=len(live-previous);previous=live
     assert (mem[0xC427],mem[0xC425])==(sx,sy)
     trace.append({'frame':frame,'objects':objects,'simon':[sx,sy]})
    cases.append({'stage':16,'room':room,'slot':slot,'case':label,'r':r,'frames':400,'projectile_births':births,'trace':trace})
 print('12h:',len(cases),'scenarios',sum(c['frames'] for c in cases),'frames',sum(c['projectile_births'] for c in cases),'shots PASS')
 (ROOT/'research/enemy12-trace.json').write_text(json.dumps({'all_equal':True,'random_seam':'unmodified BF32 LD A,R; actual values observed after BF34 and replayed in C','cases':cases},indent=1)+'\n')
if __name__=='__main__':main()
