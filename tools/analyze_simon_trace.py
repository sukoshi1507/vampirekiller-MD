#!/usr/bin/env python3
"""Analyze observed openMSX animation-index traces without guessing action names."""
import argparse
import csv
import json
from collections import Counter
from pathlib import Path

FIELDS = ('lower_c42e', 'upper_c42f')
PROBE_MARKERS = {'experimental_input_held', 'experimental_input_released'}

def analyze(path):
    with Path(path).open(newline='') as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise ValueError('Trace is empty')
    for field in ('tick', 'marker', *FIELDS):
        if field not in rows[0]:
            raise ValueError(f'Missing trace field: {field}')
    segments = []
    last_tick = None
    ticks_missing = 0
    for row in rows:
        tick = int(row['tick'])
        pair = tuple(int(row[field]) for field in FIELDS)
        if any(not (0 <= n <= 255) for n in pair):
            raise ValueError(f'Animation index outside byte range at tick {tick}')
        marker = row['marker']
        if last_tick is not None:
            if tick <= last_tick:
                raise ValueError('Non-increasing interrupt counter')
            ticks_missing += tick - last_tick - 1
        last_tick = tick
        if segments and segments[-1]['marker'] == marker and segments[-1]['pair'] == list(pair) and tick == segments[-1]['last_tick'] + 1:
            segments[-1]['last_tick'] = tick
            segments[-1]['samples'] += 1
        else:
            segments.append({'marker': marker, 'pair': list(pair), 'first_tick': tick,
                             'last_tick': tick, 'samples': 1})
    markers = {}
    for segment in segments:
        record = markers.setdefault(segment['marker'], {'samples': 0, 'segments': 0, 'pairs': Counter()})
        record['samples'] += segment['samples']
        record['segments'] += 1
        record['pairs'][tuple(segment['pair'])] += segment['samples']
    observed = {tuple(segment['pair']) for segment in segments}
    has_gameplay = any(s['marker'] == 'gameplay_unclassified' for s in segments)
    has_probe = any(s['marker'] in PROBE_MARKERS for s in segments)
    warnings = []
    if not has_gameplay:
        warnings.append('No gameplay_unclassified marker: gameplay entry unconfirmed')
    if not has_probe:
        warnings.append('No experimental input pulse captured')
    if ticks_missing:
        warnings.append(f'{ticks_missing} interrupt indices missing from CSV')
    if len(observed) <= 1:
        warnings.append('Only one animation pair observed: no frame progression demonstrated')
    return {
        'sample_count': len(rows), 'segment_count': len(segments),
        'tick_gaps': ticks_missing, 'distinct_pairs': len(observed),
        'quality': {'gameplay_marker_seen': has_gameplay, 'probe_marker_seen': has_probe,
                    'attack_confirmed': False, 'warnings': warnings},
        'markers': {name: {'samples': v['samples'], 'segments': v['segments'],
                   'pairs': [{'lower': lo, 'upper': up, 'samples': n}
                             for (lo, up), n in v['pairs'].most_common()]}
                    for name, v in markers.items()},
        'segments': segments,
        'caveat': 'Observed CPU interrupt values only. A probe is not a verified attack or a displayed frame.'
    }

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('csv', type=Path)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    result = analyze(args.csv)
    output = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output)
    else:
        print(output, end='')

if __name__ == '__main__':
    main()
