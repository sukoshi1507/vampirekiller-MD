#!/usr/bin/env python3
"""LOT14F core: object09 combined score/sound routing, controlled RAM fixtures."""
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
 entry(13,3);q=addr('vk_projectiles');cases=[]
 for body,warning in ((True,17),(False,0)):
  for off,size in [(e,126),(q,144),lookup(syms,'s8x'),lookup(syms,'s8q'),lookup(syms,'sat_used'),lookup(syms,'c800_sat'),lookup(syms,'d700_sat')]:
   for i in range(size):m.wr(off+i,0)
  pin(88,172);m.wr(addr('vk_hud_score'),0,32);m.wr(e+143,150);m.wr(e+144,warning);m.wr(e+145,255);m.wr(e+147,1);m.wr(e+162,1);m.wr(e+135,18)
  if body:
   m.wr(e,1);m.wr(e+2,22);m.wr(e+3,2);m.wr(e+4,90,16);m.wr(e+6,172);m.wr(e+14,1);m.wr(e+16,0x21)
  m.wr(q,1);m.wr(q+2,10);m.wr(q+4,88,16);m.wr(q+6,148)
  accepted_before=m.rd16(psg+24)
  m.run(1,set());score=300 if body else 100
  assert m.rd32(addr('vk_hud_score'))==score,('combined HUD score',body,m.rd32(addr('vk_hud_score')))
  count=m.rd8(addr('Enemies_itemSoundCount'));ids=[m.rd8(addr('Enemies_itemSounds')+i) for i in range(count)]
  assert ids==([23,13] if body else []),('sound requests',body,ids)
  if body:
   from intro_harness import IntroHarness
   from probe import load_rom,reference_rom
   oracle=IntroHarness(load_rom(reference_rom()));oracle.hooks.pop(0x509F);oracle.cpu.clear_breakpoint(0x509F);oracle.cpu.memory[0xF0F1:0xF0F4]=bytes((1,2,3));oracle.call(0x5085)
   for sound in ids:oracle.cpu.a=sound;oracle.call(0x509F)
   assert m.rd8(psg+22)==oracle.cpu.memory[0xC096]==23
  assert m.rd16(psg+24)-accepted_before==(1 if body else 0),('accepted sound count',body,accepted_before,m.rd16(psg+24))
  assert m.rd8(q)==0 and (not body or m.rd8(e+14)==0)
  m.run(360,set());assert m.rd32(addr('vk_hud_score'))==score and m.rd8(q)==0
  m.image().save(a.out/f'touch09-{int(body)}.png');cases.append({'body':body,'score':score,'sounds':ids,'continuation':360})
 report={'frames':m.frame,'cases':cases,'scope':'stage13/3 controlled RAM fixtures; no natural traversal','passed':True}
 (a.out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print('Touch09 MD PASS',report)
if __name__=='__main__':main()
