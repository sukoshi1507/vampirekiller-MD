#!/usr/bin/env python3
"""Vectors for tests/test_ghost.c : the object 1C of the candles of object 15 (init B4D2, routine B4EA, then 99B9 and 99E5), run pass by
pass by the original Z80 routines in the kosarev/z80 emulator on the maps of the stages 7, 8 and 9, with Simon moved by a scripted
triangle wave. Usage: gen_ghost_vectors.py ROM OUT.h"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from castle_room_objects import make_machine, call
from probe import load_rom

PASSES = 400
# The object of the candles of object 15 : 1A (D002 = 1, stages 4 to 6) or 1C (D002 = 2, stages 7 to 9), same routine B4EA.
def object_type(stage):
    return 0x1A if stage <= 6 else 0x1C
# stage, room, X, Y (the candle Y + 16), Simon X (low, high, step per pass), Simon Y
SCENARIOS = [
    (9, 2, 48, 80, 20, 20, 0, 112), (9, 2, 48, 80, 20, 220, 3, 112), (9, 2, 96, 80, 100, 100, 0, 112), (9, 2, 96, 80, 20, 230, 5, 40),
    (7, 1, 160, 96, 20, 220, 2, 144), (7, 1, 160, 96, 20, 220, 4, 40), (7, 1, 160, 96, 200, 200, 0, 90),
    (7, 3, 192, 80, 20, 230, 3, 112), (7, 3, 192, 80, 20, 230, 3, 30), (7, 6, 64, 160, 20, 230, 2, 144), (7, 6, 64, 160, 20, 230, 2, 60),
    (8, 0, 80, 128, 20, 230, 3, 144), (8, 0, 80, 128, 20, 230, 3, 80), (8, 0, 80, 128, 20, 230, 3, 30), (8, 0, 100, 96, 30, 200, 2, 144),
    (8, 3, 112, 128, 20, 230, 3, 144), (8, 3, 112, 128, 20, 230, 3, 40), (8, 5, 144, 96, 20, 230, 3, 144), (8, 5, 144, 96, 20, 230, 3, 40),
    (8, 7, 32, 128, 20, 230, 3, 144), (8, 7, 80, 128, 20, 230, 3, 60), (8, 7, 192, 80, 20, 230, 3, 144), (8, 7, 192, 80, 20, 230, 3, 20),
    (9, 2, 144, 80, 20, 230, 4, 112), (9, 2, 192, 80, 20, 230, 4, 112), (9, 2, 192, 80, 20, 230, 4, 50), (9, 2, 235, 80, 20, 230, 4, 112),
    (8, 1, 100, 100, 20, 230, 3, 144), (8, 1, 100, 100, 20, 230, 3, 20), (8, 4, 160, 100, 20, 230, 3, 144), (8, 4, 60, 100, 20, 230, 3, 20),
    (8, 6, 120, 100, 20, 230, 3, 144), (8, 6, 120, 100, 20, 230, 3, 20), (8, 2, 120, 100, 20, 230, 3, 144), (8, 2, 120, 100, 20, 230, 3, 20),
    (7, 0, 120, 100, 20, 230, 3, 144), (7, 0, 120, 100, 20, 230, 3, 20), (7, 2, 120, 100, 20, 230, 3, 144), (7, 2, 120, 100, 20, 230, 3, 20),
    (7, 4, 120, 100, 20, 230, 3, 144), (7, 5, 120, 100, 20, 230, 3, 20), (7, 7, 120, 100, 20, 230, 3, 144), (7, 8, 120, 100, 20, 230, 3, 20),
    (9, 5, 120, 100, 20, 230, 3, 144), (9, 5, 120, 100, 20, 230, 3, 20), (9, 6, 120, 100, 20, 230, 3, 144), (9, 4, 120, 100, 20, 230, 3, 20),
]

# Stages 4 to 6 (object 1A): the 17 candles and blocks of object 15 (stage, room, X, candle Y + 16 (+ 16 more for a block)), Simon moved above
# and below, then every room of the three stages once; 300 passes.
CANDLES_456 = [(4, 0, 128, 80), (4, 1, 192, 64), (4, 2, 80, 64), (4, 4, 16, 64),
               (5, 0, 64, 48), (5, 4, 208, 80), (5, 5, 80, 64), (5, 5, 208, 64), (5, 5, 144, 160),
               (6, 0, 96, 192), (6, 0, 128, 192), (6, 0, 160, 192), (6, 1, 96, 48), (6, 1, 32, 144), (6, 1, 96, 144), (6, 4, 96, 112),
               (6, 4, 224, 112)]
for (st, ro, cx, cy) in CANDLES_456:
    SCENARIOS.append((st, ro, cx, cy, 20, 230, 3, 190, 300))
    SCENARIOS.append((st, ro, cx, cy, 20, 230, 2, 30, 300))
for st in (4, 5, 6):
    for ro in range(6):
        SCENARIOS.append((st, ro, 120, 100, 20, 230, 3, 144 if ro % 2 else 20, 200))


def build(data, stage, room):
    m = make_machine(bytes(data))
    mem = m.memory
    mem[0xD000] = stage; mem[0xD001] = room; mem[0xD002] = data[0x1E6A + stage]; mem[0xC41A] = 0
    call(m, 0x5A49); call(m, 0x5B1B); call(m, 0x4F83)
    for w in range(4):
        m.set_memory_block(0x4000 + w * 8192, bytes(data)[w * 8192:(w + 1) * 8192])
    mem[0xD012] = 0; mem[0xC420] = 0; mem[0xCE00] = 0; mem[0xCE03] = 0
    for a in range(0xD600, 0xD700, 4): mem[a] = 0xE0
    for a in range(0xD638, 0xD680, 4): mem[a] = 0xE0
    for k in range(7):
        for i in range(0x80): mem[0xC800 + 0x80 * k + i] = 0
    return m


def run(data, stage, room, x, y, lo, hi, step, sy, passes=PASSES):
    TYPE = object_type(stage)
    m = build(data, stage, room)
    mem = m.memory
    b = 0xC800
    mem[0xC427] = lo; mem[0xC425] = sy
    mem[b] = TYPE; mem[b + 3] = y; mem[b + 5] = x
    mem[b + 0x7E] = 1; mem[b + 0x7F] = 1; mem[b + 13] = data[0x20E1 + TYPE]
    m.ix = b
    call(m, data[0x1FCC + 2 * (TYPE - 1)] | data[0x1FCD + 2 * (TYPE - 1)] << 8)
    rows = []
    span = max(hi - lo, 0)
    for p in range(passes):
        t = (p * step) % (2 * span) if span else 0
        sx = lo + (2 * span - t if t > span else t)
        mem[0xC427] = sx
        if mem[b] == 0: break
        m.ix = b
        call(m, data[0x5942 + 2 * (TYPE - 1)] | data[0x5943 + 2 * (TYPE - 1)] << 8)
        call(m, 0x99B9); call(m, 0x99E5)
        rows.append((mem[b + 1], mem[b + 5], mem[b + 3], mem[b + 11], mem[b + 9] | mem[b + 10] << 8, mem[b + 7] | mem[b + 8] << 8,
                     mem[b + 16], 1 if mem[b] else 0))
        if mem[b] == 0: break
    return rows


def main():
    data = load_rom(Path(sys.argv[1]))
    for t in (0x1A, 0x1C):
        assert data[0x20E1 + t] == 1
    out = ["/* Generated by tools/gen_ghost_vectors.py from the reference ROM routines (Z80 emulator). Do not edit. */",
           "typedef struct { int stage, room, x, y, lo, hi, step, sy, passes, first; } GhostVector;",
           "static const int ghost_rows[] = {"]
    vec, nrows, jumps, falls, edges = [], 0, 0, 0, 0
    for sc in SCENARIOS:
        rows = run(data, *sc)
        vec.append((sc, len(rows), nrows))
        prev = None
        for r in rows:
            out.append(" %d,%d,%d,%d,%d,%d,%d,%d," % r)
            if prev and prev[0] == 3 and r[0] == 1 and r[5] == 0xFD00: jumps += 1
            if prev and prev[0] == 3 and r[0] == 1 and r[5] == 0x0200: falls += 1
            if prev and prev[0] == 3 and r[0] == 2 and (r[1] < 0x10 or r[1] >= 0xE8): edges += 1
            prev = r
            nrows += 1
    out.append("};")
    out.append("static const GhostVector ghost_vectors[] = {")
    for sc, n, first in vec:
        out.append(" {%d,%d,%d,%d,%d,%d,%d,%d,%d,%d}," % (*sc[:8], n, first))
    out.append("};")
    Path(sys.argv[2]).write_text("\n".join(out) + "\n")
    print(len(vec), 'scenarios', nrows, 'rows;', jumps, 'jumps (B58E),', falls, 'falls off a ledge (B5EF),', edges, 'edge stops')


if __name__ == '__main__':
    main()
