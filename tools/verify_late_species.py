#!/usr/bin/env python3
"""Late enemy graphics table against the species really created on the MSX (openMSX traces, stages 12-18).

research/late-species-msx.json holds, per room, the raw C800 types, D700 kinds and C800 +11 poses seen over 900
images with Simon left of X C0 (tools/run_species_trace.py, one run per stage; `--merge DIR...` rebuilds the
file from the species.txt of those runs). This check, without an emulator, asserts that
  1. every species seen in a room (C800 raw 9/11/13/15/16/18/27/35, D700 kinds 4 and 9) has cells in that room's
     late_graphics.inc table (mapping raw -> internal type of SPECS), whatever channel created it;
  2. the channels of tools/build_late_graphics.py are exactly what the traces show: fixed list (6154),
     flag 64 (9DE7: eagle 0F and its flea 23h, whose images equal those of 0D), companions 04/D700 of 0B and the
     shot of 10h; nothing else appears outside the boss rooms, which own their banks;
  3. the poses seen are among the poses the table is generated for.
Usage: verify_late_species.py [--merge DIR...]"""
import json, re, sys
from pathlib import Path
from probe import ROOT, load_rom, reference_rom
from build_late_graphics import SPECS

DATA = ROOT / 'research/late-species-msx.json'
# C800 raw type -> internal type of the port (SPECS); 35 (flea 23h) draws as 13 (0D): same cells, checked in the generator.
CMAP = {s[0]: s[1] for s in SPECS}
CMAP.update({35: 14, 27: 15, 14: 19})
DMAP = {1: 10, 4: 13, 9: 21}          # D700 byte 0: F0 projectile, bones of 0B, shot of 10h
BOSS_ROOMS = {(12, 6), (15, 9), (16, 8), (18, 9)}
POSES = {s[0]: set(s[2]) for s in SPECS}
POSES[35] = {0, 0x67, 0x68, 0x6A, 0x6B}
POSES[15] = {0x6D, 0x6E, 0x8D}
EAGLE_CONSTRUCTOR_POSE = 0x67           # seen once at x = y = 0 before the first 6445 pass


def merge(dirs):
    rooms = {}
    for d in dirs:
        for line in (Path(d) / 'species.txt').read_text().splitlines():
            m = re.match(r'(\d+) (\d+) ([CD]) (\d+) frame=', line)
            if m:
                r = rooms.setdefault('%s:%s' % m.group(1, 2), {'C': set(), 'D': set(), 'poses': {}})
                r[m.group(3)].add(int(m.group(4)))
            m = re.match(r'POSE (\d+) (\d+) (\d+) (\d+) ', line)
            if m:
                r = rooms.setdefault('%s:%s' % m.group(1, 2), {'C': set(), 'D': set(), 'poses': {}})
                r['poses'].setdefault(m.group(3), set()).add(int(m.group(4)))
    out = {k: {'C': sorted(v['C']), 'D': sorted(v['D']), 'poses': {t: sorted(p) for t, p in sorted(v['poses'].items())}}
           for k, v in sorted(rooms.items(), key=lambda kv: tuple(map(int, kv[0].split(':'))))}
    DATA.write_text(json.dumps({'method': 'openMSX, rooms forced by RAM, 900 images, Simon at X 64 (see tools/run_species_trace.py)',
                                'rooms': out}, indent=1) + '\n')
    print('merged', len(out), 'rooms')


def table():
    txt = (ROOT / 'sgdk/src/generated/late_graphics.inc').read_text()
    rooms = re.search(r'late_graphics_rooms\[7\]\[12\]=\{(.*?)\};', txt, re.S).group(1)
    rl = [[tuple(map(int, q.split(','))) for q in re.findall(r'\{([\d,]+)\}', row)]
          for row in re.findall(r'\{((?:\{[\d,]+\},?)+)\}', rooms)]
    cells = [tuple(map(int, q.split(','))) for q in re.findall(r'\{(\d+,\d+,\d+)\}', re.search(r'late_graphics_cells\[\]=\{(.*?)\};', txt, re.S).group(1))]
    return {(st, r): {c[0] for c in cells[g[2]:g[2] + g[3]]} for st, row in zip(range(12, 19), rl) for r, g in enumerate(row)}


def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--merge':
        return merge(sys.argv[2:])
    rom = load_rom(reference_rom())
    data = json.loads(DATA.read_text())['rooms']
    fixed = json.loads((ROOT / 'research/late-fixed-enemies-z80.json').read_text())['stages']
    tab = table(); errors = []; n = 0
    for st in range(12, 19):
        ptr = int.from_bytes(rom[14 * 8192 + 0x5A6 + 2 * st:14 * 8192 + 0x5A8 + 2 * st], 'little')
        for r in range(len(fixed[st - 12]['rooms'])):
            k = '%d:%d' % (st, r)
            if k not in data:
                if (st, r) not in BOSS_ROOMS and any(q['raw_type'] for q in fixed[st - 12]['rooms'][r]):
                    errors.append((k, 'fixed enemies but no MSX observation'))
                continue
            d = data[k]; n += 1
            flag = rom[14 * 8192 + ptr - 0x8000 + r]
            fx = {q['raw_type'] & 127 for q in fixed[st - 12]['rooms'][r]} - {0}
            boss = (st, r) in BOSS_ROOMS
            for t in d['C']:
                if t in CMAP:
                    if CMAP[t] not in tab[(st, r)]:
                        errors.append((k, 'raw %d (internal %d) seen on the MSX, absent from the table %s' % (t, CMAP[t], sorted(tab[(st, r)]))))
                elif not boss:
                    errors.append((k, 'raw %d seen outside a boss room and not described' % t))
                # channels
                allowed = t in fx or (flag & 64 and t in (15, 35)) or (t == 4 and 11 in fx) or boss
                if not allowed:
                    errors.append((k, 'raw %d has no channel in build_late_graphics.py (fixed %s flag %d)' % (t, sorted(fx), flag)))
            for t in d['D']:
                if t in DMAP:
                    if DMAP[t] not in tab[(st, r)]:
                        errors.append((k, 'D700 kind %d (internal %d) absent from the table' % (t, DMAP[t])))
                elif not boss:
                    errors.append((k, 'D700 kind %d outside a boss room not described' % t))
            for t in fx - set(d['C']):
                if not boss:
                    errors.append((k, 'fixed raw %d never created' % t))
            if flag & 64:
                if not {15, 35} <= set(d['C']):
                    errors.append((k, 'flag 64 room without eagle 15 and flea 35: %s' % d['C']))
            elif {15, 35} & set(d['C']):
                errors.append((k, 'raw 15/35 without flag 64'))
            for t, ps in d['poses'].items():
                t = int(t)
                if t in POSES:
                    extra = set(ps) - POSES[t] - ({EAGLE_CONSTRUCTOR_POSE} if t == 15 else set())
                    if extra:
                        errors.append((k, 'raw %d poses %s not generated' % (t, sorted(map(hex, extra)))))
    for k, e in errors:
        print('FAIL', k, e)
    assert not errors, '%d inconsistencies' % len(errors)
    print('Late species: %d rooms of the MSX traces covered by the late graphics tables, channels and poses: PASS' % n)


if __name__ == '__main__':
    main()
