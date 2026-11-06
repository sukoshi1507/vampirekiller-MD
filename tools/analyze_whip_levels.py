#!/usr/bin/env python3
"""Analyse des mesures de tools/measure_whip_levels.py (un dossier par run).

Usage : analyze_whip_levels.py DIR [DIR ...]
Pour chaque coup (debut = C42F passe de 0 a non nul, C42E = 0) :
  - sequence C42E/C42F (paires, nombre de ticks VBLANK), duree = ticks de la premiere a la derniere composition non nulle ;
  - noms de sprites SAT absents de la fenetre de repos precedente (lanière) ;
  - boite des pixels opaques de ces sprites relative a Simon (x = sprite.x - C427, y = sprite.y+1 - C425),
    decodage MSX2 1 bpp : motif n a l'adresse VRAM 0xF800 + 8n, quadrants UL, LL, UR, LR ;
  - registres PSG modifies dans les 3 ticks autour du debut.
Sortie : texte sur stdout. Lecture seule.
"""
import csv, sys
from pathlib import Path

SAT, PAT, ENTRY = 0, 128, 2176  # sat-pat.bin : 128 octets de SAT puis 2048 octets de motifs par tick


def frame(blob, tick):
    o = (tick - 1) * ENTRY
    return blob[o:o + ENTRY]


def visible(fr):
    out = []
    for i in range(32):
        y, x, n, c = fr[SAT + 4 * i:SAT + 4 * i + 4]
        if y < 0xD8:
            out.append((y, x, n, c))
    return out


def pixels(fr, n, dx, dy, y, x):
    pts = set()
    base = PAT + 8 * n
    quads = [(0, 0), (0, 8), (8, 0), (8, 8)]  # UL, LL, UR, LR : (col, row)
    for bi, (cx, ry) in enumerate(quads):
        for r in range(8):
            v = fr[base + bi * 8 + r]
            for c in range(8):
                if (v >> (7 - c)) & 1:
                    pts.add((x + cx + c - dx, y + 1 + ry + r - dy))
    return pts


def analyse(d):
    rows = list(csv.DictReader(open(Path(d) / 'ticks.csv')))
    psg = list(csv.DictReader(open(Path(d) / 'psg.csv')))
    blob = open(Path(d) / 'sat-pat.bin', 'rb').read()
    print('==', d)
    starts = [i for i in range(1, len(rows))
              if rows[i]['c42f'] != '0' and rows[i - 1]['c42f'] == '0' and rows[i]['c42e'] == '0']
    last_start = -100
    for s in starts:
        if s - last_start < 20:
            continue
        last_start = s
        tk = int(rows[s]['tick'])
        mark = rows[s]['mark']
        # Fenetre de repos : 60 ticks avant, sans composition
        base = set()
        for k in range(max(1, tk - 60), tk - 1):
            base |= {n for _, _, n, _ in visible(frame(blob, k))}
        # Composition : du premier tick non nul au dernier non nul de la suite (fenetre 40 ticks)
        seq = []
        k = s
        while k < len(rows) and k < s + 40:
            e, f = rows[k]['c42e'], rows[k]['c42f']
            if f != '0' or e != '0':
                if not seq or seq[-1][0] != (e, f):
                    seq.append([(e, f), 1])
                else:
                    seq[-1][1] += 1
            elif seq:
                break
            k += 1
        dur = sum(n for _, n in seq)
        seqs = ' '.join('%s/%s x%d' % (a, b, n) for (a, b), n in seq)
        names, boxes = set(), []
        for k in range(tk - 1, tk + 20):
            fr = frame(blob, k + 1)
            vis = visible(fr)
            new = [v for v in vis if v[2] not in base]
            if not new:
                continue
            names |= {v[2] for v in new}
            pts = set()
            cx, cy = int(rows[k]['c427']), int(rows[k]['c425'])
            for y, x, n, c in new:
                pts |= pixels(fr, n, cx, cy, y, x)
            if pts:
                xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
                boxes.append((rows[k]['tick'], min(xs), max(xs), min(ys), max(ys)))
        ch = []
        for j in range(max(1, s - 2), min(len(psg), s + 3)):
            regs = [r for r in range(14) if psg[j][f'r{r}'] != psg[j - 1][f'r{r}']]
            if regs:
                ch.append((j - s, ','.join('%d:%s' % (r, psg[j][f'r{r}']) for r in regs)))
        print('coup', mark, 'tick', tk, '| C42E/C42F', seqs, '| duree', dur, 'ticks')
        print('   sprites lanière', sorted(hex(n) for n in names))
        if boxes:
            ux = (min(b[1] for b in boxes), max(b[2] for b in boxes))
            uy = (min(b[3] for b in boxes), max(b[4] for b in boxes))
            print('   boite union rel x[%d,%d] y[%d,%d]' % (ux[0], ux[1], uy[0], uy[1]))
            for b in boxes:
                print('   tick %s x[%d,%d] y[%d,%d]' % b)
        print('   PSG (delta tick:reg=val)', ch)


if __name__ == '__main__':
    for d in sys.argv[1:]:
        analyse(d)
