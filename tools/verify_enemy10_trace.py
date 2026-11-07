#!/usr/bin/env python3
"""Execute real-room Z80 spawn/updates vs production C, 300 video frames/case."""
import ctypes as C
import json
from pathlib import Path
import sys
from castle_room_objects import make_machine, call
from extract_rooms import decode_room
from probe import load_rom, reference_rom, ROOT


def main():
    lib = C.CDLL(str(Path(sys.argv[1]).resolve()))
    lib.ten_init.argtypes = [C.POINTER(C.c_ubyte)] + [C.c_int]*5
    lib.ten_step.argtypes = [C.c_int]*3
    lib.ten_record.argtypes = [C.c_int, C.POINTER(C.c_ubyte), C.POINTER(C.c_int)]
    rom = load_rom(reference_rom())
    cases = []
    scenarios = []
    for stage in range(14,19):
        m = make_machine(rom); m.memory[0xD000] = stage; m.memory[0xD002] = rom[0x1E6A+stage]; call(m,0x6154)
        base = 0xDB00+(stage-1-3*m.memory[0xD002])*256
        count = json.loads((ROOT/'research'/f'stage{stage}-foundations.json').read_text())['room_count']
        for room in range(count):
            for slot in range(4):
                entry = bytes(m.memory[base+room*16+slot*4:base+room*16+slot*4+4])
                if entry[0]&127 != 16: continue
                x = (entry[1]&15)*16
                for name, sx, difficulty in [('left',max(8,x-110),0),('right',min(240,x+110),0),('fire_left',max(8,x-22),0),('fire_right',min(240,x+22),0),('difficulty',max(8,x-22),2)]:
                    scenarios.append((f'{stage}:{room}:{slot}:{name}',stage,room,slot,sx,difficulty))
    for label, stage, room, slot, sx, difficulty in scenarios:
        m = make_machine(rom); mem = m.memory
        mem[0xD000] = stage; mem[0xD001] = room; mem[0xD002] = rom[0x1E6A+stage]
        call(m, 0x6154)
        base = 0xDB00 + (stage-1-3*mem[0xD002])*256
        entry = bytes(mem[base+room*16+slot*4:base+room*16+slot*4+4]); assert entry[0]&127 == 0x10
        x, y = (entry[1] & 15)*16, entry[1] & 240
        tilemap = bytes(decode_room(rom, stage, room)[2])
        mem[0xD012] = difficulty; mem[0xC427] = sx; mem[0xC425] = 112
        m.set_memory_block(0xD100, tilemap)
        for a in range(0xD600, 0xD700, 4): mem[a] = 0xE0
        m.c = 0x10; m.d = x; m.e = y; call(m, 0x5F1D)
        assert mem[0xC800] == 0x10
        cmap = (C.c_ubyte*768).from_buffer_copy(tilemap)
        lib.ten_init(cmap, stage, room, x, y, difficulty)
        observed = []; births = 0; previous = set()
        def compare(frame, sprites=False):
            nonlocal births, previous
            objects = []
            live = set()
            for slot in [-1]+list(range(8)):
                b = 0xC800 if slot < 0 else 0xD700+128*slot
                raw = bytes(mem[b:b+32]); out = (C.c_ubyte*32)(); sat = (C.c_int*24)()
                n = lib.ten_record(slot, out, sat)
                if slot >= 0 and not raw[0]:
                    assert not n, (label, frame, slot, 'unexpected C projectile')
                    continue
                assert bytes(out) == raw, (label, frame, slot, [(hex(i), a, c) for i, (a,c) in enumerate(zip(raw,out)) if a != c])
                zsat = [[mem[b+0x22+5*k],mem[b+0x23+5*k],mem[b+0x24+5*k]] for k in range(mem[b+0x20])] if raw[0] and sprites else []
                if zsat:
                    assert n == len(zsat), (label, frame, slot, n, zsat)
                    assert [list(sat[k*3:k*3+3]) for k in range(n)] == zsat, (label, frame, slot, list(sat)[:3*n], zsat)
                if slot >= 0: live.add(slot)
                objects.append({'slot':slot,'raw':raw.hex(),'sprites':zsat})
            births += len(live-previous); previous = live
            assert (mem[0xC427],mem[0xC425]) == (sx,112)
            observed.append({'frame':frame,'simon':[sx,112],'objects':objects})
        compare(-1)
        for frame in range(300):
            mem[0xC003] = (frame // 2) & 255
            if frame % 2 == 0:
                if mem[0xC800]:
                    m.ix = 0xC800; call(m, 0xADF8); call(m, 0x99B9); call(m, 0x99E5); call(m,0x6445)
                call(m, 0x9E31)
                lib.ten_step(sx, 112, frame)
            compare(frame, frame % 2 == 0)
        cases.append({'name':label,'stage':stage,'room':room,'entry':entry.hex(),'difficulty':difficulty,'frames':300,'projectile_births':births,'trace':observed})
        print(label, '300 frames PASS;', births, 'projectiles')
    # Differential boxes on the actual source object (83EA contact, 83F2 whip).
    m = make_machine(rom); mem = m.memory
    mem[0xD000] = 14
    for a in range(0xD600,0xD700,4): mem[a] = 0xE0
    m.c = 16; m.d = 128; m.e = 112; call(m,0x5F1D)
    cmap = (C.c_ubyte*768)()
    lib.ten_init(cmap,14,0,128,112,0)
    box_checks = {'contact':0,'whip':0}
    for posture in (0,1,2,3,4):
        mem[0xC420] = posture
        for sx in range(96,161,2):
            mem[0xC427] = sx
            for sy in range(32,177,2):
                mem[0xC425] = sy; m.ix = 0xC800; call(m,0x8290)
                assert lib.ten_contact(sx,sy,posture) == (m.f&1), ('contact',sx,sy,posture)
                box_checks['contact'] += 1
    for left in (0,1):
        mem[0xC42C] = left
        for chain in (0,1):
            mem[0xC416] = chain
            for crouch in (0,1):
                mem[0xC420] = 2 if crouch else 0
                for sx in range(72,185,2):
                    mem[0xC427] = sx
                    for sy in range(32,145,2):
                        mem[0xC425] = sy; m.ix = 0xC800; call(m,0x82B8)
                        # 82B8 returns the coordinates in B,C; 83F2 is class 4's whip routine.
                        call(m,0x83F2)
                        assert lib.ten_whip(sx,sy,left,crouch,chain) == (m.f&1), ('whip',sx,sy,left,crouch,chain)
                        box_checks['whip'] += 1
    print('Z80/C boxes PASS', box_checks)
    report={'all_equal':True,'reference':'Actual Z80 6154/5F1D, ADF8/99B9/99E5/6445, 9E31', 'box_checks':box_checks, 'cases':cases}
    (ROOT/'research/enemy10-trace.json').write_text(json.dumps(report,indent=1)+'\n')

if __name__ == '__main__': main()
