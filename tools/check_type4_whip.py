#!/usr/bin/env python3
"""Compare chaque test 84A7 du type 4 (t4.csv de run_type4_whip_trace.py) a la regle de la ROM :
X fouet = C427 +- 20 selon C42C, touche si |X fouet - D| < 20 et (C425 - B - E + 10h) & FF < 14h."""
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1])))
hits = {(r['frame'], r['ixh'], r['ixl']) for r in rows if r['ev'] == 'hit'}
ok = bad = 0
near = []
for r in rows:
    if r['ev'] != 'box' or r['type'] != '4':
        continue
    D, E, L, H = int(r['d']), int(r['e']), int(r['l']), int(r['h'])
    sx, sy, f, st = int(r['c427']), int(r['c425']), int(r['c42c']), int(r['c420'])
    assert (L, H) == (8, 16), (L, H)
    B = 10 if st == 2 else 18
    w = sx - 20 if f else sx + 20
    hit = 0 <= w <= 255 and abs(w - D) < 20 and ((sy - B - E + 16) & 255) < 20
    real = (r['frame'], r['ixh'], r['ixl']) in hits
    if hit == real:
        ok += 1
    else:
        bad += 1
        print('MISMATCH', dict(r))
    w2 = sx + 20 if f else sx - 20
    if 0 <= w2 <= 255 and abs(w2 - D) < 20 and ((sy - B - E + 16) & 255) < 20 and not real:
        near.append((r['frame'], sx, f, D, E))
print('box tests', ok + bad, 'ok', ok, 'bad', bad, 'hits', len(hits))
print('whip behind Simon would have touched (no kill):', len(near), near[:6])
cont = [r for r in rows if r['ev'] == 'contact' and r['type'] == '4']
print('contact params', {(r['l'], r['h']) for r in cont})
