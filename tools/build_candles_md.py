#!/usr/bin/env python3
"""Execute5A49/5B1B for the full stage0..18 candle/fixed-object set.
White keys retain their separate items.c path; --check verifies generated outputs.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import castle_room_objects
from castle_room_objects import decode_room
from probe import load_rom,reference_rom,ROOT
from decode_room_links import decode

ROOMS = {0:3, 1:9, 2:6, 3:6, 4:6, 5:6, 6:6, 7:9, 8:8, 9:9, 10:9, 11:6}
ROOMS.update({12:12,13:12,14:8,15:10,16:10,17:12,18:10})
WALL_STAGES = 19  # all stages 0 to 11 have their full object set

def decode_wall(data, stage, room):
    """DE00 slot items and C5B5/C5C5 records after 5A49 and 5B1B (5BB7 -> 9179)."""
    machine = castle_room_objects.make_machine(data)
    machine.memory[0xD000] = stage
    machine.memory[0xD001] = room
    machine.memory[0xD002] = data[castle_room_objects.STAGE_TABLE_5E6A + stage]
    machine.memory[0xC41A] = 0
    castle_room_objects.call(machine, 0x5A49)
    castle_room_objects.call(machine, 0x5B1B)
    base = 0xDE00 + (0 if stage == 0 else (0, 0x40, 0x80)[(stage - 1) % 3]) + 4 * room   # 944F
    items = (machine.memory[base], machine.memory[base + 2])
    walls = []
    for rec in (0xC5B5, 0xC5C5):
        r = bytes(machine.memory[rec:rec + 16])
        if r[0]:
            # 9189: +1 E (Y), +2 D (X), +4 B, +5 C; the stream entry is (B << 2) | C (5BBD).
            walls.append((r[2], r[1], 0xC0 | (r[4] << 2) | r[5]))
    return items, walls

def collect_walls(data):
    defs, first, items = [], [], []
    for stage, n in ROOMS.items():
        frow, irow = [], []
        for room in range(13):
            frow.append(len(defs))
            if room == 12: continue
            if stage >= WALL_STAGES or room >= n:
                irow.append((0, 0)); continue
            it, walls = decode_wall(data, stage, room)
            irow.append(it)
            defs.extend((x, y, code, stage, room) for x, y, code in walls)
        first.append(frow); items.append(irow)
    return defs, first, items

def load_frames(data, stage, room):
    """Record base -> frame drawn by the 8671 of the room load. B = 1 there: 873A runs and leaves HL on a
    room byte, then 86C2 (BIT 3,(HL)) picks the frame of a mode 0 candle: not the counter +6."""
    from verify_item_trace import setup
    h = setup(data, stage, room)
    m, c = h.cpu.memory, h.cpu
    slots = [a for a in range(0xC470, 0xC4F0, 16) if m[a] == 2]
    for a in slots:
        m[a] = 1                      # the state the record has before the load pass
        m[a + 6] = (m[a + 8] + 1) & 255
    frames = {}
    c.set_breakpoint(0x86C2)
    c.sp = 0xF100; m[0xF100:0xF102] = bytes([0, 1]); c.pc = 0x8671; c.ticks_to_stop = 5000000
    for _ in range(1000):
        c.run()
        if c.pc == 0x86C2:
            frames[c.ix] = m[c.hl] >> 3 & 1
            c.step_over_breakpoint()
        elif c.pc == 0x0100:
            break
        elif c.pc in h.hooks:         # sound, random: the harness stubs, then RET
            h.hooks[c.pc]()
            c.pc = m[c.sp] | m[c.sp + 1] << 8
            c.sp = (c.sp + 2) & 65535
        elif c.ticks_to_stop:         # another breakpoint of the harness
            c.step_over_breakpoint()
        else:
            raise RuntimeError(f'8671 budget exceeded at {c.pc:04X} ({stage}, {room})')
    return frames

def collect(data):
    candles, fixed, streams, loads = [], [], [], []
    cfirst, ffirst = [], []
    for stage, n in ROOMS.items():
        crow, frow = [], []
        for room in range(13):
            crow.append(len(candles)); frow.append(len(fixed))
            if room >= n: continue
            r = decode_room(data, stage, room)
            lf = load_frames(data, stage, room)
            for e in r['candles']:
                loads.append(lf.get(int(e['slot'], 16), 0))
                b = bytes.fromhex(e['raw'])
                candles.append((b[2], b[1], b[4], b[5], b[9], stage, room))
                assert b[6] == (b[8] + 1) & 255   # 5B1B: counter +6 = low byte of the stream address (+8) + 1
                streams.append(b[8])
            for e in r['other_objects']:
                b = bytes.fromhex(e['raw'])
                if b[4] == 0x18: continue  # A visible white key (stage 8 room 7) is the key of items.c, not a drop.
                if int(e['slot'], 16) >= 0xC500 and b[0] == 0x83:
                    fixed.append((b[2], b[1], 0, b[4], b[13], stage, room))
        cfirst.append(crow); ffirst.append(frow)
    return candles, fixed, cfirst, ffirst, streams, loads

def fmt_defs(name, items):
    out = [f"const VKCandleDef {name}[{len(items)}] = {{"]
    out += [f"    {{ {x}, {y}, {m}, 0x{k:02X}, 0x{c:02X} }},   /* stage {s} room {r} */" for x, y, m, k, c, s, r in items]
    return "\n".join(out) + "\n};\n"

def fmt_first(name, rows, ctype="VKByte"):
    out = [f"const {ctype} {name}[19][13] = {{"]
    out += ["    { " + ", ".join(map(str, row)) + " }," for row in rows]
    return "\n".join(out) + "\n};\n"

def main():
    import argparse
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument("rom",nargs="?",type=Path,default=reference_rom());ap.add_argument("out",nargs="?",type=Path,default=ROOT/"sgdk/src/generated");ap.add_argument("--check",action="store_true");a=ap.parse_args()
    data=load_rom(a.rom);outdir=a.out
    ROOMS.update({s["stage"]:len(s["rooms"]) for s in decode(data) if s["stage"]>=12})
    cand, fixed, cf, ff, streams, loads = collect(data)
    # rows must end with the total for the unused rooms
    c = ("/* Generated by tools/build_candles_md.py. Never hand edit. */\n#include \"candles_md.h\"\n\n"
         "/* x, y, mode (+4: 0 candle, 1 courtyard torch, 3 hidden block), object (+5),\n * content (+9 of a 0x1F block). */\n"
         + fmt_defs("vk_candle_defs", cand)
         + "/* Low byte of the stream address (+8) of each candle record after 5B1B; the counter +6 of\n * 8671 is this + 2 once the room load has run 8671 (tools/verify_rom_markers.py). */\n"
         + f"const VKByte vk_candle_stream[{len(cand)}] = {{\n" + "".join(f"    0x{v:02X},\n" for v in streams) + "};\n"
         + "/* Frame (bit 3 of the room byte at 86C2) the 8671 of the room load draws, for the candles of mode 0. */\n"
         + f"const VKByte vk_candle_load_frame[{len(cand)}] = {{" + ",".join(str(v) for v in loads) + "};\n"
         + "/* First candle of each room; room r has first[r + 1] - first[r]. */\n" + fmt_first("vk_candle_first", cf, "VKWord")
         + "\n/* x, y, 0, object, content (+13, chest only). */\n"
         + fmt_defs("vk_fixed_object_defs", fixed) + fmt_first("vk_fixed_object_first", ff) + "\n")
    walls, wfirst, witems = collect_walls(data)
    c += "/* Wall objects shown at the room load (5BB7 -> 9179): x, y, stream entry. */\n"
    c += f"const VKWallDef vk_wall_defs[{max(len(walls), 1)}] = {{\n"
    c += "".join(f"    {{ {x}, {y}, 0x{code:02X} }},   /* stage {st} room {r} */\n" for x, y, code, st, r in walls)
    c += "};\n" + fmt_first("vk_wall_first", wfirst)
    c += "/* DE00 (5AAF, 944F): item of wall slots 0 and 1, per room. */\nconst VKByte vk_wall_items[19][12][2] = {\n"
    for row in witems:
        c += "    { " + ", ".join(f"{{ 0x{a:02X}, 0x{b:02X} }}" for a, b in row) + " },\n"
    c += "};\n\n"
    c += "const VKByte vk_key_block_tiles[19][16] = {\n"
    for stage in range(19):
        off = 0x4796 if data[0x1e6a + stage] == 0 else 0x47aa
        c += "    { " + ", ".join(str(v) for v in data[off:off+16]) + " },\n"
    c += "};\n"
    h = ("/* Generated by tools/build_candles_md.py. Never hand edit. */\n#ifndef VK_CANDLES_MD_H\n#define VK_CANDLES_MD_H\n#include \"../player.h\"\n"
         "typedef struct { VKByte x, y, mode, kind, content; } VKCandleDef;\ntypedef struct { VKByte x, y, code; } VKWallDef;\n#define VK_OBJECT_SHEET_COUNT 0x22\n"
         f"#define VK_WALL_DEF_COUNT {len(walls)}\nextern const VKWallDef vk_wall_defs[{max(len(walls), 1)}];\nextern const VKByte vk_wall_first[19][13];\nextern const VKByte vk_wall_items[19][12][2];\n"
         f"#define VK_CANDLE_DEF_COUNT {len(cand)}\n#define VK_FIXED_OBJECT_COUNT {len(fixed)}\n"
         f"extern const VKByte vk_key_block_tiles[19][16];\nextern const VKCandleDef vk_candle_defs[{len(cand)}];\nextern const VKByte vk_candle_stream[{len(cand)}];\nextern const VKByte vk_candle_load_frame[{len(cand)}];\nextern const VKWord vk_candle_first[19][13];\n"
         f"extern const VKCandleDef vk_fixed_object_defs[{len(fixed)}];\nextern const VKByte vk_fixed_object_first[19][13];\n#endif\n")
    for name,text in [('candles_md.c',c),('candles_md.h',h)]:
        if a.check:assert (outdir/name).read_text()==text,name
        else:(outdir/name).write_text(text)
    print(f"candles={len(cand)} fixed={len(fixed)}")
if __name__ == '__main__': main()
