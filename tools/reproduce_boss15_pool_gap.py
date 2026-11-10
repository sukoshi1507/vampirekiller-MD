#!/usr/bin/env python3
"""Boss15/shared-pool regression: actual candle vs ROM, report discrepancies."""
import argparse,json
from pathlib import Path
from probe import ROOT
from md_harness import Machine,Game_View,load_symbols,lookup,BUTTONS

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--free-third',action='store_true');ap.add_argument('--core',required=True);ap.add_argument('--out',type=Path,default=Path('scratchpad/loot-md'));a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
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
 #Real reference room and whip, frozen body and its three projectiles.
 from probe import load_rom,reference_rom
 from verify_item_trace import setup
 h=setup(load_rom(reference_rom()),15,9);z=h.cpu.memory;z[0xCE00]=5;z[0xC418]=128;z[0xC002]=64;z[0xD010]=1;h.call(0x63C5)
 for tick in range(4):
  z[0xC003]=tick+1;z[0xC427]=88;z[0xC425]=176
  for fn in (0xB6AB,0x98E5,0x9E31,0x9918,0x64EC,0x64E5):h.call(fn)
 assert [z[0xD700+128*i] for i in range(8)]==[8,8,8,0,0,0,0,0]
 assert all(z[0xD638+4*i]!=224 for i in range(18))
 if a.free_third:h.cpu.ix=0xD800;h.call(0x99F6)
 z[0xC427]=20;z[0xC425]=114;z[0xC420]=0;z[0xC42C]=0;z[0xC416]=1;z[0xC422]=3
 for fn in (0x999F,0x7D68,0x8671):h.call(fn)
 expected_ordinary=1 if a.free_third else 0
 assert z[0xC470]==0
 assert sum(bool(z[0xC800+128*i]) for i in range(1,7))==expected_ordinary
 assert bool(z[0xC500])==(not a.free_third)
 entry(15,9);
 if a.free_third:
  assert lookup(syms,'vk_boss')[1]==192;m.wr(addr('vk_boss')+116,0) #extra_shot[0], m68k offset retained
 m.wr(e+162,1);pin(20,114);m.run(4,set());before=m.rd16(addr('late_enemy_count'));m.image().save(a.out/'boss-before-candle.png')
 for frame in range(20):
  pin(20,114);m.wr(addr('vk_whip')+5,0);m.run(1,{BUTTONS['B']} if frame<2 else set())
  if not(m.rd8(c+4)&1):break
 assert not(m.rd8(c+4)&1),'actual MD candle must be hit'
 for _ in range(4):pin(20,114);m.run(1,set())
 ordinary=[m.rd8(e+18*i+2) for i in range(7) if m.rd8(e+18*i)];after=m.rd16(addr('late_enemy_count'));m.image().save(a.out/'boss-after-candle.png')
 raw=addr('fallback_raw')
 native_fallback=bytes(m.rd8(raw+i) for i in range(16))
 assert bool(native_fallback[0])==(not a.free_third),('fallback allocation',native_fallback.hex())
 if not a.free_third:
  # Four MD images elapsed after the hit: two logical C500 passes.
  h.call(0x8A4A);h.call(0x8A4A)
 if not a.free_third:assert native_fallback==bytes(z[0xC500:0xC510]),('C500 IX16',native_fallback.hex(),bytes(z[0xC500:0xC510]).hex())
 if a.free_third:assert m.rd8(e)==0 and m.rd8(e+18)==1,'body owns physical C8000, loot must be C8001'
 correct=len(ordinary)==expected_ordinary and after==9
 if a.free_third:
  cache=addr('late_enemies');tiles=[m.rd16(cache+8*i+4)&2047 for i in range(after)]
  assert all(t<1440 for t in tiles[:6]) and 1440<=tiles[6]<1460 and all(t<1440 for t in tiles[7:]),('body-loot-projectile priority',tiles)
 continuation_min=after
 for _ in range(360):m.run(1,set());continuation_min=min(continuation_min,m.rd16(addr('late_enemy_count')))
 assert continuation_min>=(8 if a.free_third else 9),('boss must remain drawn during loot continuation',continuation_min)
 report={'passed':correct,'reference':{'C800_types':[z[0xC800+128*i] for i in range(7)],'D700_types':[z[0xD700+128*i] for i in range(8)],'C500_fallback':bytes(z[0xC500:0xC510]).hex(),'all18_SAT_reserved_before_free':True,'free_third':a.free_third},'native':{'ordinary_C800_types':ordinary,'sprite_count_before':before,'sprite_count_after':after,'C500_initial':native_fallback.hex(),'continuation':360,'min_sprites_continuation':continuation_min},'scope':'actual stage15/9 candle; controlled freeze and Simon position; no natural traversal'}
 (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('Boss15 pool integration', 'PASS' if correct else 'FAIL',report)
 if not correct:raise SystemExit(1)
if __name__=='__main__':main()
