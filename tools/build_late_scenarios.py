#!/usr/bin/env python3
"""LOT12: controller debug-menu room tour, real edge/whip/door, explicit RAM fixtures."""
import argparse,json
from probe import ROOT,load_rom,reference_rom
from decode_room_links import decode
from castle_room_objects import decode_room
from extract_stage_exits import extract
from verify_item_trace import setup

KONAMI=['tap '+k for k in ('UP','UP','DOWN','DOWN','LEFT','RIGHT','LEFT','RIGHT','B','A')]
MAP_CACHE={}
def safe_point(stage,room,x,y):
 rom=load_rom(reference_rom())
 if (stage,room) not in MAP_CACHE:
  h=setup(rom,stage,room)
  for _ in range(20):h.call(0x8671)
  MAP_CACHE[stage,room]=bytes(h.cpu.memory[0xD100:0xD400])
 tiles=MAP_CACHE[stage,room];limit=rom[0x3C78+stage]
 def solid(x,y):
  i=(((y-16)&248)<<2)+(x>>3)
  return i<768 and ((tiles[i]-1)&255)<limit
 points=[(xx,yy) for yy in range(64,209,8) for xx in range(16,241,8) if (solid(xx-5,yy) or solid(xx+5,yy)) and not any(solid(xx+dx,yy-dy) for dx in (-8,8) for dy in (2,10,18,26))]
 assert points,(stage,room,'supported fixture')
 return min(points,key=lambda p:abs(p[0]-x)+abs(p[1]-y))
def load(stage,room,x=88,y=176):
 x,y=safe_point(stage,room,x,y)
 return [f'# RAM fixture for action only: stage{stage}/room{room}, not a walked route.', 'poke vk_game+16 1 32',f'poke vk_game+22 {stage}',f'poke vk_game+20 {room}','poke vk_front_mode 4 16',f'poke vk_game+0 {x}',f'poke vk_game+1 {y}']+['poke vk_game+'+str(o)+' 0' for o in (2,3,4,7,8,23,26,30)]+['poke vk_candles 255','poke health+1 0','poke health+2 0 16','poke Enemies_god 1','poke vk_enemies+162 1','poke items+2 0','poke items+3 0','poke freeze_drawn 0','poke room_freeze 4','poke room_freeze+1 4','poke room_freeze+2 0',"write freeze_vtimer 32 ram32('vtimer')",'until ram8(\'room_freeze\') == 0 and ram8(\'freeze_drawn\') != 0 max 100','wait 30']
