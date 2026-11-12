#!/usr/bin/env python3
"""LOT11 normal-path loading and persistent stream flags against unmodified Z80."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_z80_maps import execute_map
from decode_room_links import decode,check
from castle_room_objects import decode_room
from verify_item_trace import setup

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());graphs=decode(rom)[12:];cases=[];pixels=0;keys=0;images=0
 for graph in graphs:
  stage=graph['stage'];lib.late_init(stage);h=setup(rom,stage,0);h.call(0x6402);mem=h.cpu.memory;o=(C.c_int*64)();lib.late_info(o)
  assert list(o)[2:5]==[mem[0xC427],mem[0xC425],mem[0xC42C]],(stage,'6402 entry')
  for room,q in enumerate(graph['rooms']):
   lib.late_init(stage);lib.late_room(room);lib.late_info(o);out=(C.c_ubyte*768)();assert lib.late_map(stage,room,out)
   expected=execute_map(rom,stage,room)[0];assert bytes(out)==expected,(stage,room,'4F83 collision');pixels+=768
   for d,name in enumerate(('UP','DOWN','LEFT','RIGHT')):assert lib.late_link(stage,room,d)==(255 if q[name] is None else q[name])
   objects=decode_room(rom,stage,room);defs=[];white=[];fixed=[]
   for ob in objects['candles']:
    b=bytes.fromhex(ob['raw']);defs.extend((b[2],b[1],b[4],b[5],b[9]))
    if b[4]==3 and b[5]==24:white.append((b[2],b[1]))
   for ob in objects['other_objects']:
    b=bytes.fromhex(ob['raw'])
    if b[0]==131 and b[4]==24:white.append((b[2],b[1]))
    elif b[0]==131:fixed.append((b[2],b[1],b[4],b[13]))
   fo=(C.c_int*32)();fn=lib.late_fixed(fo);assert sorted(tuple(fo[4*i:4*i+4]) for i in range(fn))==sorted(fixed),(stage,room,'5B1B fixed/chests')
   assert o[0]==len(objects['candles']) and list(o)[12:12+len(defs)]==defs,(stage,room,'5B1B candles')
   assert o[11]==530 and bool(o[5])==bool(white),(stage,room,'layout/keys')
   if white:assert tuple(o[6:8])==white[0];keys+=1
   for index,ob in enumerate(objects['candles']):
    b=bytes.fromhex(ob['raw']);h=setup(rom,stage,room);mem=h.cpu.memory;a=int(ob['slot'],16);flag=mem[a+7]*256+mem[a+8]
    # 7D68 executes the real whip/candle path, including persistence and drop creation.
    mem[0xC427]=(b[2]+28 if b[2]<12 else b[2]-12)&255;mem[0xC425]=(b[1]+18)&255;mem[0xC422]=3;mem[0xC42C]=int(b[2]<12);mem[0xC416]=0
    before=mem[a];mem[0xC420]=0;h.call(0x7D68);h.call(0x8671)
    assert not mem[a],(stage,room,index,'real whip did not destroy the target')
    lib.late_init(stage);lib.late_room(room);lib.late_hit(index)
    assert bool(lib.late_out()&(1<<index))==(mem[flag]==0),(stage,room,index,'stream after whip',hex(flag))
    mem[0xC422]=0;mem[0xC427]=0;mem[0xC425]=32
    for p in range(180):
     mem[0xC003]=p&255;h.call(0x8A4A);lib.late_wait()
     assert bool(lib.late_out()&(1<<index))==(mem[flag]==0),(stage,room,index,p,'persistent flag')
     images+=2
    # Reload from the modified stream, without5A49, as a normal room return.
    h.clear_room_objects();h.call(0x5B1B);lib.late_room((room+1)%len(graph['rooms']));lib.late_room(room);lib.late_info(o)
    native=sorted(tuple(o[12+5*i:17+5*i]) for i in range(o[0]) if o[1]&(1<<i))
    original=[]
    for slot in range(8):
     b0=bytes(mem[0xC470+slot*16:0xC480+slot*16])
     if b0[0]:original.append((b0[2],b0[1],b0[4],b0[5],b0[9]))
    assert native==sorted(original),(stage,room,index,'room return compacted slots',native,original)
    cases.append((stage,room,index,b[4],b[5]))
   lib.late_reset();lib.late_info(o)
   h=setup(rom,stage,0);h.call(0x6402);assert list(o)[2:5]==[h.cpu.memory[0xC427],h.cpu.memory[0xC425],h.cpu.memory[0xC42C]],(stage,'respawn641F')
  print('Late runtime stage',stage,'all rooms/candles/keys/entry/respawn PASS',flush=True)
 print('Late runtime:',pixels,'collision bytes;',keys,'white-key rooms;',len(cases),'whip/reentry cases;',images,'images PASS',flush=True)
 (ROOT/'research/late-runtime-trace.json').write_text(json.dumps({'rooms':74,'map_bytes':pixels,'keys':keys,'cases':cases,'images':images,'ROM_link_asymmetries_preserved':check(graphs)},indent=2)+'\n')
if __name__=='__main__':main()
