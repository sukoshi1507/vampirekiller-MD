#!/usr/bin/env python3
"""Isolated compiled merchant test; RAM room entry, actual shop state freezes Simon."""
import argparse,json
from pathlib import Path
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS
from probe import ROOT

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,default=ROOT/'validation/trade-md');args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
 symbols=load_symbols(elf=ROOT/'sgdk/out/rom.out');m=Machine(args.core,ROOT/'sgdk/out/rom.bin');view=Game_View(m,symbols)
 def addr(name):return lookup(symbols,name)[0]
 g,w=addr('vk_game'),addr('vk_walls');c=addr('vk_candles');f=addr('room_freeze')
 assert lookup(symbols,'vk_enemies')[1]==166 and lookup(symbols,'vk_game')[1]==36 and lookup(symbols,'vk_walls')[1]==693
 def until(fn,n=400):
  for _ in range(n):
   if fn():return
   # The normal late collision engine can now make Simon fall during the
   # wall animation. Keep this merchant-only fixture stationary until it opens.
   if m.rd8(w+348)==0:
    for off,value in ((0,16),(1,160),(3,0),(6,0),(7,0),(26,0),(30,0)):m.wr(g+off,value)
   m.run(1,set())
  raise AssertionError(('timeout',m.frame,'freeze/health',m.rd8(f),m.rd8(addr('health')+1),m.rd8(g+3),m.rd16(addr('vk_front_mode')),m.rd8(w+348),[m.rd8(w+i) for i in range(18)]))
 until(lambda:m.rd16(addr('vk_front_mode'))==1);m.run(4,{BUTTONS['START']});m.run(4,set());m.run(4,{BUTTONS['START']});m.run(6,set());until(lambda:m.rd16(addr('vk_front_mode'))==3)
 reports=[]
 for stage,room,x,y,sale in [(12,4,128,112,29),(13,4,32,144,18),(14,6,96,144,14),(17,8,32,48,3),(17,9,32,48,22),(18,1,192,144,27),(18,8,32,48,22)]:
  m.wr(g+16,1,32);m.wr(g+20,room);m.wr(g+22,stage);m.wr(addr('vk_front_mode'),4,16)
  m.wr(g,16);m.wr(g+1,160);m.wr(g+26,0);m.wr(g+30,0);m.wr(addr('Enemies_god'),1)
  m.wr(addr('health')+1,0);m.wr(addr('health')+2,0,16)
  m.wr(addr('freeze_drawn'),0);m.wr(f,4);m.wr(f+1,4);m.wr(f+2,0)
  m.wr(addr('freeze_vtimer'),m.rd32(addr('vtimer')),32)
  until(lambda:m.rd8(w+16)==stage and m.rd8(w+17)==room and m.rd8(w)==0x82 and m.rd8(f)==0 and m.rd8(addr('freeze_drawn'))!=0)
  # Settle the queued room/SAT DMA before checking image and opening the shop.
  m.run(5,set());assert [m.rd8(w+i) for i in (1,2)]==[y,x]
  assert sum(max(p)>40 for p in m.image().get_flattened_data())>1000,('room must be visible',stage,room)
  m.image().save(args.out/f'stage{stage}-room{room}-wall.png')
  # Contact latch is RAM-injected, as in zo_merchant; no invented enemy behavior.
  for _ in range(8):
   m.wr(w+3,1);until(lambda:m.rd8(w)==3,10)
   until(lambda:m.rd8(w)==0x82 or m.rd8(w+348)!=0,100)
   if m.rd8(w+348):break
  until(lambda:m.rd8(w+348)==2,100);assert m.rd8(w+350)==sale
  m.wr(c+118,99);m.run(3,set())
  wx=x-(16 if x<128 else 32);wy=y-24;image=m.image()
  # MSX/MD hardware sprites can cover the window (a flea does in13/4).
  # This core smoke check covers its corners; the504 complete pixel comparisons
  # against9397 are in verify_trade_trace.py, before sprite composition.
  assert all(min(image.getpixel((wx+dx,wy)))>200 for dx in (0,1,78,79)),('window corners',stage,room)
  assert image.getpixel((wx+1,wy+1))==(0,0,0),('window interior',stage,room)
  m.image().save(args.out/f'stage{stage}-room{room}-shop.png')
  simon=bytes(m.rd8(g+i) for i in range(15));position=(m.rd8(g),m.rd8(g+1));price=m.rd8(w+351);timeout=m.rd8(w+352)
  m.run(300,set());assert bytes(m.rd8(g+i) for i in range(15))==simon and m.rd8(w+348)==2
  assert 8<=timeout-m.rd8(w+352)<=10
  m.run(1,{BUTTONS['B']});until(lambda:m.rd8(w+348)==3,8)
  assert m.rd8(c+118)==99-(price//16*10+price%16),(stage,'hearts',m.rd8(c+118),price)
  if sale in (29,27):assert m.rd8(c+120)==sale-25
  if sale==18:assert m.rd8(c+119)==3
  if sale==3:assert m.rd8(c+322)&16
  if sale==14:assert m.rd8(c+330)&1
  until(lambda:m.rd8(w+348)==0,30)
  reports.append({'stage':stage,'room':room,'sale':sale,'price_bcd':price,'frozen_images':300,'position':position})
  print('Trade MD',stage,room,'300 frozen images and purchase PASS',flush=True)
 (args.out/'summary.json').write_text(json.dumps({'frames':m.frame,'sizes':{n:lookup(symbols,n)[1] for n in ('vk_enemies','vk_game','vk_candles','vk_walls')},'cases':reports},indent=2)+'\n')
 print('Trade MD',m.frame,'frames PASS')
if __name__=='__main__':main()
