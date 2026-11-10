#!/usr/bin/env python3
"""LOT14 core: actual hopper shots use separate D700 and shared SAT allocation."""
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
 entry(13,3);q=addr('vk_projectiles');assert lookup(syms,'vk_projectiles')[1]==144
 for i in range(126):m.wr(e+i,0)
 for i in range(144):m.wr(q+i,0)
 for name in ('sat_used','c800_sat','d700_sat'):
  off,size=lookup(syms,name)
  for i in range(size):m.wr(off+i,0)
 ext=lookup(syms,'s8x');stride=ext[1]//7
 for i in range(ext[1]):m.wr(ext[0]+i,0)
 for i in range(6):
  off=e+18*i;m.wr(off,1);m.wr(off+2,11 if i==0 else 16);m.wr(off+4,96 if i==0 else 24+24*i,16);m.wr(off+6,176 if i==0 else 64);m.wr(off+10,0,16);m.wr(off+3,3 if i==0 else 1);m.wr(off+15,0 if i==0 else 255);m.wr(off+16,0x49 if i==0 else 0x85)
 m.wr(e+135,18);m.wr(e+145,255);m.wr(e+162,0);m.wr(addr('Enemies_s7R'),1,16);pin(16,176)
 def count():return sum(bool(m.rd8(q+18*i)) for i in range(8))
 for target in (1,2):
  for _ in range(8):
   pin(16,176);m.wr(e+145,255);m.wr(e+14,2);m.wr(ext[0]+3,1);m.wr(ext[0]+2,0);m.run(1,set())
   if count()>=target:break
  assert count()==target,('actual hopper D700 allocation',target,count())
 # Keep the two created bones in the room during denied third attempts.
 for _ in range(6):
  for i in range(8):
   if m.rd8(q+18*i):m.wr(q+18*i+4,96,16);m.wr(q+18*i+6,144);m.wr(q+18*i+12,0,16)
  pin(16,176);m.wr(e+14,2);m.wr(ext[0]+3,1);m.wr(e+145,255);m.run(1,set());assert count()==2,'SAT must refuse third shot'
 assert all(m.rd8(e+18*i) for i in range(6));m.wr(e+162,1);m.run(3,set());m.image().save(a.out/'six-c800-two-d700.png')
 # The ordinary production shield path scans and transforms real D700 records.
 m.wr(c+322,32);m.wr(e+146,16)
 for i in range(2):m.wr(q+18*i+4,96,16);m.wr(q+18*i+6,144)
 for _ in range(8):
  pin(88,172);m.run(1,set())
  if all(m.rd8(q+18*i+2)==24 for i in range(2)):break
 assert all(m.rd8(q+18*i+2)==24 for i in range(2)) and m.rd8(e+146)==14,('shield private pool',m.rd8(e+146))
 assert m.rd8(psg+22)==11;m.image().save(a.out/'d700-shield-flames.png')
 pin(16,176);m.run(40,set());assert count()==0 and all(m.rd8(e+18*i) for i in range(6))
 # >=300 real core images with retained C800 records; no further source shots.
 m.run(320,set());assert count()==0 and all(m.rd8(e+18*i) for i in range(6))
 m.image().save(a.out/'c800-retained.png')
 report={'frames':m.frame,'C800':6,'D700_peak':2,'third_shot':'SAT denied','shield_charge':[16,14],'sound':11,'continuation':360,'passed':True}
 (a.out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print('Mixed pools MD PASS',report)
if __name__=='__main__':main()
