#!/usr/bin/env python3
"""LOT10 compiled SGDK:0E outline, three F2 uses, frozen world, projectile shield."""
import argparse,json
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS
from verify_item_trace import setup
from extract_rooms import decode_room
from verify_viewer import gx_rgb565

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,default=Path('scratchpad/item-md'));a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
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
 entry(13,0);m.wr(c+330,0);m.run(3,set());before=m.image().crop((32,48,64,80))
 m.wr(c+330,1);m.run(4,set());image=m.image()
 assert all(min(image.getpixel((x,y)))>200 for x,y in [(32+i,48) for i in range(32)]+[(32+i,79) for i in range(32)]+[(32,48+i) for i in range(32)]+[(63,48+i) for i in range(32)]),'0E white outline'
 image.save(a.out/'stage13-revealed.png');m.wr(c+330,0);m.run(4,set())
 assert list(m.image().crop((32,48,64,80)).get_flattened_data())==list(before.get_flattened_data()),'0E removal restores room pixels'
 print('Items MD0E:128 border pixels and underlying room restoration PASS',flush=True)
 m.wr(c+322,128);m.wr(c+329,3);rom=load_rom(reference_rom());palette=[gx_rgb565(rgb) for rgb in decode_room(rom,13,0)[0]['palette_rgb333']];pixel_checks=0
 for use in range(3):
  pin();m.run(1,{BUTTONS['A']});until(lambda:m.rd8(phase)==2,400);m.run(2,set())
  assert m.rd8(c+329)==2-use and (m.rd8(c+322)&128)==(128 if use<2 else 0)
  assert m.rd8(psg+22)==25
  player=bytes(m.rd8(g+i) for i in range(15));tick=m.rd8(e+158);timer=m.rd8(e+127)
  image=m.image();image.save(a.out/'map-diagnostic.png');reference=setup(rom,13,0);mem=reference.cpu.memory;mem[0xC002]=64;mem[0xCE00]=0;mem[0xC701]=128;mem[0xC70F]=3;mem[0xC00B]=2;mem[0xC427]=m.rd8(g);mem[0xC425]=m.rd8(g+1)
  reference.call(0x9552);mem[0xC00B]=0;reference.call(0x9552)
  for y in range(48,174):
   for x in range(24,232):assert image.getpixel((x,y))==palette[reference.pixel(x,y)],('MD F2 pixel',use,x,y,image.getpixel((x,y)),palette[reference.pixel(x,y)]);pixel_checks+=1
  m.run(320,{BUTTONS['RIGHT']});assert bytes(m.rd8(g+i) for i in range(15))==player and m.rd8(e+127)==timer
  assert m.rd8(e+158)==(tick+160)&255,'C003 continues while F2 freezes gameplay'
  if use==2:m.image().save(a.out/'stage13-third-map.png')
  m.run(1,{BUTTONS['A']});until(lambda:m.rd8(phase)==0,6,{BUTTONS['A']});m.run(2,set())
 print('Items MD F2:',pixel_checks,'pixels,three uses,960 frozen images PASS',flush=True)
 m.run(1,{BUTTONS['A']});m.run(4,set());assert m.rd8(phase)==0
 entry(14,0);pin(88,192)
 for i in range(7):
  for k in range(18):m.wr(e+18*i+k,0)
 ext=lookup(syms,'s8x');assert ext[1]%7==0
 for k in range(ext[1]):m.wr(ext[0]+k,0)
 m.wr(e,1);m.wr(e+2,10);m.wr(e+4,96,16);m.wr(e+6,164);m.wr(e+10,1,16);m.wr(e+16,3)
 m.wr(e+135,18);m.wr(e+162,1);m.wr(e+145,255);m.wr(e+146,16);m.wr(c+322,32);sounds=m.rd16(psg+24)
 for _ in range(6):
  pin(88,192);m.run(1,set())
  if m.rd8(e+2)==24:break
 else:raise AssertionError('compiled8610 did not intercept projectile')
 assert m.rd8(e+146)==15 and m.rd8(psg+22)==11 and m.rd16(psg+24)==(sounds+1)&65535
 life=0
 while m.rd8(e) and life<40:
  pin(88,192);m.run(1,set());life+=1
  if life==6:m.image().save(a.out/'stage14-shield-flame.png')
 assert life==32,('shield flame lifetime',life)
 print('Items MD shield:charge16->15,single0B,32-image flame PASS',flush=True)
 (a.out/'summary.json').write_text(json.dumps({'frames':m.frame,'map_pixels':pixel_checks,'map_uses':3,'frozen_images':960,'shield_flame_images':life},indent=2)+'\n')
 print('Items MD',m.frame,'images PASS')
if __name__=='__main__':main()
