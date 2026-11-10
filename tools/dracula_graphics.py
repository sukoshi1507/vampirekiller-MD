"""Stage18 graphics through the executed room-entry chain, including6333."""
from verify_dracula_trace import Observed
from probe import load_rom,reference_rom

def room():
 h=Observed(load_rom(reference_rom()));m=h.cpu;mem=m.memory
 mem[0xF0F1:0xF0F4]=bytes([1,2,3]);mem[0xD000]=18;mem[0xD001]=9;mem[0xD002]=5
 for a in (0x548D,0x53B6,0x564C,0x570D,0x5768,0x5780,0x6333,0x4F83,0x4F91,0x632D):h.call(a)
 assert mem[0xCE00]==6 and any(h.vram[0xC000:0xC800])
 return h

def packed(h,x,y,w,height):
 return bytes((h.pixel(x+c,y+r)<<4)|h.pixel(x+c+1,y+r) for r in range(height) for c in range(0,w,2))

def tiles(h,x,y,w,height,sprite=False):
 words=[]
 for a,b in ([(c,r) for c in range(w//8) for r in range(height//8)] if sprite else [(c,r) for r in range(height//8) for c in range(w//8)]):
  for row in range(8):
   value=0
   for col in range(8):value=(value<<4)|h.pixel(x+a*8+col,y+b*8+row)
   words.append(value)
 return words
