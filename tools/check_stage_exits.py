#!/usr/bin/env python3
"""Static generated-data checks; optional ROM-backed Z80 differential checks.
Usage: python tools/check_stage_exits.py [reference.rom]
Requires z80==1.2.0 and a C compiler for the ROM-backed comparison.
"""
import ctypes,json,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
rows=json.loads((ROOT/'research/stages-1-10/MSX2_STAGE_EXITS.json').read_text())
counts=[9,6,6,6,6,6,9,8,9,9]
h=(ROOT/'sgdk/src/generated/stage_exits.h').read_text()
parsed=[tuple(map(int,v.split(','))) for v in re.findall(r'\{ ([\d, ]+) \}, /\* stage \d+ \*/',h)]
assert len(parsed)==10
for row,values,count in zip(rows,parsed,counts):
    k,d=row['key'],row['door']
    assert values==(k['room'],k['x'],k['y'],k['hidden'],d['room'],d['x'],d['y'])
    assert k['room']<count and d['room']<count
main=(ROOT/'sgdk/src/main.c').read_text()
assert 'vk_game_enter_stage(&vk_game, next_stage)' in main and 'vk_game.stage >= 10' in main
assert 'TEST exits for stages' not in main
print('10 generated exit/key definitions and progression dispatch: PASS')
if len(sys.argv)>1:
    from probe import load_rom
    from extract_stage_exits import extract
    from castle_room_objects import make_machine,call
    data=load_rom(Path(sys.argv[1]));assert extract(data)==rows
    print('Keys, BB37 door loader and 6402 entry routine vs original ROM: PASS')
    out=ROOT/'sgdk/out/native-tests';out.mkdir(parents=True,exist_ok=True)
    so=out/'door_probe.so'
    subprocess.run(['cc','-shared','-fPIC','-I'+str(ROOT/'sgdk/src'),str(ROOT/'tests/door_probe.c'),str(ROOT/'sgdk/src/items.c'),'-o',str(so)],check=True)
    probe=ctypes.CDLL(str(so)).door_probe;probe.argtypes=[ctypes.c_int]*5;probe.restype=ctypes.c_int
    cases=0
    for row in rows:
        d=row['door'];m=make_machine(data)
        m.memory[0xc5ad]=d['y'];m.memory[0xc5ae]=d['x']
        for facing in (0,1):
            m.memory[0xc42c]=facing
            for y in sorted({0,255,*[(d['y']+dy)&255 for dy in (-8,-7,-6,-5,0,48,49,50,51)]}):
                for x in range(256):
                    m.b=y;m.c=x;call(m,0x8580)
                    original=int(bool(m.f&1))
                    actual=probe(row['stage'],d['room'],x,y,facing)
                    assert original==actual,(row['stage'],x,y,facing,original,actual)
                    cases+=1
    print(f'Original Z80 8580 vs compiled MD predicate: {cases} cases PASS')
else:
    print('ROM-backed checks not requested; run again with the supplied ROM path.')
