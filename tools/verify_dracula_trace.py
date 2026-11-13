#!/usr/bin/env python3
"""Executed stage18 Z80 room65B0 / objectsAB42 versus production dracula.c."""
import ctypes as C,sys,json
from pathlib import Path
from probe import load_rom,reference_rom,ROOT
from build_boss12_md import BossGraphics
from extract_rooms import decode_room

class Observed(BossGraphics):
 def __init__(self,rom):
  super().__init__(rom);self.random=[]
  for a in (0x6856,0x6862):self.cpu.set_breakpoint(a)
  self.hooks[0x0056]=self.fill_vram;self.cpu.set_breakpoint(0x0056)
 def fill_vram(self):
  for i in range(self.cpu.bc):self.vram[(self.cpu.hl+i)&0x1FFFF]=self.cpu.a
 def call(self,address,budget=5000000):
  c=self.cpu;c.sp=0xF100;c.memory[0xF100:0xF102]=bytes((0,1));c.pc=address;c.ticks_to_stop=budget
  for _ in range(100000):
   c.run()
   if c.pc==0x0100:return
   if c.pc in (0x6856,0x6862):self.random.append(c.a);c.step_over_breakpoint()
   elif c.pc in self.hooks:
    self.hooks[c.pc]();c.pc=c.memory[c.sp]|c.memory[(c.sp+1)&65535]<<8;c.sp=(c.sp+2)&65535
   elif not c.ticks_to_stop:raise RuntimeError(f'Z80 budget at {c.pc:04X} calling {address:04X}')
  raise RuntimeError('too many stops')

def build(rom,graphics=False):
 if graphics:
  from dracula_graphics import room
  h=room()
 else:h=Observed(rom)
 m=h.cpu;mem=m.memory
 mem[0xD000]=18;mem[0xD001]=9;mem[0xD002]=5;mem[0xCE00]=6;mem[0xC418]=128;mem[0xC415]=32;mem[0xC002]=64
 mem[0xF0F1:0xF0F4]=bytes([1,2,3]);m.set_memory_block(0xD100,bytes(decode_room(rom,18,9)[2]))
 for a in range(0xD600,0xD700,4):mem[a]=224
 return h

def compare(lib,h,name,p):
 mem=h.cpu.memory
 for base,fn,count in [(0xC800,lib.d18_object,7),(0xD700,lib.d18_shot,8)]:
  for k in range(count):
   a=base+128*k;out=(C.c_ubyte*32)();fn(k,out);raw=bytes(mem[a:a+32])
   assert bytes(out)==raw,(name,p,hex(a),[(hex(i),a,b) for i,(a,b) in enumerate(zip(raw,out)) if a!=b])
 expected=[]
 for a in [0xC800+128*k for k in range(7)]+[0xD700+128*k for k in range(8)]:
  if mem[a]:
   for k in range(0,mem[a+32],2):
    b=a+34+5*k
    if (mem[b+3]|mem[b+8])&15:expected.append(list(mem[b:b+3]))   # a pair is drawn when either layer has a colour (the ball F0: colour 0 then 8, openMSX SAT)
 sprites=(C.c_int*108)();n=lib.d18_sprites(sprites)
 assert [list(sprites[3*i:3*i+3]) for i in range(n)]==expected,(name,p,'sprites',list(sprites)[:3*n],expected)
 ce=(C.c_ubyte*128)();lib.d18_ce(ce)
 for k in (0,1,2,14,15,18,19,21,22,49,50,51,52,53,54,55,56,57,64)+tuple(range(96,128)):assert ce[k]==mem[0xCE00+k],(name,p,'ce',hex(k),ce[k],mem[0xCE00+k])



