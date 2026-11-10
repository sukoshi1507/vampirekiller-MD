#!/usr/bin/env python3
"""LOT13 compiled SGDK: actual candle whip, C800 reservation, pickups and C500 fallback."""
import argparse,json
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from castle_room_objects import decode_room
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
 results=[];rom=load_rom(reference_rom())
 for stage,kind,amount in [(12,1,1),(13,2,5)]:
  entry(stage,0);m.wr(e+162,1);m.wr(e+135,18)
  for i in range(7*18):m.wr(e+i,0)
  for i in range(8):m.wr(c+6+14*i,0)
  m.wr(c+4,255);m.wr(c+118,20)
  b=bytes.fromhex(next(o['raw'] for o in decode_room(rom,stage,0)['candles'] if bytes.fromhex(o['raw'])[5]==kind))
  pin(b[2]-12,b[1]+18);m.wr(addr('vk_whip'),0);m.wr(addr('vk_whip')+5,0)
  m.run(2,{BUTTONS['B']});m.run(2,set());until(lambda:m.rd8(e)!=0,30)
  expected=16 if kind==1 else 26;assert m.rd8(e+2)==expected,('candle allocation',kind,m.rd8(e+2))
  assert not any(m.rd8(c+6+14*i) for i in range(8)),'successful C800 candle must not allocate C500'
  pin(16,176)
  until(lambda:m.rd8(e+2)==(25 if kind==1 else 26),40)
  assert m.rd8(e)==1
  m.image().save(a.out/f'candle-{kind}-reserved.png')
  before=m.rd8(c+118)
  for _ in range(12):
   pin(m.rd16(e+4),m.rd8(e+6));m.wr(e+127,0);m.wr(e+129,0);m.wr(e+146,0);m.run(1,set())
   if not m.rd8(e):break
  assert not m.rd8(e) and m.rd8(c+118)==before+amount,('candle air pickup',kind,m.rd8(e),m.rd8(c+118))
  assert m.rd8(psg+22)==15
  m.run(360,set());m.image().save(a.out/f'candle-{kind}-picked.png')
  results.append({'content':kind,'hearts':amount,'sound':15,'post_pick_images':360})
  print('Candle MD',kind,'real B hit,C800 reservation,no C500 duplicate,air pickup,PSG,360-image continuation PASS',flush=True)
 # Saturated C800: same actual candle B hit must fall back to C500.
 entry(13,0);m.wr(e+162,1);m.wr(e+135,18);m.wr(c+4,255);m.wr(c+118,20)
 for i in range(8):m.wr(c+6+14*i,0)
 for i in range(7):
  q=e+18*i
  for j in range(18):m.wr(q+j,0)
  m.wr(q,1);m.wr(q+2,16);m.wr(q+4,24+24*i,16);m.wr(q+6,64);m.wr(q+10,1,16);m.wr(q+15,255);m.wr(q+16,0x85)
 pin(36,162);m.wr(addr('vk_whip'),0);m.run(2,{BUTTONS['B']});m.run(2,set())
 until(lambda:m.rd8(c+6)!=0,30)
 raw=addr('fallback_raw');assert m.rd8(raw)==1 and m.rd8(raw+4)==2
 assert all(m.rd8(e+18*i) and m.rd8(e+18*i+2)==16 for i in range(7))
 pin(16,176);until(lambda:m.rd8(raw)==0x82,40);m.image().save(a.out/'fallback-air.png')
 before=m.rd8(c+118)
 for _ in range(12):
  pin(m.rd8(raw+2)+8,m.rd8(raw+1)+18);m.run(1,set())
  if not m.rd8(raw):break
 assert not m.rd8(raw) and m.rd8(c+118)==before+5 and m.rd8(psg+22)==15
 m.run(360,set());m.image().save(a.out/'fallback-picked.png')
 results.append({'fallback':'seven C800 occupied','hearts':5,'sound':15,'post_pick_images':360})
 print('Candle MD pool-full fallback,C500 air pickup,PSG,360-image continuation PASS',flush=True)
 (a.out/'summary.json').write_text(json.dumps({'frames':m.frame,'cases':results},indent=2)+'\n')
if __name__=='__main__':main()
