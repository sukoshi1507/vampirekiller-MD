#!/usr/bin/env python3
"""Late wall/merchant state versus executed9179/91BE/9387/94BA, unmodified ROM."""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from build_boss12_md import BossGraphics
from build_trade_data import collect

class TradeHarness(BossGraphics):
 def __init__(self,rom):
  super().__init__(rom);self.random=[];self.frames=[-1,-1];self.keys=0;self.sounds=[];self.exit=False;self.stop=None
  self.cpu.set_breakpoint(0x92EA);self.cpu.set_breakpoint(0x925E)
  self.hooks[0x509F]=lambda:self.sounds.append(self.cpu.a)
  self.hooks[0x0096]=lambda:setattr(self.cpu,'a',255^((self.keys&3)<<4))
  self.hooks[0x0141]=lambda:setattr(self.cpu,'a',255)
  self.hooks[0x41B6]=self.leave_shop
  for a in self.hooks:self.cpu.set_breakpoint(a)
 def clear_room_objects(self):
  # Execute the real6382..638D LDIR, stopping before the call to5B1B.
  self.stop=0x638F;self.cpu.set_breakpoint(self.stop)
  try:self.call(0x6382)
  finally:self.cpu.clear_breakpoint(self.stop);self.stop=None
 def leave_shop(self):self.cpu.memory[0xC000]=self.cpu.a;self.exit=True
 def call(self,address,budget=5000000):
  c=self.cpu;c.sp=0xF100;c.memory[0xF100:0xF102]=bytes([0,1]);c.pc=address;c.ticks_to_stop=budget
  for _ in range(100000):
   c.run()
   if c.pc==0x0100 or c.pc==self.stop:return
   if c.pc==0x92EA:self.random.append(c.a);c.step_over_breakpoint()
   elif c.pc==0x925E:
    k=(c.ix-0xC5B5)//16
    if 0<=k<2:self.frames[k]=c.a
    c.step_over_breakpoint()
   elif c.pc in self.hooks:
    self.hooks[c.pc]();c.pc=c.memory[c.sp]|c.memory[(c.sp+1)&65535]<<8;c.sp=(c.sp+2)&65535
   elif not c.ticks_to_stop:raise RuntimeError(f'budget {c.pc:04X} calling {address:04X}')
  raise RuntimeError('too many stops')

def setup(rom,stage,room,hidden=None):
 h=TradeHarness(rom);m=h.cpu;mem=m.memory
 mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage];mem[0xF0F1:0xF0F4]=bytes([1,2,3])
 h.call(0x5A49);h.call(0x5B1B);h.call(0x8671)
 if hidden is not None:mem[hidden+3]=1;h.call(0x8671)
 h.call(0x91BE);mem[0xC002]=64;mem[0xC415]=32
 return h

def values(lib):
 out=(C.c_int*15)();lib.trade_values(out);return list(out)
def metadata(lib,h,codes):
 for i in range(2):
  a=0xC5B5+16*i
  if h.cpu.memory[a]:lib.trade_metadata(i,h.cpu.memory[a+7]*256+h.cpu.memory[a+8],codes[i])
def compare(lib,h,label,p):
 mem=h.cpu.memory
 out=(C.c_ubyte*32)()
 for k in range(2):
  one=(C.c_ubyte*16)();lib.trade_record(k,one);out[16*k:16*k+16]=one
 raw=bytes(mem[0xC5B5:0xC5D5]);assert bytes(out)==raw,(label,p,'records',raw.hex(),bytes(out).hex())
 v=values(lib);base=0xDE00+(0 if mem[0xD000]==0 else (0,64,128)[(mem[0xD000]-1)%3])+4*mem[0xD001]
 expected=[mem[0xC70C],mem[0xC70B],(mem[0xC417]>>4)*10+(mem[0xC417]&15),mem[0xD012],mem[base+1],mem[base+3]]
 assert v[:6]==expected,(label,p,'state',v[:6],expected)
 for k in range(2):
  expected_frame=h.frames[k] if mem[0xC5B5+16*k] else -1
  assert v[13+k]==expected_frame,(label,p,'frame',k,v[13+k],expected_frame)
 return v

