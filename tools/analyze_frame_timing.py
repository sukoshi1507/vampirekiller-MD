#!/usr/bin/env python3
"""Analyse d'une trace de tools/run_frame_timing_trace.py (research/frame-timing.md).

Usage : analyze_frame_timing.py OUTDIR

Conventions. VBLANK N = interruption N (crochet 4028, colonne frame). Image N = l'image
balayee apres le VBLANK N (entre les VBLANK N et N+1). Une ecriture VRAM faite a t
microsecondes apres le VBLANK N, sur la ligne y de l'ecran (212 lignes), est dans
l'image N si t < T(y) = (50 + y) x 63,7 us (262 lignes NTSC dont 212 affichees : le
VBLANK tombe 50 lignes avant la ligne 0), sinon dans l'image N + 1. La SAT lue au
VBLANK N+1 (ft_frames.csv, colonne sat) est celle de l'image N (les envois 6560 / 658A
ont lieu ~1 ms apres le VBLANK, avant la ligne 0). Un son demande (509F, C096) pendant
l'image N part au VBLANK N+1 (8964 tourne au debut de chaque interruption, 403B).

Sorties : pour chaque action « hold » du plan (touche enfoncee au VBLANK k0) : VBLANK de
la lecture (4BBB qui voit la touche, debut de passe), premiere image dont la SAT change,
sons demandes ; puis les commandes VDP (decor, HUD), CHGCLR et les sons, avec la passe
qui les calcule et l'image ou ils apparaissent.
"""
import csv
import re
import sys
from pathlib import Path

LINE_US = 63.7
LINES_BEFORE = 50


def image_of(frame, us, y):
    return frame if us < (LINES_BEFORE + y) * LINE_US else frame + 1


def main():
    out = Path(sys.argv[1])
    frames = {int(r['frame']): r for r in csv.DictReader((out / 'ft_frames.csv').open())}
    events = list(csv.DictReader((out / 'ft_events.csv').open()))
    for e in events:
        e['frame'] = int(e['frame'])
        e['us'] = int(e['us'])
    passes = sorted(e['frame'] for e in events if e['what'] == 'c003_inc')

    def pass_of(frame, us):
        """VBLANK de debut de la passe en cours a (frame, us)."""
        best = None
        for p in passes:
            if p < frame or (p == frame and us >= 0):
                best = p
            else:
                break
        return best

    def long_pass(p):
        """Passe de chargement (plus de 2 images) : dessin de salle, ignore ici."""
        i = passes.index(p)
        return i + 1 < len(passes) and passes[i + 1] - p > 2

    def sat(n):
        """Entrees de la SAT de l'image n, triees : 6560 tourne l'ordre a chaque passe (C00F)."""
        r = frames.get(n + 1)
        if not r:
            return None
        h = r['sat']
        return sorted(h[i:i + 8] for i in range(0, len(h), 8))

    print('Passes : %d debuts ; ecarts %s' % (len(passes), sorted({b - a for a, b in zip(passes, passes[1:]) if b - a < 4})))
    for what in ('keys', 'c003_inc', 'sat_upload', 'col_upload'):
        us = [e['us'] for e in events if e['what'] == what and e['frame'] > 800]
        if us:
            print('%-10s : %5d a %5d us apres le VBLANK (mediane %d)' % (what, min(us), max(us), sorted(us)[len(us) // 2]))

    holds = []
    for line in (out / 'events.txt').read_text().splitlines():
        m = re.match(r'frame=(\d+) .* BEGIN hold (\S+) (\S+)', line)
        if m:
            holds.append((int(m[1]), m[2], m[3]))
    print('\nEntrees (touche enfoncee au VBLANK k0) :')
    for k0, keys, label in holds:
        if label.startswith('s'):
            continue
        reads = [e['frame'] for e in events if e['what'] == 'keys' and e['frame'] >= k0]
        f0 = reads[0]
        base = sat(k0 - 1)
        first = next((n for n in range(k0, k0 + 30) if sat(n) != base), None)
        sounds = [(e['frame'], e['us'], e['a']) for e in events
                  if e['what'] == 'sound_req' and k0 <= e['frame'] < k0 + 12]
        print('  %-6s %-2s k0=%d lecture=%d (+%d) premiere image changee=%s (+%s) sons=%s' % (
            label, keys, k0, f0, f0 - k0, first, None if first is None else first - k0,
            ['%s @%d+%dus -> VBLANK %d' % (a, f, u, f + 1) for f, u, a in sounds]))

    loads = [p for p in passes if long_pass(p) and p > 800]
    print('\nPasses de chargement (> 2 images, dessin de salle) : %s' % [
        (p, passes[passes.index(p) + 1] - p) for p in loads])
    print('\nCommandes VDP hors animation et hors chargement (decor, HUD) :')
    for e in events:
        if e['what'] != 'vdp_cmd' or e['frame'] < 800 or e['a'] == '98':
            continue
        y = int(e['c'])
        p = pass_of(e['frame'], e['us'])
        if long_pass(p):
            continue
        img = image_of(e['frame'], e['us'], y)
        print('  VBLANK %d +%5dus cmd %s DX %s DY %s %sx%s : passe %d, image %d (passe + %d)' % (
            e['frame'], e['us'], e['a'], e['b'], e['c'], e['d'], e['e'], p, img, img - p))

    print('\nCHGCLR (bordure) :')
    for e in events:
        if e['what'] == 'chgclr' and e['frame'] > 800:
            p = pass_of(e['frame'], e['us'])
            print('  VBLANK %d +%dus F3EB=%s : passe %d ; image %d a partir de la ligne %d, image %d entiere' % (
                e['frame'], e['us'], e['a'], p, e['frame'],
                max(0, int(e['us'] / LINE_US) - LINES_BEFORE), e['frame'] + 1))

    print('\nSons (509F) :')
    for e in events:
        if e['what'] == 'sound_req' and e['frame'] > 800:
            p = pass_of(e['frame'], e['us'])
            print('  son %s demande VBLANK %d +%dus (passe %d) -> part au VBLANK %d (passe + %d)' % (
                e['a'], e['frame'], e['us'], p, e['frame'] + 1, e['frame'] + 1 - p))

    print('\nEcritures C415 / C417 / C42D (hors baisses 75D2) :')
    for e in events:
        if e['what'] in ('write_C415', 'write_C417', 'write_C42D') and e['frame'] > 800 and e['pc'] not in ('75D2', '63C3'):
            print('  %s=%s VBLANK %d +%dus PC %s (passe %d)' % (
                e['what'][6:], e['a'], e['frame'], e['us'], e['pc'], pass_of(e['frame'], e['us'])))
    hidden = [n for n in sorted(frames) if n > 800 and frames.get(n + 1) and frames[n + 1]['sat_y'] == '224'
              and frames.get(n) and frames[n]['sat_y'] != '224']
    print('  Simon cache (Y E0 dans la SAT) a partir des images %s' % hidden[:6])


if __name__ == '__main__':
    main()
