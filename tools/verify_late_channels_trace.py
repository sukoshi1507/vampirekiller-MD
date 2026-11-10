#!/usr/bin/env python3
"""5EB8 channel cadence/side/gates in every marked late room, executed Z80 vs C."""
import ctypes as C,json,sys
from pathlib import Path
from probe import load_rom,reference_rom,ROOT
from castle_room_objects import make_machine,call

class Observed:
 def __init__(self,rom,points=(0xB218,0xB2A0)):
  self.cpu=make_machine(rom);self.random=[];self.points=points
  for a in (0x0100,*points):self.cpu.set_breakpoint(a)
 def call(self,address):
  c=self.cpu;c.sp=0xF100;c.memory[0xF100:0xF102]=bytes([0,1]);c.pc=address;c.ticks_to_stop=5000000
  for _ in range(10000):
   c.run()
   if c.pc==0x0100:return
   if c.pc in self.points:self.random.append(c.a);c.step_over_breakpoint()
   elif not c.ticks_to_stop:raise RuntimeError((hex(address),hex(c.pc)))
  raise RuntimeError('channel budget')

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom())
 for addr in (0xB216,0xB29E):assert rom[addr-0x4000:addr-0x4000+2]==bytes.fromhex('ed5f')
 flags=json.loads((ROOT/'research/late-enemy-flags.json').read_text()) if (ROOT/'research/late-enemy-flags.json').exists() else None
 # Discover actual flags through bank14's ROM table via 5ED8 rather than maintaining a copied list.
 records=[]
 for stage in range(12,19):
  count=json.loads((ROOT/'research'/f'stage{stage}-foundations.json').read_text())['room_count']
  for room in range(count):
   off=14*8192+0x5A6+2*stage;ptr=int.from_bytes(rom[off:off+2],'little');flag=rom[14*8192+ptr-0x8000+room]
   if not flag&0xC0:continue
   for mode in ['normal','right_guard','pause','frozen','item06','item06_long']:
    observer=Observed(rom);m=observer.cpu;mem=m.memory;mem[0xD000]=stage;mem[0xD001]=room;mem[0xD002]=rom[0x1E6A+stage]
    observer.call(0x9CA9);lib.channels_init(stage,room);trace=[];births=0
    if mode.startswith('item06'):
     mem[0xC431]=4 if mode.endswith('long') else 0;observer.call(0x8D7C);lib.channels_item(mem[0xC431])
    for p in range(500):
     # Sources are cleared after each observed creation so the cadence test never blocks on capacity.
     for a in range(0xD600,0xD700,4):mem[a]=0xE0
     for k in range(7):mem[0xC800+128*k]=0
     sx=224 if mode=='right_guard' else 64
     pause=2 if mode=='pause' and 50<=p<150 else 0
     frozen=int(mode=='frozen' and 50<=p<150)
     mem[0xC427]=sx;mem[0xC425]=160;mem[0xD010]=frozen;observer.random.clear()
     if mode.startswith('item06'):observer.call(0x75C0);pause=-1
     else:mem[0xC440]=pause
     if not frozen:observer.call(0x5EB8)
     events=[]
     for k in range(7):
      a=0xC800+128*k
      if not mem[a]:continue
      m.ix=a;observer.call(0x993B);observer.call(0x99B9);observer.call(0x99E5)
      if mem[a]:events.extend([mem[a],mem[a+1],mem[a+5],mem[a+3],mem[a+11],mem[a+12],mem[a+17]])
     expected=[mem[0xCF10],mem[0xCF0C],mem[0xCF0D]]+events
     random_input=(C.c_ubyte*len(observer.random))(*observer.random);lib.channels_random(random_input,len(observer.random))
     result=(C.c_int*80)();n=lib.channels_step(sx,pause,frozen,result)
     got=list(result)[:n]
     assert lib.channels_random_used()==len(observer.random),(stage,room,mode,p,'R consumption')
     if mode.startswith('item06'):assert lib.channels_pause()==mem[0xC440],(stage,room,mode,p,'item timer')
     assert got==expected,(stage,room,flag,mode,p,got,expected)
     births+=len(events)//7
     trace.append({'frame':2*p,'counter_phase':expected[:3],'created_objects':events,'pause':pause,'frozen':frozen})
    if flag==0x80:assert births==0
    if flag==0x40 and mode=='normal':assert births>0
    records.append({'stage':stage,'room':room,'flag':flag,'mode':mode,'images':1000,'created':births,'trace':trace})
 print('late channels:',len(records),'scenarios',len(records)*1000,'images PASS; 80h does not dispatch a channel')
 (ROOT/'research/late-channels-trace.json').write_text(json.dumps({'all_equal':True,'random_seam':'unmodified B216/B29E; actual R values observed and replayed in C','records':records},indent=1)+'\n')
if __name__=='__main__':main()