def play_shop(lib,h,label,answer=1,books=0,hearts=99,held=0):
 mem=h.cpu.memory;lib.trade_inventory(hearts,books);mem[0xC417]=(hearts//10)*16+hearts%10;mem[0xC702]=books
 lib.trade_clock(mem[0xC003]);mem[0xC000]=12;mem[0xC001]=0;h.exit=False
 frames=0; simon=bytes(mem[0xC424:0xC428]); sprites=bytes(h.vram[0xF400:0xF680])
 while not h.exit and frames<700:
  keys=held if frames<2 else answer if frames==3 else 0
  h.keys=keys;mem[0xC003]=(mem[0xC003]+1)&255;h.sounds.clear()
  if frames==0:h.call(0x440A)
  elif mem[0xC001]==1:h.call(0x43F2)
  else:h.call(0x43FD)
  lib.trade_shop(keys);v=compare(lib,h,label,frames)
  assert v[6]==(0 if h.exit else 2 if mem[0xC001]==1 else 3),(label,'shop phase',frames,v[6],mem[0xC001])
  assert bytes(mem[0xC424:0xC428])==simon and bytes(h.vram[0xF400:0xF680])==sprites,(label,"frozen Simon/sprites",frames)
  assert v[11]==mem[0xC709],(label,"C709",frames,v[11],mem[0xC709])
  if not h.exit:
   assert v[7:10]==list(mem[0xC708:0xC709])+[mem[0xC707],mem[0xC706]],(label,'sale/price/timeout',frames,v[7:10],mem[0xC707])
  if v[6]==3:assert v[10]==mem[0xC004]
  sounds=(C.c_ubyte*2)();n=lib.trade_sounds(sounds)
  # 8D30 returns a sound in A, without509F;94EF overwrites A with12.
  actual=list(sounds)[:n]
  assert actual==h.sounds,(label,'sounds',frames,actual,h.sounds)
  given=(C.c_int*7)();lib.trade_given(given)
  assert list(given)==[mem[0xC419],mem[0xC416],mem[0xC701]&248,mem[0xC431],mem[0xC702],mem[0xC700],int(bool(mem[0xC701]&4))],(label,'given',list(given),list(mem[0xC700:0xC703]))
  frames+=1
 assert h.exit and frames>=15
 return frames

def graphics(lib,rom,cases):
 checks=0;max_tiles=0
 for stage,room,x,y,code,address in cases:
  h=TradeHarness(rom);mem=h.cpu.memory
  mem[0xF0F1:0xF0F4]=bytes([1,2,3]);mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage]
  for entry in (0x548D,0x53B6,0x564C,0x570D,0x5768,0x5780,0x4F83,0x4F91,0x5A49,0x5B1B,0x8671):h.call(entry)
  if address is not None:mem[address+3]=1;h.call(0x8671)
  h.call(0x91BE)
  # Wall objects are bitmap copies925E, not6445 enemy sprites on the MSX.
  for frame in range(5):
   h.cpu.ix=0xC5B5;h.cpu.de=(x<<8)|y;h.cpu.a=frame;h.call(0x925E)
   out=(C.c_int*8)();pixels=(C.c_ubyte*768)();lib.trade_view(stage,x,y,frame,out,pixels)
   assert list(out)==[1,12,x,y+8,14,208,13,1],('sprite mapping',stage,room,list(out))
   expected=bytes(h.pixel(x+c,y+8+r) for r in range(24) for c in range(32))
   #925E uses transparency: compare the drawn frame pixels to its source page;
   # blank pixels retain room bitmap on MSX and are transparent MD sprite pixels.
   source=bytes(h.pixel(frame*32+c,424+r) for r in range(24) for c in range(32))
   assert bytes(pixels)==source,('wall pixels',stage,room,frame)
   assert all(not v or v==expected[i] for i,v in enumerate(pixels))
   checks+=768
  mem[0xC703]=0
  base=0xDE00+(0,64,128)[(stage-1)%3]+4*room
  # Allnine products, three price modes plus the combined-bit priority case.
  for product in range(9):
   sale=rom[0x5428+4*product];mem[base]=sale
   for books in (0,64,128,192):
    mem[0xC702]=books;before=bytes(h.vram[:32768]);h.call(0x9387)
    wy,wx=mem[0xC704],mem[0xC705];coord=(C.c_ubyte*2)();lib.trade_view_window(x,y,coord)
    assert list(coord)==[wx,wy],('window coordinates',stage,room,list(coord),wx,wy)
    pixels=(C.c_ubyte*2560)();count=lib.trade_compose(sale,mem[0xC707],pixels);max_tiles=max(max_tiles,count);assert count<=34
    expected=bytes(h.pixel(wx+c,wy+r) for r in range(32) for c in range(80))
    assert bytes(pixels)==expected,('merchant pixels',stage,room,sale,books,next((i for i,(a,b) in enumerate(zip(pixels,expected)) if a!=b),None))
    h.call(0x950D);assert bytes(h.vram[:32768])==before,('window restoration',stage,room,sale,books)
    checks+=2560
 print('Trade graphics',checks,'pixels;5 wall frames,504 windows/restorations;max',max_tiles,'tiles PASS',flush=True)
 return checks,max_tiles

