#!/usr/bin/env python3
"""Checks that every sprite cell seen in the VRAM snapshots of a stage 9 openMSX run (rooms 0 to 8, one snapshot per
new set of patterns, see research/stage9-enemies.md) exists, as a pixel mask, in the generated stage 9 block.
A cell is a pattern pair (N, N + 4) ORed by the CC bit, or a single pattern. Prints the cells without match.
Usage: check_stage9_cells.py RUN_DIR [SNAP_SUBSTRING]"""
import glob, re, sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
text = (root / 'sgdk/src/generated/stage9_enemies_md.c').read_text()
w = [int(x, 16) for x in re.findall(r'0x([0-9A-F]{8})', text)]
cells = {}
for c in range(32):
    pix = []
    for y in range(16):
        row = 0
        for x in range(16):
            col, r = divmod(x, 8), y // 8
            word = w[((c * 2 + col) * 2 + r) * 8 + (y % 8)] if False else None
        pix.append(row)
    cells[c] = None
# tile order of the builder: for c in 0..1 (column), r in 0..1 (row), y in 0..7
def cell_mask(c):
    mask = []
    for y in range(16):
        row = 0
        for x in range(16):
            col, r = x // 8, y // 8
            word = w[c * 32 + (col * 2 + r) * 8 + (y % 8)]
            nib = (word >> (4 * (7 - (x % 8)))) & 15
            row = (row << 1) | (1 if nib else 0)
        mask.append(row)
    return tuple(mask)
known = {cell_mask(c): 0x40 + 4 * c for c in range(32)}
run = Path(sys.argv[1])
flt = sys.argv[2] if len(sys.argv) > 2 else ''
seen, bad = {}, {}
for f in sorted(glob.glob(str(run / 'snaps' / f'*{flt}*.vram'))):
    vram = open(f, 'rb').read()
    regs = [int(x) for x in open(f[:-5] + '.txt').read().split('\n')[0].split()[1:]]
    gen = (regs[6] & 0x3F) << 11
    sat = ((regs[11] & 3) << 15) | ((regs[5] & 0xF8) << 7) | 0x200
    col = sat - 0x200
    sprites = []
    for n in range(32):
        a = sat + 4 * n
        if vram[a] == 216: break
        sprites.append((vram[a + 2], vram[col + 16 * n]))
    def pat(p):
        return tuple((vram[gen + p * 8 + y] << 8) | vram[gen + p * 8 + 16 + y] for y in range(16))
    i = 0
    while i < len(sprites):
        p, c = sprites[i]
        if p < 0x40: i += 1; continue
        m = pat(p)
        if i + 1 < len(sprites) and sprites[i + 1][1] & 0x40 and sprites[i + 1][0] == p + 4:   # N then N + 4
            m = tuple(a | b for a, b in zip(m, pat(p + 4))); i += 1
        elif c & 0x40 and i and sprites[i - 1][0] == p - 4: i += 1; continue
        elif i + 1 < len(sprites) and c & 0x40 and sprites[i + 1][0] == p + 4 and not sprites[i + 1][1] & 0x40:
            pass
        elif i + 1 < len(sprites) and sprites[i + 1][0] == p - 4 and sprites[i + 1][1] & 0x40:   # N + 4 then N (CC)
            m = tuple(a | b for a, b in zip(m, pat(p - 4))); p -= 4; i += 1
        if not any(m): i += 1; continue
        key = (p, ok2 := (p + 4 if m != pat(p) else 0))
        seen.setdefault(key, []).append((m, f))
        i += 1
# A pattern pair is explained when at least one snapshot (the game copies a pose into the sprite generator over
# some frames, so a snapshot can hold a half-copied pose) shows the cell of the block.
nbad = 0
for key, lst in sorted(seen.items()):
    hits = [x for x in lst if x[0] in known]
    if hits:
        print(f'patterns {key[0]:02X}/{key[1]:02X}: {len(hits)}/{len(lst)} snapshots = cell {known[hits[0][0]]:02X}')
    else:
        nbad += 1
        print(f'patterns {key[0]:02X}/{key[1]:02X}: NO MATCH in {len(lst)} snapshots')
print('pattern pairs', len(seen), 'unexplained', nbad)
if len(sys.argv) > 3:
    for c in range(32):
        print(hex(0x40 + 4 * c))
        for r in cell_mask(c): print('  ' + format(r, '016b').replace('0', '.').replace('1', '#'))
