#!/usr/bin/env python3
"""Analyse des mesures de tools/measure_whip_slots.py (un dossier par run, fichier slots.csv).

Usage : analyze_whip_slots.py DIR [DIR ...]
Pour chaque pression de bouton (premier tick de chaque marque stand-/crouch-) :
  - issue : DEBUT si C422 passe de 0 a non nul dans les 10 ticks suivant la pression (attaque lancee),
    BLOQUE sinon ;
  - ecart depuis la pression precedente (ticks), C096 (drapeau son, observe seulement) au moment de la
    pression, et etat des emplacements C450 et C460 (octets 0 et 1) au moment de la pression.
Puis, par run : accord entre l'issue et la presence de C451 (octet 1 de C450) non nul au moment de la
pression, et les intervalles pendant lesquels C451 est non nul (ticks).
Lecture seule.
"""
import csv, sys
from pathlib import Path


def num(v):
    return int(v)


def analyse(d):
    rows = list(csv.DictReader(open(Path(d) / 'slots.csv')))
    print('==', d, '(%d ticks)' % len(rows))
    presses = []
    prev_mark = 'idle'
    for i, r in enumerate(rows):
        m = r['mark']
        if m != prev_mark and (m.startswith('stand') or m.startswith('crouch')):
            presses.append(i)
        prev_mark = m
    agree = disagree = 0
    last = None
    for i in presses:
        r = rows[i]
        started = any(rows[j]['C422'] != '0' and rows[j - 1]['C422'] == '0'
                      for j in range(max(1, i), min(i + 10, len(rows))))
        busy = num(r['C451']) != 0
        if started == busy:
            disagree += 1
        else:
            agree += 1
        gap = '' if last is None else str(i - last)
        last = i
        print('%-18s tick %6s %-6s ecart %4s C096=%3s C450[0,1]=%s,%s C460[0,1]=%s,%s C436=%s C451_au_appui=%s' % (
            r['mark'], r['tick'], 'DEBUT' if started else 'BLOQUE', gap, r['C096'],
            r['C450'], r['C451'], r['C460'], r['C461'], r['C436'], 'actif' if busy else 'libre'))
    print('  accord (BLOQUE <=> C451 actif au moment de l\'appui) : %d oui, %d non' % (agree, disagree))
    spans, start = [], None
    for i, r in enumerate(rows):
        active = num(r['C451']) != 0
        if active and start is None:
            start = i
        if not active and start is not None:
            spans.append((rows[start]['tick'], rows[i - 1]['tick'], i - start))
            start = None
    if start is not None:
        spans.append((rows[start]['tick'], rows[-1]['tick'], len(rows) - start))
    print('  intervalles C451 actif (tick debut, tick fin, duree):', spans)


if __name__ == '__main__':
    for d in sys.argv[1:]:
        analyse(d)
