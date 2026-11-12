#!/usr/bin/env python3
"""Compiled LOT11: all74 late room loads, collision RAM and stage11->12 door."""
import argparse,json
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS
from decode_room_links import decode
from verify_item_trace import setup

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,required=True);a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
 syms=load_symbols(elf=ROOT/'sgdk/out/rom.out');m=Machine(a.core,ROOT/'sgdk/out/rom.bin');Game_View(m,syms)
 def addr(n):return lookup(syms,n)[0]
 g,c,e,f,p=map(addr,('vk_game','vk_candles','vk_enemies','room_freeze','vk_player'));rom=load_rom(reference_rom())
 assert lookup(syms,'vk_candles')[1]==530
 def until(fn,n=400,keys=set()):
  for _ in range(n):
   if fn():return
   m.run(1,keys)
  raise AssertionError(('late MD timeout',m.frame,m.rd8(g+22),m.rd8(g+20)))
 def pin(x=88,y=192):
  for off,v in ((0,x),(1,y),(2,0),(3,0),(4,0),(6,0),(7,0),(26,0),(30,0)):m.wr(g+off,v)
  for i in range(15):m.wr(p+i,m.rd8(g+i))
 def entry(stage,room):
  m.wr(g+16,1,32);m.wr(g+20,room);m.wr(g+22,stage);m.wr(addr('vk_front_mode'),4,16);pin();m.wr(c,255)
  m.wr(addr('health')+1,0);m.wr(addr('health')+2,0,16);m.wr(addr('Enemies_god'),1);m.wr(e+162,1);m.wr(e+145,255)
  m.wr(addr('freeze_drawn'),0);m.wr(f,4);m.wr(f+1,4);m.wr(f+2,0);m.wr(addr('freeze_vtimer'),m.rd32(addr('vtimer')),32)
  until(lambda:m.rd8(f)==0 and m.rd8(addr('freeze_drawn')),60);m.run(6,set())
 until(lambda:m.rd16(addr('vk_front_mode'))==1);m.run(4,{BUTTONS['START']});m.run(4,set());m.run(4,{BUTTONS['START']});m.run(6,set());until(lambda:m.rd16(addr('vk_front_mode'))==3)
 maps=0;rooms=0
 for graph in decode(rom)[12:]:
  stage=graph['stage']
  for room in range(len(graph['rooms'])):
   entry(stage,room);h=setup(rom,stage,room)
   for _ in range(20):h.call(0x8671)
   expected=bytes(h.cpu.memory[0xD100:0xD400]);rp=addr('room_map')
   actual=bytes(m.rd8(rp+i) for i in range(768));assert actual==expected,(stage,room,'mutable collision',[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
   # A loaded room must remain addressable for >=300 frames with the world frozen.
   clock=m.rd8(e+158)
   for i in range(300):pin();m.run(1,set())
   assert m.rd8(e+158)==(clock+150)&255,(stage,room,"world clock must advance150 passes")
   assert m.rd8(g+22)==stage and m.rd8(g+20)==room and m.rd8(c)==stage and m.rd8(c+1)==room,(stage,room,'room stability')
   if room==0 or (stage==18 and room==9):m.image().save(a.out/f'stage{stage}-room{room}.png')
   maps+=768;rooms+=1
  print('Late MD stage',stage,'all rooms x300 frames and collision bytes PASS',flush=True)
 # Exercise the newly enabled ordinary door dispatch11->12 using the compiled controller path.
 entry(11,5);it=addr('items')
 for i in range(6):m.wr(it+i,0)
 m.wr(it,1);pin(228,144);m.wr(g+2,0);m.wr(g+3,0)
 until(lambda:m.rd8(it+2)!=0,40,{BUTTONS['RIGHT']});until(lambda:m.rd8(g+22)==12,400)
 assert m.rd8(g+20)==0,'stage12 arrival room'
 m.image().save(a.out/'stage11-to12.png')
 print('Late MD compiled door11->12 PASS',flush=True)
 (a.out/'summary.json').write_text(json.dumps({'frames':m.frame,'rooms':rooms,'collision_bytes':maps,'stable_frames_per_room':300,'door11_to12':True,'scope':'RAM room entry, real compiled loading and door buttons; not full traversal'},indent=2)+'\n')
if __name__=='__main__':main()
