#!/usr/bin/env python3
"""Real late room tables/AI/whip/candles: controlled stress, not natural traversal."""
import json
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup
from castle_room_objects import decode_room

def main():
 rom=load_rom(reference_rom());rows=[];global_peak=0;witness=None
 for stage in range(12,19):
  count=json.loads((ROOT/f'research/stage{stage}-foundations.json').read_text())['room_count']
  for room in range(count):
   for delay in (32,96):
    h=setup(rom,stage,room);m=h.cpu.memory;h.call(0x6154);h.call(0x63C5);h.call(0x9CA9)
    base=0xDB00+(stage-1-3*m[0xD002])*256
    for i in range(4):
     b=m[base+room*16+4*i:base+room*16+4*i+4]
     if b[0]:h.cpu.c=b[0]&127;h.cpu.d=(b[1]&15)*16;h.cpu.e=b[1]&240;h.call(0x5F1D)
    candles=decode_room(rom,stage,room)['candles'];peak=(0,0,0);hits=0;events=[]
    for p in range(400):
     m[0xC003]=p&255;m[0xC427]=88;m[0xC425]=160;m[0xC440]=0;m[0xD010]=0;m[0xC43A]=255
     for a in (0x98E5,0x9E31):h.call(a)
     if p>=delay and p%8==0:
      targets=[0xC800+128*i for i in range(7) if m[0xC800+128*i] in (11,16)]
      if p<delay+16 and targets:
       a=targets[0];before=bytes(m[a:a+32]);m[0xC427]=(m[a+5]-20)&255;m[0xC425]=(m[a+3]+18)&255;m[0xC420]=0;m[0xC42C]=0;m[0xC416]=1;m[0xC422]=3;h.call(0x999F);h.call(0x7DAD)
       events.append({'pass':p,'whip':hex(a),'before':before.hex(),'after':bytes(m[a:a+32]).hex()})
      elif hits<len(candles):
       ob=candles[hits];b=bytes.fromhex(ob['raw']);hits+=1
       if b[4]<3:
        m[0xC427]=(b[2]-12)&255;m[0xC425]=(b[1]+18)&255;m[0xC420]=0;m[0xC42C]=0;m[0xC416]=1;m[0xC422]=3;h.call(0x999F);h.call(0x7D68);h.call(0x8671);events.append({'pass':p,'candle':ob['slot']})
     h.call(0x8A4A)
     e=sum(bool(m[0xC800+128*k]) for k in range(7));q=sum(bool(m[0xD700+128*k]) for k in range(8))
     if e+q>sum(peak[:2]):peak=(e,q,p)
     if e+q>global_peak:
      global_peak=e+q;witness={'stage':stage,'room':room,'delay':delay,'pass':p,'C800':e,'D700':q,'C800_records':[bytes(m[0xC800+128*k:0xC820+128*k]).hex() for k in range(7)],'D700_records':[bytes(m[0xD700+128*k:0xD720+128*k]).hex() for k in range(8)],'events':list(events)}
    rows.append((stage,room,delay,*peak));print('Mixed room',rows[-1],'800 images',flush=True)
 assert len(rows)==148 and global_peak>7,('separation witness',global_peak)
 (ROOT/'research/mixed-room-pools-trace.json').write_text(json.dumps({'images':148*800,'samples':rows,'peak':global_peak,'witness':witness,'scope':'Controlled Simon positions,guard and real whip strokes; not natural traversal. No ROM instructions modified.'},indent=2)+'\n')
 print('Mixed rooms:148 scenarios,118400 images,peak',global_peak,'exceeds shared7 PASS')
if __name__=='__main__':main()
