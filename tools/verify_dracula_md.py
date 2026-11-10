#!/usr/bin/env python3
"""Isolated final-boss SGDK check in Genesis Plus GX; RAM entry bypasses LOT11 traversal."""
import argparse,json
from pathlib import Path
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS
from probe import ROOT
from verify_viewer import gx_rgb565
from build_dracula_background import generate

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,default=ROOT/'validation/dracula-md');args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
 symbols=load_symbols(elf=next((e for e in (ROOT/'sgdk/out/rom.out',ROOT/'sgdk/out/release/rom.out') if e.exists()),ROOT/'sgdk/out/rom.out'));m=Machine(args.core,ROOT/'sgdk/out/rom.bin');view=Game_View(m,symbols)
 def addr(name):return lookup(symbols,name)[0]
 g,d,w=addr('vk_game'),addr('dracula'),addr('vk_whip');ce=d+480
 assert lookup(symbols,'dracula')[1]==688
 def read(off):return m.rd8(d+off)
 def mode():return m.rd16(addr('vk_front_mode'))
 def run_until(predicate,limit):
  for _ in range(limit):
   if predicate():return
   m.run(1,set())
  raise AssertionError(('timeout',m.frame,mode(),[m.rd8(ce+i) for i in (1,21,64)],read(608)))
 def pin(x=16,y=160):
  m.wr(g,x);m.wr(g+1,y);m.wr(g+2,0);m.wr(g+3,0);m.wr(g+4,0);m.wr(g+26,0);m.wr(g+30,0)
  h=addr('health');m.wr(h+1,0);m.wr(h+2,0,16);m.wr(addr('Enemies_god'),1)
 def tick(n=1):
  for _ in range(n):
   if not m.rd8(ce+64):pin()
   m.run(1,set())
 run_until(lambda:mode()==1,400);m.run(4,{BUTTONS['START']});m.run(4,set());m.run(4,{BUTTONS['START']});m.run(6,set());run_until(lambda:mode()==3,100)
 m.wr(g+16,1,32);m.wr(g+20,9);m.wr(g+22,18);m.wr(addr('vk_front_mode'),4,16);pin();tick(180)
 assert m.rd8(d)==17 and m.rd8(ce+1)==1 and read(608)==128
 m.image().save(args.out/'first-form.png')
 _,assets=generate();image=m.image();palette=[gx_rgb565(((m.rd8(d+634+2*i)>>4)&7,m.rd8(d+635+2*i)&7,m.rd8(d+634+2*i)&7)) for i in range(16)]
 # Stable room frame excluding the small active combatants on the right. These
 # pixels include the previously missing192..245 patterns and both face patches.
 mapping=assets['maps'][read(676)*3+read(677)];checks=0
 for y in range(40,176):
  for x in range(32,160):
   i=(y//8-4)*32+x//8;t=mapping[i];pixel=assets['words'][t*8+y%8]>>(28-4*(x%8))&15
   assert image.getpixel((x,y))==palette[pixel],('MD room pixel',x,y,image.getpixel((x,y)),palette[pixel]);checks+=1
 def kill_boss():
  for _ in range(500):
   if m.rd8(ce+21):return
   if m.rd8(d+14)&1:
    pin((m.rd8(d+5)-20)&255,(m.rd8(d+3)-(32 if m.rd8(d)==17 else 0))&255)
    m.wr(d+608,4);m.wr(w,12);m.wr(w+1,0);m.wr(w+2,0);m.wr(w+3,m.rd8(g));m.wr(w+4,m.rd8(g+1));m.wr(w+5,0)
    m.run(1,set())
   else:tick()
  raise AssertionError('actual whip callback did not defeat boss')
 kill_boss();assert m.rd32(addr('vk_hud_score'))==30000
 for _ in range(700):
  if m.rd8(ce+1)==6:break
  tick()
 assert m.rd8(d)==23 and read(608)==128 and m.rd8(ce+1)==6
 tick(8);m.image().save(args.out/'second-form.png');kill_boss();assert m.rd32(addr('vk_hud_score'))==80000
 for _ in range(1600):
  if m.rd8(ce+64)==2 and m.rd8(ce+49)>0:break
  tick()
 assert m.rd8(ce+64)==2 and m.rd8(ce+49)>0
 tick(320);m.image().save(args.out/'credits.png')
 assert sum(1 for pixel in m.image().get_flattened_data() if max(pixel)>200)>50, 'credits text must scroll into view'
 from dracula_graphics import room
 reference=room();reference.call(0x47B1);reference.cpu.memory[0xCE40]=1;reference.call(0x66BA)
 target_line=m.rd8(ce+49);scroll=m.rd8(ce+51)
 for p in range(6000):
  reference.cpu.memory[0xC003]=(p+1)&255;reference.call(0x66BA)
  if reference.cpu.memory[0xCE31]==target_line and reference.cpu.memory[0xCE33]==scroll:break
 else:raise AssertionError('credit reference state not reached')
 actual=[max(pixel)>200 for pixel in m.image().get_flattened_data()]
 errors=[]
 for delta in (-1,0,1):
  expected=[reference.pixel(x,(y+scroll+delta)&255)==14 for y in range(224) for x in range(256)]
  errors.append(sum(a!=b for a,b in zip(actual,expected)))
 assert min(errors)==0,('credit scroll pixels',target_line,scroll,errors)

 # The first real completion starts the courtyard and preserves the score.
 for _ in range(15000):
  if mode()==3:break
  tick()
 assert mode()==3 and m.rd32(g+16)==0
 assert m.rd32(addr('vk_hud_score'))==80000 and m.rd8(g+16+6)==1
 m.run(40,set());m.image().save(args.out/'new-cycle.png')
 assert sum(1 for pixel in m.image().get_flattened_data() if max(pixel)>40)>1000, 'courtyard must be visible'
 # Re-enter only the isolated boss room for the second completion. The normal
 # stage12..18 traversal remains intentionally outside this test.
 m.wr(g+16,1,32);m.wr(g+20,9);m.wr(g+22,18);m.wr(addr('vk_front_mode'),4,16);pin();tick(180)
 assert m.rd8(d)==17 and read(608)==128
 kill_boss()
 for _ in range(700):
  if m.rd8(ce+1)==6:break
  tick()
 assert m.rd8(d)==23;kill_boss()
 for _ in range(15000):
  if mode()==1:break
  tick()
 assert mode()==1 and m.rd32(addr('vk_hud_score'))==160000
 m.run(20,set());m.image().save(args.out/'second-clear-title.png')
 assert m.image().getpixel((16,40))==gx_rgb565((0,0,7)), 'title palette must be restored'
 report={'passed':True,'scope':'RAM entry into stage18/room9; no traversal validation','frames':m.frame,'room_pixels':checks,'credits_pixels':256*224,'score':160000,'completions':2,'core':str(Path(args.core).resolve())}
 (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('Dracula MD: room pixels, actual whip callbacks, both forms, credits and courtyard/title returns PASS',report);m.close()
if __name__=='__main__':main()
