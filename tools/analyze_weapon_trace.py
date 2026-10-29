#!/usr/bin/env python3
"""Summarise a run of tools/run_weapon_trace.py (research/secondary-weapons.md).

Usage: analyze_weapon_trace.py OUTDIR [FROM TO]
For every life of a slot (C450, C460: +1 != 0) prints its type, the frame of the
launch and of the end, the passes in between, Simon (X, Y, C420, C42C) at the launch
and the slot path (state, X, Y, pattern) at each pass. With FROM TO, the per-frame
rows of that window.
"""
import csv
import sys
from pathlib import Path


def main():
    out = Path(sys.argv[1])
    rows = list(csv.DictReader((out / 'weapon.csv').open()))
    if len(sys.argv) == 4:
        lo, hi = int(sys.argv[2]), int(sys.argv[3])
        for r in rows:
            if lo <= int(r['frame']) <= hi:
                print(r['frame'], 'simon', r['x'], r['y'], r['c420'], 'c003', r['c003'], 'c422', r['c422'],
                      'c42f', r['c42f'], 'a', ' '.join(r[f'a{i}'] for i in range(9)),
                      'b', ' '.join(r[f'b{i}'] for i in range(9)), 'hearts', r['c417'], 'c416', r['c416'])
        return 0
    for slot in 'ab':
        life = None
        for r in rows:
            t = int(r[f'{slot}1'])
            if t and life is None:
                life = {'start': int(r['frame']), 'type': t, 'simon': (r['x'], r['y'], r['c420'], r['c42c']),
                        'path': []}
            if life is not None:
                key = (r[f'{slot}0'], r[f'{slot}5'], r[f'{slot}4'], r[f'{slot}6'])
                if not life['path'] or life['path'][-1][1] != key:
                    life['path'].append((int(r['frame']), key))
            if not t and life is not None:
                end = int(r['frame'])
                print(f"slot {slot} type {life['type']} frames {life['start']}..{end} ({end - life['start']}) "
                      f"simon x,y,c420,c42c {life['simon']} c416 after {r['c416']}")
                print('   ', ' '.join(f"{f}:{s}/{x},{y},{p:>2}" for f, (s, x, y, p) in life['path']))
                life = None
    return 0


if __name__ == '__main__':
    sys.exit(main())