class BackgroundOracle:
 def __init__(self):
  from build_dracula_background import generate
  _,self.assets=generate();self.cache={};self.checked=0
 def frame(self,visual):
  upper,lower,bust,x,blank,_,count,*stack=visual;key=(upper,lower,bust,x,blank,tuple(stack[:count]))
  if key in self.cache:return self.cache[key]
  if blank:buf=bytearray(256*176//2)
  else:
   a=self.assets;buf=bytearray(256*176//2)
   for i,t in enumerate(a['maps'][upper*3+lower]):
    for r in range(8):
     offset=(i//32*8+r)*128+i%32*4
     buf[offset:offset+4]=a['words'][t*8+r].to_bytes(4,'big')
   for bust in stack[:count]:
    for c in range(4):
     for row in range(4):
      for r in range(8):
       value=a['bust'][(bust-1)*128+(c*4+row)*8+r]
       for col in range(8):
        pixel=value>>(28-4*col)&15
        if pixel:
         px=x-16+c*8+col;py=145+row*8+r-32;off=py*128+px//2
         if px&1:buf[off]=(buf[off]&240)|pixel
         else:buf[off]=(buf[off]&15)|(pixel<<4)
  self.cache[key]=bytes(buf);return self.cache[key]
 def compare(self,lib,h,p):
  visual=(C.c_int*11)();lib.d18_visual(visual)
  if h.cpu.memory[0xCE40]:return
  expected=self.frame(list(visual));actual=bytes(h.vram[32*128:208*128])
  assert expected==actual,('background pixels',p,list(visual),next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),None))
  self.checked+=2


class CreditOracle:
 def __init__(self):
  from build_dracula_credits import generate
  _,self.assets=generate();self.buf=bytearray(32768);self.checked=0
 def band(self,mapping,row):
  for col,tile in enumerate(mapping):
   for r in range(8):self.buf[(row*8+r)*128+4*col:(row*8+r)*128+4*col+4]=self.assets['words'][tile*8+r].to_bytes(4,'big')
 def compare(self,lib,h,p):
  visual=(C.c_int*11)();lib.d18_visual(visual);ce=(C.c_ubyte*128)();lib.d18_ce(ce)
  if not ce[64] and not lib.d18_exit():return
  if visual[5]&1:
   for row in range(32):self.band(self.assets['base'][row*32:row*32+32],row)
  if visual[5]&2:self.band(self.assets['bands'][ce[49]-1],self.assets['y'][ce[49]-1]//8)
  if visual[5]&4:
   y=(ce[51]-2)&255;self.buf[y*128:(y+1)*128]=bytes(128)
  if visual[4]:self.buf[:]=bytes(32768)
  assert bytes(self.buf)==bytes(h.vram[:32768]),('credit pixels',p,list(ce[48:65]),list(visual),next((i for i,(a,b) in enumerate(zip(h.vram,self.buf)) if a!=b),None))
  self.checked+=2

def lifecycle(lib,rom,weapon=0,late=False,difficulty=0):
 h=build(rom,True);m=h.cpu;mem=m.memory;lib.d18_init();requests=[];h.hooks[0x509F]=lambda:requests.append(m.a)
 palette=bytes(v for i in range(16) for v in (((i%8)<<4)|((3*i)%8),(5*i)%8))
 h.vram[0xF680:0xF6A0]=palette;lib.d18_palette((C.c_ubyte*32).from_buffer_copy(palette))
 seed=5 if late else 0;mem[0xCF32]=seed;lib.d18_clock(seed)
 mem[0xD012]=difficulty
 seen=set();hits=[0,0];trace=[];background=BackgroundOracle();credits=CreditOracle()
 for p in range(10000):
  sx=32;tick=(p+1)&255;mem[0xC003]=tick;mem[0xC427]=sx;mem[0xC425]=160
  if mem[0xCE01] in (1,6) and mem[0xC800] in (17,23) and mem[0xC80E]&1 and (not late or mem[0xCE01]!=6 or p>2500) and p%8==0 and not mem[0xCE15]:
   mem[0xC416]=weapon;m.ix=0xC800;requests.clear();h.call(0x7E0C);lib.d18_hit(weapon);hits[0 if mem[0xC800]==17 else 1]+=1
   assert lib.d18_energy()==mem[0xC418],('energy',p,lib.d18_energy(),mem[0xC418])
  frozen=int(late and mem[0xCE01]==6 and 1900<=p<2000);mem[0xD010]=frozen
  h.random.clear();requests.clear()
  finished=int(mem[0xCE40]==3)
  mem[0xC0A6]=0xF8 if finished else 0
  h.call(0x66BA if mem[0xCE40] else 0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
  assert len(h.random)<=16
  assert all((b-a)&127==8 for a,b in zip(h.random[::2],h.random[1::2]))
  random=(C.c_ubyte*16)(*(h.random+[0]*(16-len(h.random))));lib.d18_random(random)
  lib.d18_step(sx,160,difficulty,tick,frozen,finished);compare(lib,h,'lifecycle',p)
  assert lib.d18_random_used()==len(h.random)
  background.compare(lib,h,p);credits.compare(lib,h,p)
  seen.add(mem[0xCE01]);trace.append([mem[0xCE01],mem[0xCE40]])
  sounds=(C.c_ubyte*8)();n=lib.d18_sounds(sounds);assert list(sounds)[:n]==requests,('sounds',p,list(sounds)[:n],requests)
  score=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(mem[0xC405:0xC408]));assert lib.d18_score()==score,('score',p,lib.d18_score(),score)
  if mem[0xC409]:
   assert lib.d18_exit() and mem[0xD012]==min(2,difficulty+1);break
 assert mem[0xC409] and hits[0] and hits[1], (p, list(mem[0xCE30:0xCE41]),hits)
 print('Dracula credit pixels:',credits.checked,'images PASS')
 print('Dracula background pixels:',background.checked,'images PASS')
 print('Dracula lifecycle weapon/difficulty',weapon,difficulty,'states',sorted(seen),'hits',hits,'images',2*len(trace),'PASS')
 return trace


def transition(lib,rom):
 for cycles in (0,1):
  h=build(rom);m=h.cpu;mem=m.memory;mem[0xCF34]=cycles
  modes=[];h.hooks[0x62D5]=lambda:None;h.hooks[0x41B6]=lambda:modes.append(m.a)
  m.set_breakpoint(0x62D5);m.set_breakpoint(0x41B6);h.call(0x4326)
  out=(C.c_int*3)();lib.d18_transition(cycles,out)
  assert list(out)==[mem[0xCF34],int(modes==[0]),mem[0xD000]],('transition',cycles,list(out),modes,mem[0xD000])
 print('Dracula post-credits4326: first clear restarts courtyard, second clear returns title PASS')


def boxes(lib,rom):
 checks=0
 for t,projectile in [(17,False),(23,False),(44,False),(5,True)]:
  h=build(rom);m=h.cpu;mem=m.memory;lib.d18_init();lib.d18_place(t,int(projectile));a=0xD700 if projectile else 0xC800;slot=7 if projectile else 0
  mem[a]=t;mem[a+3]=160;mem[a+5]=128
  captured=[];target_routine=0x8258 if projectile else 0x82A4
  m.set_breakpoint(0x84F0);h.hooks[0x84F0]=lambda:captured.append([m.d,m.e,m.h,m.l]);m.ix=a;h.call(target_routine)
  out=(C.c_int*4)();lib.d18_target(slot,out);assert list(out)==captured[0]
  del h.hooks[0x84F0];m.clear_breakpoint(0x84F0)

  for state in range(5):
   mem[0xC420]=state
   for sx in range(96,161,2):
    mem[0xC427]=sx
    for sy in range(96,225,2):
     mem[0xC425]=sy;m.ix=a;h.call(0x824C if projectile else 0x8290)
     assert lib.d18_contact(slot,sx,sy,state)==(m.f&1),('box',t,sx,sy,state);checks+=1
  for left in (0,1):
   mem[0xC42C]=left
   for chain in (0,1):
    mem[0xC416]=chain
    for state in (0,2):
     mem[0xC420]=state
     for sx in range(72,185,2):
      mem[0xC427]=sx
      for sy in range(96,209,2):
       mem[0xC425]=sy;m.ix=a;h.call(0x8270 if projectile else 0x827C)
       assert lib.d18_whip(slot,sx,sy,left,state,chain)==(m.f&1),('whip',t,sx,sy,left,state,chain);checks+=1
 print('Dracula first/second form, fly and projectile boxes:',checks,'PASS')
 return checks


def strikes(lib,rom):
 frames=0;struck=0
 for mode in ('projectiles','flies'):
  h=build(rom);m=h.cpu;mem=m.memory;lib.d18_init();score=0;requests=[];h.hooks[0x509F]=lambda:requests.append(m.a)
  if mode=='flies':mem[0xCE01]=4;lib.d18_second()
  for p in range(400):
   frozen=int(55<=p<100);mem[0xD010]=frozen;mem[0xC003]=(p+1)&255;mem[0xC427]=32;mem[0xC425]=160
   if p>=40 and p%9==0:
    candidates=[(0xC800+128*k,k) for k in range(7) if mem[0xC800+128*k]==44] if mode=='flies' else [(0xD700+128*k,7+k) for k in range(8) if mem[0xD700+128*k]==5]
    if candidates:
     a,k=candidates[0];m.ix=a;mem[0xC416]=p&1;requests.clear()
     if mode=='flies':h.call(0x7DD4)
     else:
      m.a=12;h.call(0x509F);h.call(0x822A);h.call(0x9A1A)
     assert lib.d18_strike(k,p&1,2 if p&1 else 1)==1
     sounds=(C.c_ubyte*8)();n=lib.d18_sounds(sounds);assert list(sounds)[:n]==requests,('strike sounds',mode,p,list(sounds)[:n],requests)
     struck+=1
   h.random.clear();requests.clear()
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
   lib.d18_step(32,160,0,(p+1)&255,frozen,0);compare(lib,h,mode,p);frames+=2
   actual=sum(((v>>4)*10+(v&15))*100**i for i,v in enumerate(mem[0xC405:0xC408]));assert lib.d18_score()==actual,('strike score',mode,p,lib.d18_score(),actual)
 print('Dracula projectile/fly strikes:',struck,'hits,',frames,'images including freeze PASS')
 return frames

def main():
 lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom());reports=[]
 h=build(rom);mem=h.cpu.memory;mem[0xC41E]=9;mem[0xC41F]=7;mem[0xC427]=16;mem[0xC425]=176
 requests=[];h.hooks[0x509F]=lambda:requests.append(h.cpu.a);h.call(0x77D1)
 out=(C.c_int*5)();lib.d18_entry(out)
 assert list(out)==[18,9,8,mem[0xC427],requests[0]] and mem[0xC425]==176
 assert mem[0xC427]==246 and requests==[0x86]
 print('Dracula entry77D1: room9 X246, Y preserved, music86 PASS')

 for name,sx in [('left',32),('right',224),('near',128),('freeze',80)]:
  h=build(rom,True);m=h.cpu;mem=m.memory;lib.d18_init();trace=[];motion_background=BackgroundOracle()
  for p in range(1000):
   frozen=int(name=='freeze' and 80<=p<200);mem[0xC003]=(p+1)&255;mem[0xC427]=sx;mem[0xC425]=160;mem[0xD010]=frozen
   h.call(0xB6AB);h.call(0x98E5);h.call(0x9E31);h.call(0x9918)
   lib.d18_step(sx,160,0,(p+1)&255,frozen,0);compare(lib,h,name,p);motion_background.compare(lib,h,p)
   trace.append(bytes(mem[0xC800:0xC820]).hex())
  reports.append({'name':name,'images':2000,'body':trace})
 strike_frames=strikes(lib,rom)
 box_checks=boxes(lib,rom)
 transition(lib,rom)
 endings=[lifecycle(lib,rom),lifecycle(lib,rom,1,False,1),lifecycle(lib,rom,2,True,2),lifecycle(lib,rom,3,False,2)]
 print('Dracula first form:',len(reports)*2000,'images PASS')
 (ROOT/'research/dracula-trace.json').write_text(json.dumps({"motion":reports,"endings":endings,"box_checks":box_checks,"strike_frames":strike_frames})+'\n')
if __name__=='__main__':main()
