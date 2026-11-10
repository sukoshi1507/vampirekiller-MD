#!/usr/bin/env python3
"""LOT14E core: compiled C800 SAT alias and initial09 glyph, RAM fixtures."""
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
 entry(13,0);q=addr('vk_projectiles')
 for off,size in [(e,126),(q,144),lookup(syms,'sat_used'),lookup(syms,'c800_sat'),lookup(syms,'d700_sat')]:
  for i in range(size):m.wr(off+i,0)
 pin(16,176);m.wr(e,1);m.wr(e+2,22);m.wr(e+3,2);m.wr(e+4,128,16);m.wr(e+6,0);m.wr(e+10,1,16);m.wr(e+14,1);m.wr(e+16,0x21);m.wr(e+145,255);m.wr(e+162,1)
 m.run(360,set());assert m.rd8(e)==1 and m.rd32(addr('sat_used'))==12,('active alias mask',m.rd8(e),m.rd32(addr('sat_used')))
 m.image().save(a.out/'active09-alias.png')
 m.wr(e+6,96);m.wr(e+14,0);m.wr(e+16,0);m.run(360,set());assert m.rd8(e)==1 and m.rd32(addr('sat_used'))==15
 #Source E8 bitmap, CC02|05=07, common19-stage glyph generated in loot bank4.
 from probe import load_rom,reference_rom
 from build_boss12_md import BossGraphics
 from extract_rooms import decode_room
 from verify_viewer import gx_rgb565
 rom=load_rom(reference_rom());h=BossGraphics(rom);h.cpu.memory[0xD000]=13;h.cpu.memory[0xD001]=0;h.cpu.memory[0xD002]=rom[0x1E6A+13];h.cpu.memory[0xF0F1:0xF0F4]=bytes((1,2,3));h.call(0x5768);h.call(0x5780)
 palette=[gx_rgb565(rgb) for rgb in decode_room(rom,13,0)[0]['palette_rgb333']];im=m.image();pixels=0
 for yy in range(16):
  bits=h.vram[0xFF40+yy]*256+h.vram[0xFF50+yy]
  for xx in range(16):
   if bits&(0x8000>>xx):
    assert im.getpixel((124+xx,89+yy))==palette[7],('initial09 pixel',xx,yy,im.getpixel((124+xx,89+yy)),palette[7]);pixels+=1
 assert pixels>0;im.save(a.out/'initial09-glyph.png')
 report={'frames':m.frame,'alias_mask':12,'constructor_mask':15,'glyph_pixels':pixels,'continuation_each':360,'scope':'stage13/0 controlled RAM fixtures, no natural traversal','passed':True}
 (a.out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print('C800 SAT MD PASS',report)
if __name__=='__main__':main()
