#!/usr/bin/env python3
"""ROM executions behind markers that were PROVISIONAL in sgdk/src.

Each check runs the original Z80 routine (z80 package, VDP adapter of intro_harness.py) and
compares what the port does (tests/rom_markers_bridge.c):
  hud        45B9 / 45B3: PLAYER and ENEMY bars, every energy value, pixels read in VRAM
  game over  42E3: the screen of state 7
  doors      7718 -> 772A: the 18 inner doors and the castle door (requests to 509F, C5AC, C701)
  candles    8874, 5B1B + 8671, then 5A49 (state 4, 62E6): the stage stream flags come back
  candle frames  8671: cadence of the wall candle (candle_view.c CANDLE_FRAME_TICKS)
  room load  62F5 leaves C415 and C418
  freeze     the port table against research/room-freeze-rooms.json (openMSX, run_room_freeze_trace.py)
Usage: verify_rom_markers.py BRIDGE.so
"""
import ctypes as C,json,sys
from pathlib import Path
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup
from extract_stage_exits import extract

def hud(lib,rom):
    h=setup(rom,2,0);m=h.cpu.memory;h.call(0x4513)
    for e in range(33):
        m[0xC415]=e;h.call(0x45B9);n=lib.markers_bar_pixels(e)
        assert n==2*e
        for y in range(14,18):
            assert [h.pixel(x,y) for x in range(60,124)]==[1]*n+[0]*(64-n),('PLAYER bar',e,y)
            assert h.pixel(59,y)==h.pixel(124,y)==0x0E
        for y in (13,18):assert all(h.pixel(x,y)==0x0E for x in range(59,125)),('PLAYER frame',e,y)
    for e in range(129):
        m[0xC418]=e;h.call(0x45B3);n=(e&0xFC)//2 or (2 if e else 0)   # hud.c vk_hud_enemy
        for y in range(23,27):
            assert [h.pixel(x,y) for x in range(60,124)]==[8]*n+[0]*(64-n),('ENEMY bar',e,y)
    print('HUD bars: 33 PLAYER and 129 ENEMY values, fill 1 / 8, empty colour 0, frame 0E PASS',flush=True)

def game_over(rom):
    h=setup(rom,2,0);m=h.cpu.memory;m[0xE600]=0;h.call(0x42E3)
    lit=[(x,y) for y in range(32,212) for x in range(256) if h.pixel(x,y)]
    assert {h.pixel(x,y) for x,y in lit}=={0x0E}
    box=(min(x for x,_ in lit),max(x for x,_ in lit),min(y for _,y in lit),max(y for _,y in lit))
    assert box==(89,167,89,95),box
    assert m[0xC004]==0x78
    print('Game over screen 42E3: only "GAME  OVER" in colour 0E, x 89..167, y 89..95, C004 = 78h PASS',flush=True)

def doors(lib,rom):
    rows=extract(rom);assert [r['stage'] for r in rows]==list(range(1,19))
    for r in rows:
        stage,d=r['stage'],r['door'];left=int(d['x']<128)
        for key in (0,1):
            h=setup(rom,stage,d['room']);m=h.cpu.memory
            m[0xC5AC]=1;m[0xC5AD]=d['y'];m[0xC5AE]=d['x'];m[0xC42C]=left;m[0xC420]=0
            m[0xC427]=d['x']+8 if left else d['x']-8;m[0xC425]=d['y']+16
            m[0xC701]=(m[0xC701]&0xFE)|key;h.sounds.clear();h.call(0x7718)
            if not key:assert not h.sounds and m[0xC5AC]==1,('door without key',stage,h.sounds)
            else:
                fade=lib.markers_door_fades(stage)
                assert fade==int(rom[0x3777+stage]!=0)
                assert h.sounds==([0xFF] if fade else [])+[0x1A],('door',stage,h.sounds)
                assert m[0xC5AC]==2 and not m[0xC701]&1
    h=setup(rom,1,0);m=h.cpu.memory;m[0xD000]=0
    m[0xC5AC]=4;m[0xC5AD]=144;m[0xC5AE]=224;m[0xC42C]=0;m[0xC420]=0;m[0xC427]=216;m[0xC425]=176;m[0xC701]&=0xFE
    h.sounds.clear();h.call(0x7718)
    assert h.sounds==[0x15] and m[0xC5AC]==5 and not lib.markers_door_fades(0)
    print('Doors 7718: 18 inner doors (no key: nothing; key: FF by 7777 then 1A, C5AC 2), castle door 15 PASS',flush=True)

