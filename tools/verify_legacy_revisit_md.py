#!/usr/bin/env python3
"""LOT14D core: stage10 fish3 first-shot cadence, controlled RAM fixtures."""
import argparse,json
from pathlib import Path
from probe import ROOT
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,default=Path('scratchpad/loot-md'));a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
 syms=load_symbols(elf=ROOT/'sgdk/out/rom.out');m=Machine(a.core,ROOT/'sgdk/out/rom.bin');Game_View(m,syms)
 def addr(n):return lookup(syms,n)[0]
 g,c,e,f,p=map(addr,('vk_game','vk_candles','vk_enemies','room_freeze','vk_player'))
 phase=addr('map_state');psg=addr('vk_psg');assert lookup(syms,'vk_enemies')[1]==166
 def until(fn,n=400,keys=set()):
  for _ in range(n):
   if fn():return
   m.run(1,keys)
  raise AssertionError(('timeout',m.frame,m.rd8(phase),m.rd8(g+20),m.rd8(e+2)))
 def pin(x=88,y=172):
  m.wr(g,x);m.wr(g+1,y);m.wr(g+2,0);m.wr(g+3,0);m.wr(g+4,0);m.wr(g+6,0);m.wr(g+7,0);m.wr(g+26,0);m.wr(g+30,0)
  for i in range(15):m.wr(p+i,m.rd8(g+i))
 def entry(stage,room):
  m.wr(g+16,1,32);m.wr(g+20,room);m.wr(g+22,stage);m.wr(addr('vk_front_mode'),4,16);pin(16,192)
  m.wr(addr('health')+1,0);m.wr(addr('health')+2,0,16);m.wr(addr('Enemies_god'),1)
  m.wr(addr('freeze_drawn'),0);m.wr(f,4);m.wr(f+1,4);m.wr(f+2,0);m.wr(addr('freeze_vtimer'),m.rd32(addr('vtimer')),32)
  until(lambda:m.rd8(f)==0 and m.rd8(addr('freeze_drawn')),30);m.run(6,set())
 until(lambda:m.rd16(addr('vk_front_mode'))==1);m.run(4,{BUTTONS['START']});m.run(4,set());m.run(4,{BUTTONS['START']});m.run(6,set());until(lambda:m.rd16(addr('vk_front_mode'))==3)
 entry(10,6);q=addr('vk_projectiles');ext=lookup(syms,'s8x');cases=[]
 for direction in (-1,1):
  for off,size in [(e,126),(q,144),ext,lookup(syms,'f3'),lookup(syms,'s8q'),lookup(syms,'sat_used'),lookup(syms,'c800_sat'),lookup(syms,'d700_sat')]:
   for i in range(size):m.wr(off+i,0)
  pin(88,176);m.wr(e,1);m.wr(e+2,17);m.wr(e+3,2);m.wr(e+4,192,16);m.wr(e+6,176);m.wr(e+10,1,16);m.wr(e+14,2);m.wr(e+16,0x12)
  m.wr(ext[0],direction*384,16);m.wr(ext[0]+4,9);m.wr(e+138,0,16);m.wr(e+145,255);m.wr(e+162,0)
  m.run(1,set());assert m.rd8(q)==1 and m.rd16(q+4)==192+3*direction,('first shot',direction,m.rd8(q),m.rd16(q+4))
  assert m.rd8(q+6)==156 and m.rd8(q+2)==10
  m.wr(e+162,1);m.run(360,set());assert m.rd8(q)==1 and m.rd16(q+4)==192+3*direction and m.rd8(e)==1
  m.image().save(a.out/f'fish3-first-shot-{direction}.png');cases.append({'direction':direction,'first_x':m.rd16(q+4),'frozen_continuation':360})
 report={'frames':m.frame,'cases':cases,'scope':'stage10/6 RAM fixtures; no natural traversal','passed':True}
 (a.out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print('Legacy fish3 MD PASS',report)
if __name__=='__main__':main()
