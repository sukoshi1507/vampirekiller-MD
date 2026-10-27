#!/usr/bin/env python3
"""Chronologie des sprites ennemis de la salle 1 a partir d'une trace RAM openMSX.

Entree : fichier frames.txt produit par une trace VBLANK (4028) qui journalise
la SAT RAM en D600-D67F (32 entrees Y, X, N, couleur). Aucune ROM n'est lue ici.
Sortie : JSON sur stdout, une entree par groupe de sprites ennemis (N >= 0x40),
avec les images ou chaque pose apparait et les durees de pose.

Usage : extract_enemy_timeline.py FRAMES_TXT [--first 900 --last 1100]
Statut des donnees : prouve par la trace (RAM SAT). Les motifs (tuiles) ne sont
pas extraits ici : voir research/enemy-assets.md.
"""
import argparse
import json
from pathlib import Path


def read_frames(path):
    frames = {}
    for line in Path(path).read_text().splitlines():
        parts = line.split()
        if not parts or parts[0] != 'F':
            continue
        index = int(parts[1])
        d = bytes.fromhex(parts[parts.index('D') + 1])
        if len(d) != 128:
            raise ValueError(f'Bloc SAT de taille inattendue a l image {index}')
        frames[index] = d
    return frames


def enemy_entries(sat):
    """Entrees visibles avec un numero de motif ennemi (>= 0x40)."""
    entries = []
    for k in range(0, 128, 4):
        y, x, n, c = sat[k:k + 4]
        if y >= 0xD8 or n < 0x40:
            continue
        entries.append({'slot': k // 4, 'y': y, 'x': x, 'n': n, 'color': c})
    return entries


def group_by_x(entries):
    groups = {}
    for e in entries:
        groups.setdefault(e['x'], []).append(e)
    return groups


def runs(sequence):
    out = []
    for frame, pose in sequence:
        if out and out[-1]['pose'] == pose and out[-1]['last'] + 1 == frame:
            out[-1]['last'] = frame
            out[-1]['frames'] += 1
        else:
            out.append({'pose': pose, 'first': frame, 'last': frame, 'frames': 1})
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('frames', type=Path)
    parser.add_argument('--first', type=int, default=900)
    parser.add_argument('--last', type=int, default=1100)
    args = parser.parse_args()
    frames = read_frames(args.frames)
    # Un suivi par X de sprite haut (Y de l'entree la plus haute) : pose = N du motif bas.
    sequence = []
    for index in range(args.first, args.last + 1):
        if index not in frames:
            continue
        entries = enemy_entries(frames[index])
        lower = sorted({e['n'] for e in entries if e['y'] == 0xB0})
        upper = sorted({e['n'] for e in entries if e['y'] == 0xA0})
        sequence.append((index, {'upper': upper, 'lower': lower,
                                 'x': sorted({e['x'] for e in entries})}))
    summary = []
    for run in runs([(f, (tuple(p['upper']), tuple(p['lower']))) for f, p in sequence]):
        summary.append({'first_frame': run['first'], 'last_frame': run['last'],
                        'frames': run['frames'],
                        'upper_N': [f'{n:02X}' for n in run['pose'][0]],
                        'lower_N': [f'{n:02X}' for n in run['pose'][1]]})
    print(json.dumps({'source': str(args.frames), 'range': [args.first, args.last],
                      'segments': summary}, indent=2))


if __name__ == '__main__':
    main()
