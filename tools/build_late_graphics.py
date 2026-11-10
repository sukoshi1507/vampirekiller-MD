#!/usr/bin/env python3
"""Compose late enemy cells from executed5780/6445 and check native SAT placement."""
import json
import argparse,ctypes as C,subprocess,tempfile
from probe import ROOT,load_rom,reference_rom
from build_boss12_md import BossGraphics
from decode_room_links import decode
from castle_room_objects import decode_room

SPECS=[(9,22,(0,*range(0x21,0x27))),(16,20,range(0x5F,0x63)),(18,23,(0x4E,0x4F)),(11,11,range(0x47,0x4B)),(13,14,(0x67,0x68,0x6A,0x6B)),(15,18,(0x6D,0x6E,0x8D))]
def words(pix):
 return [sum(pix[8*r+y][8*c+x]<<(28-4*x) for x in range(8)) for c in range(2) for r in range(2) for y in range(8)]
def generate():
 rom=load_rom(reference_rom());fixed=json.loads((ROOT/'research/late-fixed-enemies-z80.json').read_text())['stages'];out=[];allwords=[];allcells=[];rooms=[];checks=0
 with tempfile.TemporaryDirectory() as tmp:
  so=tmp+'/graphics.so';src=['tests/late_graphics_bridge.c','sgdk/src/player.c','sgdk/src/generated/player_assets.c','sgdk/src/generated/stage1_collision.c']+[str(p.relative_to(ROOT)) for p in (ROOT/'sgdk/src/generated').glob('stage*_maps.c')]
  subprocess.run(['cc','-shared','-fPIC','-O2','-std=c99','-Isgdk/src',*src,'-o',so],cwd=ROOT,check=True);lib=C.CDLL(so)
  for graph in decode(rom)[12:]:
   stage=graph['stage'];row=[]
   for room in range(12):
    if room>=len(graph['rooms']):row.append((0,0,0,0));continue
    h=BossGraphics(rom);m=h.cpu;mem=m.memory;mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage];mem[0xF0F1:0xF0F4]=bytes((1,2,3))
    h.call(0x5768);h.call(0x5780);dedup={};cells={};tilewords=[]
    def cell(internal,pattern,records):
     nonlocal checks
     pixels=[[0]*16 for _ in range(16)]
     for pat,col in records:
      for y in range(16):
       a=0xF800+8*pat;bits=h.vram[a+y]*256+h.vram[a+y+16]
       for x in range(16):
        if bits&(0x8000>>x):pixels[y][x]|=col&15
     w=tuple(words(pixels));key=(internal,pattern)
     if key in cells:assert tilewords[cells[key]*32:cells[key]*32+32]==list(w),(stage,room,key,'conflicting cell')
     else:
      if w not in dedup:dedup[w]=len(tilewords)//32;tilewords.extend(w)
      cells[key]=dedup[w]
     checks+=256
    allowed={q['raw_type']&127 for q in fixed[stage-12]['rooms'][room]}
    flagoff=14*8192+0x5A6+2*stage;ptr=int.from_bytes(rom[flagoff:flagoff+2],'little')
    if rom[14*8192+ptr-0x8000+room]&64:allowed.update((13,15))
    specs=list(SPECS)
    if any(q['kind']==21 for q in decode_room(rom,stage,room)['candles']):
     m.set_memory_block(0xC800,bytes(896))
     for a in range(0xD600,0xD700,4):mem[a]=224
     m.c=27;m.de=0x80A0;h.call(0x5F1D);basepose=mem[0xC80B];allowed.add(27);specs.append((27,15,(basepose,basepose+1)))
    if 11 in allowed:allowed.add(4);specs.append((4,13,(3,0x4B,0x4C,0x4D)))
    if 16 in allowed:allowed.add(109);specs.append((109,21,(3,0x63,0x64,0x65,0x66)))
    for raw,internal,poses in specs:
     if raw not in allowed:continue
     for pose in poses:
      m.set_memory_block(0xC800,bytes(896));m.set_memory_block(0xD700,bytes(1024))
      for a in range(0xD600,0xD700,4):mem[a]=224
      if raw in (4,109):
       m.a=11 if raw==4 else 16;m.bc=0x80A0;m.hl=m.de=0;h.call(0x9F6D);base=0xD700
      else:m.c=raw;m.de=0x80A0;h.call(0x5F1D);base=0xC800
      m.ix=base;mem[base+11]=pose
      if raw==9 and 0x21<=pose<0x25:
       mem[0xC811]=1;mem[0xC427]=200;h.call(0xAF5F);mem[base+11]=pose
      h.call(0x6445)
      entries=[]
      for k in range(mem[base+32]):
       a=base+34+5*k
       if mem[a+3]&15:entries.append(tuple(mem[a:a+4]))
      dst=(C.c_int*32)();n=lib.late_patterns(internal,pose,stage,dst)
      stride=2 if len(entries)==2*n else 1
      assert len(entries)==stride*n,(stage,room,raw,pose,len(entries),n)
      for i in range(n):
       es=entries[i*stride:(i+1)*stride];dy,dx=es[0][:2]
       assert (dst[4*i+2],dst[4*i+3])==(dx,dy),(stage,room,raw,pose,i,'SAT placement',list(dst)[:4*n],entries,bytes(mem[base:base+32]).hex())
       if dst[4*i+1]==4:
        #Constructor09 pose0 shares E8 with loot, coloured02 OR05=07.
        assert raw==9 and pose==0 and [(e[2],e[3]&15) for e in es]==[(0xE8,2),(0xE8,5)]
        assert dst[4*i]==4;checks+=256
       else:cell(internal,dst[4*i],[(e[2],e[3]) for e in es])
    # Dragon's dynamic chain uses three composed cells, verified by its existing SAT differential.
    if 14 in allowed:
     m.set_memory_block(0xC800,bytes(896))
     for a in range(0xD600,0xD700,4):mem[a]=224
     m.c=14;m.de=0x80A0;h.call(0x5F1D);m.ix=0xC800;h.call(0x6445)
     assert mem[0xC820]==8 and all((mem[0xC825+10*k]&15,mem[0xC82A+10*k]&15)==(2,12) for k in range(4)),'executed dragon colours'
     for dest,pat in ((0x90,0x80),(0x94,0x78),(0x98,0x70)):cell(19,dest,[(pat,2),(pat+4,12)])
    # Raw D700 projectile1 is the single-colour F0 sprite.
    if 14 in allowed or 18 in allowed:
     m.set_memory_block(0xD700,bytes(1024))
     for a in range(0xD600,0xD700,4):mem[a]=224
     m.a=0;m.bc=0x80A0;m.hl=m.de=0;h.call(0x9F6D);m.ix=0xD700;h.call(0x6445)
     assert (mem[0xD724],mem[0xD725]&15)==(0xF0,8),'executed raw1 projectile image'
     for dest in (0x60,0xF0):cell(10,dest,[(0xF0,8)])
    assert len(tilewords)//8<=128,(stage,room,'VRAM allocation')
    start=len(allwords)//8;first=len(allcells);allwords.extend(tilewords);allcells.extend((t,p,c) for (t,p),c in sorted(cells.items()));row.append((start,len(tilewords)//8,first,len(cells)))
   rooms.append(row)
 out=['/* Generated by tools/build_late_graphics.py via5780/6445. */','typedef struct {u32 first;u16 tiles,cell_first,cells;} VKLateGraphicsRoom;','typedef struct {u8 type,pattern,cell;} VKLateGraphicsCell;']
 out.append('static const VKLateGraphicsRoom late_graphics_rooms[7][12]={'+','.join('{'+','.join('{'+','.join(map(str,q))+'}' for q in row)+'}' for row in rooms)+'};')
 out.append('static const VKLateGraphicsCell late_graphics_cells[]={'+','.join('{'+','.join(map(str,q))+'}' for q in allcells)+'};')
 out.append('static const u32 late_graphics_words[]={\n'+''.join(','.join('0x%08X'%v for v in allwords[i:i+8])+',\n' for i in range(0,len(allwords),8))+'};\n')
 print('Late graphics:',checks,'source pixels, native SAT placement checked; max128 tiles/room')
 return '\n'.join(out)
def main():
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args();t=generate();f=ROOT/'sgdk/src/generated/late_graphics.inc'
 if a.check:assert f.read_text()==t
 else:f.write_text(t)
if __name__=='__main__':main()
