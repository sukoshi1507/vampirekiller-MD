#!/usr/bin/env python3
"""Analyse d'une sortie de tools/enemy_behavior_trace.tcl (+ tools/enemy_spawn_writers.tcl).

Usage : analyze_enemy_behavior.py RUNDIR
Lit frames.csv, sat.txt, et spawnwriters.txt si present. Aucun octet de ROM n'est lu.

Pour chaque creation d'objet (ecriture non nulle en C800/C880 hors initialisation,
image > 800) : salle (D001 au moment de l'ecriture), type (valeur ecrite), entree SAT
provisoire (Y=E1 ecrit par 5F66), premiere position reelle (Y != E0/E1), vitesse
moyenne en X (px/image), cadence de la pose basse (images entre deux changements de N
de l'entree basse), image de disparition (Y repasse a E0) et X a la disparition.
Sort aussi les baisses d'energie C415 et la liste des salles visitees.
"""
import bisect
import sys
from pathlib import Path


def load_frames(run):
    rows = {}
    for line in (run / 'frames.csv').read_text().splitlines()[1:]:
        p = line.split(',')
        rows[int(p[0])] = {'d0': int(p[2]), 'd1': int(p[3]), 'c0': int(p[4]),
                           'x': int(p[6]), 'y': int(p[7]), 'c415': int(p[8])}
    return rows


def load_sat(run):
    sat = {}
    for line in (run / 'sat.txt').read_text().splitlines():
        f, h = line.split()
        sat[int(f)] = bytes.fromhex(h)
    return sat


class SatSeries:
    """Valeur de SAT a une image donnee (sat.txt ne contient que les changements)."""

    def __init__(self, sat):
        self.keys = sorted(sat)
        self.sat = sat

    def at(self, frame):
        i = bisect.bisect_right(self.keys, frame) - 1
        return self.sat[self.keys[i]] if i >= 0 else None


def rooms_timeline(frames):
    segs = []
    for f in sorted(frames):
        key = (frames[f]['d0'], frames[f]['d1'])
        if not segs or segs[-1][0] != key:
            segs.append([key, f, f])
        else:
            segs[-1][2] = f
    return segs


def creations(run):
    path = run / 'spawnwriters.txt'
    if not path.exists():
        return []
    out = []
    for line in path.read_text().splitlines()[1:]:
        p = line.split(',')
        f, pc, addr, val = int(p[0]), p[1], p[2], int(p[3])
        if addr in ('C800', 'C880') and val != 0 and f > 800:
            out.append({'f': f, 'pc': pc, 'addr': addr, 'type': val,
                        'room': int(p[5]), 'c000': int(p[6])})
    return out


def provisional_slot(run, frame):
    """Entree SAT ecrite par 5F66 juste apres la creation (index SAT = (addr-D600)/4)."""
    path = run / 'spawnwriters.txt'
    for line in path.read_text().splitlines()[1:]:
        p = line.split(',')
        f = int(p[0])
        if p[1] == '5F66' and frame <= f <= frame + 3:
            return (int(p[2], 16) - 0xD600) // 4
    return None


def track(series, slot, start, end_limit):
    """Suit l'entree SAT 'slot' (Y, X, N) a partir de start. Retourne la liste des points."""
    pts = []
    prev = None
    for f in range(start, end_limit + 1):
        b = series.at(f)
        if b is None:
            continue
        cur = (b[4 * slot], b[4 * slot + 1], b[4 * slot + 2])
        if cur != prev:
            pts.append((f, cur[0], cur[1], cur[2]))
            prev = cur
        if f > start + 4 and cur[0] == 0xE0:
            break
    return pts


def bottom_cadence(series, slot_bottom, start, end):
    flips = []
    prev = None
    for f in range(start, end + 1):
        b = series.at(f)
        if b is None:
            continue
        n = b[4 * slot_bottom + 2]
        if prev is not None and n != prev:
            flips.append(f)
        prev = n
    gaps = [b - a for a, b in zip(flips, flips[1:])]
    return sorted(set(gaps))[:3]


def main():
    run = Path(sys.argv[1])
    frames = load_frames(run)
    series = SatSeries(load_sat(run))
    last = max(frames)
    print(f"# {run.name} : {len(frames)} images")
    print("## Segments de salle (D000/D001 : premiere image - derniere image)")
    for key, a, b in rooms_timeline(frames):
        print(f"salle {key[0]}/{key[1]} : {a}-{b}")
    print("## Creations d'objets (C800/C880, hors initialisation)")
    for c in creations(run):
        slot = provisional_slot(run, c['f'])
        if slot is None:
            print(f"creation image {c['f']} salle {c['room']} type {c['type']} {c['addr']} "
                  f"(entree SAT non trouvee)")
            continue
        pts = track(series, slot, c['f'], min(last, c['f'] + 400))
        real = [p for p in pts if p[1] not in (0xE0, 0xE1)]
        if not real:
            print(f"creation image {c['f']} salle {c['room']} type {c['type']} {c['addr']} "
                  f"SAT {slot}: pas de position reelle")
            continue
        f0, y0, x0, n0 = real[0]
        end = [p for p in pts if p[1] == 0xE0 and p[0] > f0]
        endtxt = f"disparait image {end[0][0]} (X {pts[-1][2]:02X})" if end else "pas de disparition"
        # vitesse : pente entre premiere et derniere position reelle avant disparition
        last_real = [p for p in real if (not end) or p[0] < end[0][0]]
        span = max(1, last_real[-1][0] - f0)
        dx = (last_real[-1][2] - x0) / span
        cad = bottom_cadence(series, slot + 2, f0, f0 + 200)
        print(f"creation image {c['f']} salle {c['room']} type {c['type']} {c['addr']} SAT {slot} : "
              f"position reelle image {f0} (X {x0:02X}, Y {y0:02X}, N {n0:02X}), "
              f"dX/img {dx:+.2f}, cadence pose basse {cad}, {endtxt}")
    print("## Baisses d'energie C415 (contact ennemi = -2)")
    prev = None
    for f in sorted(frames):
        r = frames[f]
        if prev is not None and r['c415'] < prev['c415']:
            print(f"image {f} salle {r['d0']}/{r['d1']} C415 {prev['c415']} -> {r['c415']} "
                  f"Simon ({r['x']},{r['y']})")
        prev = r


if __name__ == '__main__':
    main()
