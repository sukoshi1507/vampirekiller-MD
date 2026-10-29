#!/usr/bin/env python3
"""Compare chaque test 84A7 du type 4 (t4.csv de run_type4_whip_trace.py) a la regle de la ROM :
X fouet = C427 +- 20 selon C42C (+- 24 si C416 = 1, fouet a chaine), touche si
|X fouet - D| < 20 (24 si C416 = 1) et (C425 - B - E + 10h) & FF < 14h, B = 18 debout, 10 accroupi.
Verifie aussi le contact 847A : chaque baisse d'energie (4638) par un type 4 suit un contact
predit par |C427 - D| < 13 et (C425' - 2 - E + 10h) & FF < 10h + (12h saut ou accroupi, sinon 1Ah),
C425' = C425 - 8 en saut (C420 = 1)."""
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1])))
# 84BA (wx, colonne a, traces recentes) : X du fouet (A) et demi-largeur (L) mesures dans la ROM.
wx = [r for r in rows if r['ev'] == 'wx']
rows = [r for r in rows if r['ev'] != 'wx']
hits = {(r['frame'], r['ixh'], r['ixl']) for r in rows if r['ev'] == 'hit'}
# 7DD4 suit directement le 84A7 qui touche ; le crochet d'image (4028) peut tomber entre les deux.
struck = {i for i, r in enumerate(rows[:-1]) if r['ev'] == 'box' and rows[i + 1]['ev'] == 'hit'
          and (rows[i + 1]['ixh'], rows[i + 1]['ixl']) == (r['ixh'], r['ixl'])}
ok = bad = 0
near = []
combos = {}
for i, r in enumerate(rows):
    if r['ev'] != 'box' or r['type'] != '4':
        continue
    D, E, L, H = int(r['d']), int(r['e']), int(r['l']), int(r['h'])
    sx, sy, f, st = int(r['c427']), int(r['c425']), int(r['c42c']), int(r['c420'])
    chain = int(r['c416']) == 1
    assert (L, H) == (8, 16), (L, H)
    B = 10 if st == 2 else 18
    reach = 24 if chain else 20
    w = sx - reach if f else sx + reach
    hit = 0 <= w <= 255 and abs(w - D) < reach and ((sy - B - E + 16) & 255) < 20
    real = i in struck
    key = ('gauche' if f else 'droite', 'accroupi' if st == 2 else 'debout', 'chaine' if chain else 'cuir')
    t, k = combos.get(key, (0, 0))
    combos[key] = (t + 1, k + real)
    if hit == real:
        ok += 1
    else:
        bad += 1
        print('MISMATCH', dict(r))
    w2 = sx + reach if f else sx - reach
    if 0 <= w2 <= 255 and abs(w2 - D) < reach and ((sy - B - E + 16) & 255) < 20 and not real:
        near.append((r['frame'], sx, f, D, E))
wx_bad = 0
for r in wx:
    if r['type'] != '4':
        continue
    chain = int(r['c416']) == 1
    reach = 24 if chain else 20
    if int(r['a']) != (int(r['c427']) - reach if int(r['c42c']) else int(r['c427']) + reach) % 256 \
            or int(r['l']) != reach:
        wx_bad += 1
        print('WX MISMATCH', dict(r))
print('84BA (X du fouet, L) type 4 :', sum(r['type'] == '4' for r in wx), 'mesures,', wx_bad, 'ecarts,',
      'chaine :', sum(r['type'] == '4' and r['c416'] == '1' for r in wx))
print('box tests', ok + bad, 'ok', ok, 'bad', bad, 'hits', len(hits))
for key in sorted(combos):
    print('  %s : %d tests, %d coups portes' % (' '.join(key), *combos[key]))
print('whip behind Simon would have touched (no kill):', len(near), near[:6])
cont = [r for r in rows if r['ev'] == 'contact' and r['type'] == '4']
print('contact params', {(r['l'], r['h']) for r in cont})


def contact_rule(r):
    D, E, L, H = int(r['d']), int(r['e']), int(r['l']), int(r['h'])
    sx, sy, st = int(r['c427']), int(r['c425']), int(r['c420'])
    if abs(sx - D) >= L + 5:
        return False
    tall = H + (0x12 if st in (1, 2) else 0x1A)
    y = sy - 8 if st == 1 else sy
    return ((y - 2 - E + H) & 255) < tall


pred = {(r['frame'], r['ixh'], r['ixl']) for r in cont if contact_rule(r)}
energy = [r for r in rows if r['ev'] == 'energy' and r['type'] == '4']
missing = [r['frame'] for r in energy if (r['frame'], r['ixh'], r['ixl']) not in pred]
print('energy by type 4', len(energy), 'predicted by 847A rule', len(energy) - len(missing),
      'contacts predicted', len(pred), 'missing', missing[:6])
