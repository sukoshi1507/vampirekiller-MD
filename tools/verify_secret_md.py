#!/usr/bin/env python3
"""Compiled SGDK secret controls/route; isolated RAM room entry pending LOT11 traversal."""
import argparse,json
from pathlib import Path
from probe import ROOT
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS
from build_secret_data import collect

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,default=Path('scratchpad/secret-md'));a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
 symbols=load_symbols(elf=ROOT/'sgdk/out/rom.out');m=Machine(a.core,ROOT/'sgdk/out/rom.bin');Game_View(m,symbols)
 def addr(n):return lookup(symbols,n)[0]
 g,s,e,f=map(addr,('vk_game','vk_secret','vk_enemies','room_freeze'))
 assert [lookup(symbols,n)[1] for n in ('vk_game','vk_enemies','vk_candles')]==[38,166,530]
 def until(fn,n=400,keys=set()):
  for _ in range(n):
   if fn():return
   m.run(1,keys)
  raise AssertionError(('timeout',m.frame,m.rd8(s+4),m.rd8(e+127),m.rd8(g+20)))
 def entry(stage,room):
  m.wr(g+16,1,32);m.wr(g+20,room);m.wr(g+22,stage);m.wr(addr('vk_front_mode'),4,16)
  m.wr(g,16);m.wr(g+1,192);m.wr(g+3,0);m.wr(g+7,0);m.wr(g+26,0);m.wr(g+30,0)
  m.wr(addr('health')+1,0);m.wr(addr('health')+2,0,16);m.wr(addr('Enemies_god'),1)
  m.wr(addr('freeze_drawn'),0);m.wr(f,4);m.wr(f+1,4);m.wr(f+2,0);m.wr(addr('freeze_vtimer'),m.rd32(addr('vtimer')),32)
  until(lambda:m.rd8(f)==0 and m.rd8(addr('freeze_drawn'))!=0,30)
  m.run(6,set());assert sum(max(p)>40 for p in m.image().get_flattened_data())>1000
 def pin(x,y,left):
  m.wr(g,x);m.wr(g+1,y);m.wr(g+2,left);m.wr(g+3,0);m.wr(g+6,0);m.wr(g+7,1);m.wr(g+8,0);m.wr(g+26,0);m.wr(g+30,0)
 until(lambda:m.rd16(addr('vk_front_mode'))==1);m.run(4,{BUTTONS['START']});m.run(4,set());m.run(4,{BUTTONS['START']});m.run(6,set());until(lambda:m.rd16(addr('vk_front_mode'))==3)
 _,points,raw=collect();reports=[]
 for stage,room,dest,x,y in raw:
  for phase in (0,1):
   entry(stage,room);assert [m.rd8(s+i) for i in range(4)]==[1,y,x,dest]
   pin(x,y+4,phase);m.wr(e+147,phase);m.wr(e+127,0);m.wr(e+128,0)
   psg=addr('vk_psg');sounds_before=m.rd16(psg+24)
   m.run(1,{BUTTONS['UP']});until(lambda:m.rd8(s+4)==7,4,{BUTTONS['UP']})
   assert m.rd8(psg+22)==0x15 and m.rd16(psg+24)==(sounds_before+1)&65535,('secret sound request',room,phase)
   start=m.frame;timer=m.rd8(e+127);assert timer==64
   player_before=bytes(m.rd8(g+i) for i in range(15));hidden=set()
   for p in range(140):
    assert bytes(m.rd8(g+i) for i in range(15))==player_before,('Simon15 changed',stage,room,phase,p)
    if m.rd8(g+20)!=room:break
    hidden.add(m.rd8(e+165));m.run(1,{BUTTONS['UP'],BUTTONS['RIGHT']})
    if room==0 and phase==0 and p==20:m.image().save(a.out/'stage12-passage.png')
   else:raise AssertionError(('no room route',stage,room,phase))
   assert m.rd8(g+20)==dest and m.rd8(s+4)==0
   elapsed=m.frame-start;assert elapsed==128,(stage,room,phase,elapsed)
   assert hidden=={0,1}
   reports.append({'room':room,'destination':dest,'initial_phase':phase,'active_images':elapsed,'timer_after_entry':timer})
  print('Secret MD room',room,'->',dest,'both input phases PASS',flush=True)
 # Stage change after a real passage must clear its former C5B1-equivalent.
 for stage in range(13,19):
  entry(stage,0);assert [m.rd8(s+i) for i in range(4)]==[0,0,0,0]
  for p in range(320):
   pin(88,172,0);m.run(1,{BUTTONS['UP']} if p%4==0 else set());assert m.rd8(s+4)==0
  print('Secret MD stage',stage,'320 negative images PASS',flush=True)
 m.image().save(a.out/'stage18-no-passage.png')
 (a.out/'summary.json').write_text(json.dumps({'frames':m.frame,'cases':reports,'absent_stages':list(range(13,19))},indent=2)+'\n')
 print('Secret MD',m.frame,'frames,20 routes and6 absent stages PASS')
if __name__=='__main__':main()