def records(m):
    return [bytes(m[a:a+16]) for a in range(0xC480,0xC5B0,16) if m[a]]

def candles(lib,rom):
    cases=0
    for stage,room in ((1,5),(2,2),(4,3),(7,1),(12,0),(18,0)):
        h=setup(rom,stage,room);m=h.cpu.memory;first=records(m)
        lit=[a for a in range(0xC480,0xC5B0,16) if m[a]==2]
        if not lit:continue
        a=lit[0];p=m[a+7]<<8|m[a+8];value=m[p];assert value and 0xE000<=p<0xE480
        h.cpu.ix=a;h.call(0x8874);assert m[p]==0                    # the whip put the candle out
        m[0xC480:0xC5B0]=bytes(0x130);h.call(0x5B1B);h.call(0x8671)
        assert len(records(m))==len(first)-1,('candle gone in its room',stage,room)
        m[0xC480:0xC5B0]=bytes(0x130);h.call(0x5A49);h.call(0x5B1B);h.call(0x8671)   # 62E6 -> 5A49
        assert m[p]==value and records(m)==first,('stage stream decoded again',stage,room)
        assert lib.markers_candles_reset(stage,room)==0
        cases+=1
    assert cases>=4
    assert bytes(rom[0x23A:0x23D])==bytes((0xCD,0xE6,0x62)) and bytes(rom[0x22F2:0x22F5])==bytes((0xCD,0x49,0x5A))
    print('Candles:',cases,'rooms, flag cleared by 8874, candle gone, back after 5A49 (state 4: 423A -> 62E6 -> 62F2) PASS',flush=True)

def candle_frames(rom):
    """8671 once per pass: counter +6 up by one, frame = its bit 3, drawn when its bits 0-1 are 0."""
    for stage,room in ((1,5),(2,2),(4,3)):
        h=setup(rom,stage,room);m=h.cpu.memory
        a=[a for a in range(0xC470,0xC4F0,16) if m[a]==2 and m[a+4]==0][0];y,x=m[a+1],m[a+2]
        assert m[a+6]==(m[a+8]+2)&255          # first value: low byte of the stream address + 2
        seen={};shown=None;runs=[]
        for p in range(70):
            before=m[a+6];h.call(0x8671);assert m[a+6]==(before+1)&255
            image=bytes(h.pixel(x+i,y+j) for j in range(16) for i in range(16))
            if not m[a+6]&3:shown=m[a+6]>>3&1
            if shown is None:continue
            assert seen.setdefault(shown,image)==image,('candle frame',stage,room,p)
            if runs and runs[-1][0]==shown:runs[-1][1]+=1
            else:runs.append([shown,1])
        assert len(seen)==2 and seen[0]!=seen[1] and all(n==8 for _,n in runs[1:-1]),runs
    print('Candles 8671: frame = bit 3 of the counter +6, 8 passes (16 frames) each, 3 rooms x 70 passes PASS',flush=True)

def room_load(rom):
    for stage,room in ((1,3),(3,5),(6,2)):
        h=setup(rom,stage,room);m=h.cpu.memory;m[0xC415]=7;m[0xC418]=0x44
        for address in (0x6333,0x9147,0x8EE6,0x9CA9):h.call(address)     # steps of 62F5 not run by setup
        assert (m[0xC415],m[0xC418])==(7,0x44)
    writers=[i for i in range(len(rom)-2) if rom[i]==0x32 and rom[i+1]==0x18 and rom[i+2]==0xC4]
    assert writers==[0x04E5,0x0E14,0x22D7,0x2620],[hex(x) for x in writers]
    print('Room load: C418 kept; only writers 44E5, 4E14, 62D7, 6620 (LD (C418),A) PASS',flush=True)

def freeze(lib):
    rooms=json.loads((ROOT/'research/room-freeze-rooms.json').read_text())['rooms']
    for r in rooms:
        assert len(r['samples'])>=3 and lib.markers_freeze(r['stage'],r['room'])==r['frames']<<8|r['dark'],r
    assert max(abs(a-r['frames']) for r in rooms for a,_ in r['samples'])<=1
    print('Room change freeze:',len(rooms),'arrival rooms, port table = openMSX measure (samples within 1 frame) PASS',flush=True)

def main():
    lib=C.CDLL(str(Path(sys.argv[1]).resolve()));rom=load_rom(reference_rom())
    hud(lib,rom);game_over(rom);doors(lib,rom);candles(lib,rom);candle_frames(rom);room_load(rom);freeze(lib)
if __name__=='__main__':main()