def generate():
 rom=load_rom(reference_rom());graphs=decode(rom);exits=extract(rom);fixed=json.loads((ROOT/'research/late-fixed-enemies-z80.json').read_text())['stages'];out={}
 out['zzp_next.inc']=['# Actual pause/Konami/room selector; this resets the game for the target room.','until game.player.mode == 0 max 400','tap START','until ram8(\'paused\') == 1 max 30',*KONAMI,'tap RIGHT','tap B']
 for stage in range(12,19):
  graph=graphs[stage];entry=exits[stage-1];door=entry['door'];lines=[f'# LOT12 stage{stage}: all{len(graph["rooms"])} rooms via controller debug selector.', '# Not a continuous walk: room selection restarts the game. Action fixtures below are explicitly RAM-set.',"until front_mode == MODES['TITLE'] max 2000",'tap START','tap START',"until front_mode == MODES['COURTYARD'] max 500",'wait 60','tap START',"until ram8('paused') == 1 max 30",*KONAMI,*(['tap DOWN']*(stage-1)),'tap B',f'until game.stage == {stage} and game.room == 0 max 300']
  for room in range(len(graph['rooms'])):
   if room:lines+=['include zzp_next.inc',f'until game.stage == {stage} and game.room == {room} max 300']
   lines+=['poke Enemies_god 1','poke vk_enemies+162 1','until game.player.mode == 0 max 300','wait 30','set clk enemies.tick','wait 300',f'expect game.stage == {stage} and game.room == {room}',"expect enemies.tick == (clk+150)%256",f'expect ram8(\'vk_candles\') == {stage} and ram8(\'vk_candles+1\') == {room}',f'shot zzp{stage}-room{room}',f'dump s{stage}r{room}']
  # A horizontal real room edge (placed at its boundary, then use the controller).
  src,target,direction=next((r,q[name],d) for r,q in enumerate(graph['rooms']) for name,d in (('RIGHT',1),('LEFT',-1)) if q[name] is not None and r!=door['room'])
  lines+=load(stage,src,248 if direction>0 else 8,128)
  lines+=['poke vk_game+0 '+str(248 if direction>0 else 8),'poke vk_game+1 128','poke vk_game+3 0','poke vk_game+8 0',f'until game.room == {target} max 120 '+('RIGHT' if direction>0 else 'LEFT'),f'expect game.stage == {stage}',f'shot zzp{stage}-edge-{src}-{target}']
  candle=next((r,i,bytes.fromhex(ob['raw'])) for r in range(len(graph['rooms'])) for i,ob in enumerate(decode_room(rom,stage,r)['candles']) if bytes.fromhex(ob['raw'])[4]==0 and 16<=ob['x']<=224 and ob['y']<=176)
  r,index,b=candle;lines+=load(stage,r,b[2]-12,b[1]+28);lines += [f'expect ram8(\'vk_candles+4\') & {1<<index} != 0',f'poke vk_game+0 {b[2]-12}',f'poke vk_game+1 {b[1]+28}','poke vk_game+2 0','poke vk_game+3 0','poke vk_whip 0','tap B 2',f'until ram8(\'vk_candles+4\') & {1<<index} == 0 max 40',f'shot zzp{stage}-candle']
  enemies=[(r,k,q) for r,rs in enumerate(fixed[stage-12]['rooms']) for k,q in enumerate(rs) if (q['raw_type']&127) not in (0,9) and 16<=((q['raw'][1]&15)*16)<=224 and (q['raw'][1]&240)<=192]
  r,slot,q=enemies[0];native=sum(bool(e['raw_type']) for e in fixed[stage-12]['rooms'][r][:slot]);x=(q['raw'][1]&15)*16;y=q['raw'][1]&240
  lines+=load(stage,r,max(0,x-20),y+8);lines += [f'until enemies.enemy[{native}].active max 100',f'poke vk_enemies+{native*18+3} 1','poke Enemies_s7R 1 16',f'poke vk_game+0 {max(0,x-20)}',f'poke vk_game+1 {y+8}','poke vk_game+2 0','poke vk_game+3 0','poke vk_whip 0','tap B 2',f'until enemies.enemy[{native}].flame == 1 max 40',f'shot zzp{stage}-enemy-killed']
  lines+=load(stage,door['room'],20 if door['x']<128 else 228,door['y']+16)
  lines+=['poke items 1','poke vk_game+2 '+str(int(door['x']<128)),'poke vk_game+0 '+str(20 if door['x']<128 else 228),'poke vk_game+1 '+str(door['y']+16),'poke vk_game+3 0',"until ram8('items+2') == 1 max 60 "+('LEFT' if door['x']<128 else 'RIGHT'),"expect ram8('items') & 1 == 0",f'shot zzp{stage}-door']
  if stage in (12,15,18):
   bossroom={12:6,15:9,18:9}[stage];lines += [f'until game.room == {bossroom} max 500',f'expect game.stage == {stage}','wait 60',"expect ram8('vk_boss') != 0",f'shot zzp{stage}-boss-entry']
  else:lines +=[f'until game.stage == {stage+1} max 500',f'expect game.room == 0',f'shot zzp{stage}-next-stage']
  if stage==18:lines+=['include zzp_dracula_finish.inc']
  out[f'zzp_stage{stage}.txt']=lines
 # Transparent adaptive fixture driving the existing actual whip callbacks.
 pin=['when ram8(\'dracula+544\') == 0 :: poke vk_game+0 16','when ram8(\'dracula+544\') == 0 :: poke vk_game+1 160']+[f"when ram8('dracula+544') == 0 :: poke vk_game+{off} 0" for off in (2,3,4,26,30)]+['poke Enemies_god 1','wait 1']
 out['zzp_dracula_pin.inc']=pin
 for raw,form in ((17,1),(23,2)):
  condition=f"ram8('dracula') == {raw} and ram8('dracula+14') & 1 != 0"
  commands=["write vk_game+0 8 (ram8('dracula+5')-20)%256",f"write vk_game+1 8 (ram8('dracula+3')-{32 if raw==17 else 0})%256",'poke vk_game+2 0','poke vk_game+3 0','poke vk_game+4 0','poke vk_game+26 0','poke vk_game+30 0','poke dracula+608 4','poke vk_whip 12','poke vk_whip+1 0','poke vk_whip+2 0',"write vk_whip+3 8 ram8('vk_game')","write vk_whip+4 8 ram8('vk_game+1')",'poke vk_whip+5 0']
  fallback=[f"when not({condition}) and ram8('dracula+544') == 0 :: poke vk_game+{off} {value}" for off,value in ((0,16),(1,160),(2,0),(3,0),(4,0),(26,0),(30,0))]
  out[f'zzp_dracula_hit{form}.inc']=['# Test fixture: reduce boss energy and initialise a real active whip, no opcode patches.','poke health+1 0','poke health+2 0 16',*fallback]+[f'when {condition} :: {c}' for c in commands]+['poke Enemies_god 1','wait 1']
 out['zzp_dracula_finish.inc']=['# Isolated end proof after the real stage18 door; no full-health fight claim.','poke vk_enemies+162 0','set before_final_score ram32(\'vk_hud_score\')','repeat 500 zzp_dracula_hit1.inc','repeat 700 zzp_dracula_pin.inc',"expect ram8('dracula') == 23",'repeat 500 zzp_dracula_hit2.inc',"until ram8('dracula+544') == 2 and ram8('dracula+529') > 0 max 4000",'wait 320','shot zzp18-credits',"until front_mode == MODES['COURTYARD'] max 16000","expect ram32('vk_hud_score') >= before_final_score+80000",'shot zzp18-ending-courtyard']
 return {n:'\n'.join(lines)+'\n' for n,lines in out.items()}
def main():
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args()
 for n,t in generate().items():
  f=ROOT/'tests/md_scenarios'/n
  if a.check:assert f.read_text()==t,n
  else:f.write_text(t)
 print('LOT12 seven stage scenarios and explicit action fixtures','CHECK PASS' if a.check else 'generated')
if __name__=='__main__':main()