def legacy_window_reference(rom):
 h=TradeHarness(rom);mem=h.cpu.memory
 mem[0xF0F1:0xF0F4]=bytes([1,2,3]);mem[0xD000]=2
 for entry in (0x548D,0x53B6,0x564C,0x570D,0x5768,0x5780,0x4F83,0x4F91,0x5A49,0x5B1B,0x8671):h.call(entry)
 assert mem[0xC4E9]==0xCE
 mem[0xC4E3]=1;h.call(0x8671);h.call(0x91BE);mem[0xC703]=1;h.call(0x9387)
 assert list(mem[0xC704:0xC706])==[136,160]
 assert [h.pixel(x,y) for x,y in ((161,136),(161,137),(164,150))]==[14,0,0]
 print('Trade legacy9397: borderY136, black interiorY137;zo_merchant coordinate proof PASS',flush=True)

def boxes(lib,rom,cases):
 checks=0
 for stage,room,x,y,code,address in cases:
  h=setup(rom,stage,room,address);mem=h.cpu.memory;lib.trade_init(stage,room)
  if address is not None:lib.trade_block((address-0xC470)//16,x,y,code)
  record=bytes(mem[0xC5B5:0xC5D5])
  for weapon in (0,1):
   for facing in (0,1):
    for crouched in (0,1):
     mem[0xC416]=weapon;mem[0xC42C]=facing;mem[0xC420]=2 if crouched else 0
     for sx in range(max(0,x-48),min(256,x+65),4):
      for sy in range(max(0,y+22),min(256,y+59)):
       mem[0xC427]=sx;mem[0xC425]=sy;h.cpu.hl=0xC5B5;h.call(0x83A1)
       actual=lib.trade_whip(sx,sy,facing,crouched,weapon)
       assert actual==bool(h.cpu.f&1),('whip box',stage,room,weapon,facing,crouched,sx,sy,actual,h.cpu.f)
       assert bytes(mem[0xC5B5:0xC5D5])==record
       checks+=1
 print('Trade whip boxes',checks,'executed83A1 comparisons PASS',flush=True)
 return checks

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom,counts,defs,_,_,blocks=collect();reports=[]
 cases=[(s,r,x,y,code,None) for x,y,code,s,r in defs if s>=12]+[(s,r,x,y,code,address) for s,r,address,x,y,code in blocks if s>=12]
 # All74 late rooms, including the60 without a wall/merchant, execute the decoder.
 for s in range(12,19):
  for r in range(counts[s]):
   h=setup(rom,s,r);lib.trade_init(s,r);codes=[0,0]
   for x,y,code,st,room in defs:
    if (st,room)==(s,r):codes[0]=code
   metadata(lib,h,codes);compare(lib,h,'initial',r)
 for s,r,x,y,code,address in cases:
  h=setup(rom,s,r,address);mem=h.cpu.memory;lib.trade_init(s,r)
  if address is not None:assert lib.trade_block((address-0xC470)//16,x,y,code)==0
  metadata(lib,h,[code,0]);mem[0xC417]=0x50;lib.trade_inventory(50,0);score=0;images=0;hits=0
  for p in range(600):
   mem[0xC003]=(mem[0xC003]+1)&255;mem[0xC427]=64;mem[0xC425]=160
   if mem[0xC5B5]==0x82 and hits<8:
    mem[0xC5B8]=1;lib.trade_hit(0);hits+=1
   h.random.clear();h.sounds.clear();h.call(0x91BE);lib.trade_pass(h.random[0] if h.random else 0)
   v=compare(lib,h,f'{s}:{r}',p);images+=2;score+=lib.trade_points()
   actual=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(mem[0xC405:0xC408]));assert actual==score,(s,r,'score',actual,score)
   sounds=(C.c_ubyte*2)();n=lib.trade_sounds(sounds);assert list(sounds)[:n]==h.sounds,(s,r,p,'world sounds',list(sounds)[:n],h.sounds)
   if mem[0xC40C]:
    images+=play_shop(lib,h,f'{s}:{r}',1,0,99);mem[0xC40C]=0
  assert hits==8 and score==5000 and mem[0xC5B5]==0,(s,r,hits,score)
  if address is not None:assert not lib.trade_present((address-0xC470)//16)
  reports.append({'stage':s,'room':r,'hidden':address is not None,'images':images,'hits':hits,'score':score})
  print('Trade',s,r,images,'images PASS',flush=True)
 # All14 positions: refusal, poverty, timeout, four price-book variants and
 # held opening keys; every scenario runs at least300 additional world images.
 extra=0
 for s,r,x,y,code,address in cases:
  for name,answer,books,hearts,held in [('no',2,0,99,0),('poor',1,0,0,0),('timeout',0,0,99,0),('discount',1,128,99,0),('expensive',1,64,99,0),('both-books',1,192,99,0),('held-open',0,0,99,1)]:
   h=setup(rom,s,r,address);mem=h.cpu.memory;lib.trade_init(s,r)
   if address is not None:lib.trade_block((address-0xC470)//16,x,y,code)
   metadata(lib,h,[code,0]);lib.trade_inventory(50,0);mem[0xC417]=0x50
   # Some rows reach the merchant after several hits. Execute the real sequence.
   for p in range(400):
    if mem[0xC5B5]==0x82:mem[0xC5B8]=1;lib.trade_hit(0)
    h.random.clear();h.call(0x91BE);lib.trade_pass(h.random[0] if h.random else 0);compare(lib,h,name,p)
    if mem[0xC40C]:break
   else:raise AssertionError(('no merchant',s,r,name))
   extra+=play_shop(lib,h,(s,r,name),answer,books,hearts,held)
   # Room reload executes5B1B, and the actual candle-break factory for hidden.
   h.clear_room_objects();h.call(0x5B1B);h.call(0x8671);lib.trade_reenter()
   if address is not None:
    mem[address+3]=1;h.call(0x8671);lib.trade_block((address-0xC470)//16,x,y,code)
   h.call(0x91BE);metadata(lib,h,[code,0]);compare(lib,h,'reload',0)
   # Cross05 has a distinct FF path, which must retain the count.
   h.call(0x928D);lib.trade_cross()
   for p in range(160):
    h.random.clear();h.call(0x91BE);lib.trade_pass(h.random[0] if h.random else 0);compare(lib,h,'cross',p)
   assert not mem[0xC5B5]
   # Its stream flag is zero: a second room reload must not recreate the wall.
   h.clear_room_objects();h.call(0x5B1B);h.call(0x8671);h.call(0x91BE);lib.trade_reenter();compare(lib,h,'gone reload',0)
   if address is not None:assert not lib.trade_present((address-0xC470)//16)
   # Stage decode (new life) restores both direct and hidden definitions/counts.
   h.clear_room_objects();h.call(0x5A49);h.call(0x5B1B);h.call(0x8671);lib.trade_reset();lib.trade_reenter()
   if address is not None:
    mem[address+3]=1;h.call(0x8671);assert lib.trade_block((address-0xC470)//16,x,y,code)==0
   h.call(0x91BE);metadata(lib,h,[code,0]);compare(lib,h,'new life',0)
   extra+=320
  print('Trade variants',s,r,'7 cases PASS',flush=True)
 print('Trade variants',extra,'images,98 scenarios PASS')
 legacy_window_reference(rom)
 box_checks=boxes(lib,rom,cases)
 pixels,max_tiles=graphics(lib,rom,cases)
 print('Trade total',sum(r['images'] for r in reports),'images,',len(reports),'late walls PASS')
 (ROOT/'research/trade-trace.json').write_text(json.dumps({'cases':reports,'variant_images':extra,'variant_scenarios':98,'box_checks':box_checks,'pixel_checks':pixels,'window_max_tiles':max_tiles},indent=2)+'\n')
if __name__=='__main__':main()
