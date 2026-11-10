#!/usr/bin/env python3
"""LOT12 compiled SGDK: enemy death, airborne reserved loot and direct pickup."""
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
 results=[]
 for roll,kind,amount in [(4,25,1),(0,26,5)]:
  entry(12,0);pin(76,120);m.wr(e+162,1);m.wr(e+135,18)
  for i in range(7*18):m.wr(e+i,0)
  m.wr(e,1);m.wr(e+2,11);m.wr(e+3,1);m.wr(e+4,96,16);m.wr(e+6,112);m.wr(e+16,1)
  m.wr(addr('Enemies_s7R'),roll,16);m.wr(c+118,20)
  m.wr(addr('vk_whip'),12);m.wr(addr('vk_whip')+5,0)
  until(lambda:m.rd8(e+2) in (16,26),12)
  pin(16,176)
  until(lambda:m.rd8(e+2)==kind,40)
  assert m.rd8(e)==1,'airborne loot must reserve Enemy slot'
  raw=addr('enemyLoot');assert m.rd8(raw)==(36 if kind==25 else 38)
  for _ in range(4):pin(16,176);m.run(1,set())
  assert m.rd8(e)==1 and m.rd8(e+2)==kind
  m.image().save(a.out/f'airborne-{kind}.png')
  before=m.rd8(c+118);sx=m.rd16(e+4);sy=m.rd8(e+6)
  # Controlled air-contact fixture; actual Enemies_frame and step_candles run.
  for _ in range(8):
   pin(m.rd16(e+4),m.rd8(e+6));m.wr(e+127,0);m.wr(e+129,0);m.wr(e+145,0);m.wr(e+146,0);m.run(1,set())
   if not m.rd8(e):break
  assert not m.rd8(e),('loot not picked',kind,sx,sy,bytes(m.rd8(g+i) for i in range(36)).hex(),bytes(m.rd8(e+i) for i in range(166)).hex())
  assert m.rd8(c+118)==before+amount,('hearts',kind,before,m.rd8(c+118))
  assert m.rd8(psg+22)==15,('pickup PSG',kind,m.rd8(psg+22))
  m.run(2,set());m.image().save(a.out/f'picked-{kind}.png')
  results.append({'native_type':kind,'hearts':amount,'pickup_sound':15})
  print('Loot MD',kind,'reserved airborne slot, direct air pickup/hearts/PSG PASS',flush=True)
 (a.out/'summary.json').write_text(json.dumps({'frames':m.frame,'fixtures':results},indent=2)+'\n')
if __name__=='__main__':main()
