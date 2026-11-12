#!/usr/bin/env python3
"""Compares tools/run_jump_cases.py (MSX, jump.txt) with tools/jump_cases_md.py (port) case by case.
Usage: jump_cases_compare.py MSX_jump.txt MD.txt [-v]. Frames are video frames (the MSX game pass lasts 2 frames, C003 += 1;
the port steps its jump every second frame, see player.c). Per case: start latency (n of the first Y change), apex, landing
(n where C420 / mode is back to 0 on the ground, or the end), final (X, Y), and the first frame where the aligned Y/X differ.
Alignment: the MSX pass phase relative to the key is free (0 or 1 frame), so the MSX series is shifted by the difference of the
first-step frames, the shift is reported. Exit code 1 if some case differs after the alignment."""
import re, sys
from collections import OrderedDict


def read(path, kind):
    cases = OrderedDict()
    for line in open(path):
        p = line.split()
        if len(p) < 3:
            continue
        kv = dict(t.split('=') for t in p[2:] if '=' in t)
        if kind == 'msx':
            row = (int(p[1]), int(kv['x']), int(kv['y']), int(kv['c420']))
        else:
            row = (int(p[1]), int(kv['x']), int(kv['y']), int(kv['mode']))
        cases.setdefault(p[0], []).append(row)
    return cases


def first_step(rows):
    y0 = rows[0][2]
    for n, x, y, _ in rows:
        if y != y0:
            return n
    return None


def landing(rows, n0):
    """First n after the first step with mode 0 on the ground (C420 / mode == 0)."""
    for n, x, y, m in rows:
        if n0 is not None and n > n0 and m == 0:
            return n
    return None


def main():
    msx, md = read(sys.argv[1], 'msx'), read(sys.argv[2], 'md')
    verbose = '-v' in sys.argv
    bad = 0
    if '-x' in sys.argv:   # exact: same (X, Y) at the same frame (stairs, no alignment)
        for name in md:
            if name not in msx:
                continue
            a, b = dict((r[0], r[1:3]) for r in msx[name]), dict((r[0], r[1:3]) for r in md[name])
            same = sum(a[n] == b[n] for n in a if n in b)
            first = next((n for n in sorted(a) if n in b and a[n] != b[n]), None)
            bad += same != len(a)
            print('%-22s %3d/%3d frames identical, first difference n=%s %s' % (
                name, same, len(a), first, '' if first is None else 'MSX %s MD %s' % (a[first], b[first])))
        return 1 if bad else 0
    print('%-22s %5s %5s %5s %5s %9s %9s %s' % ('case', 'lat', 'apex', 'land', 'dur', 'end X,Y', 'shift', 'verdict'))
    for name in md:
        if name not in msx:
            continue
        a, b = msx[name], md[name]
        fa, fb = first_step(a), first_step(b)
        # MSX moves X before the first Y step (walk), so align on the first Y step.
        shift = (fb - fa) if (fa and fb) else 0
        la, lb = landing(a, fa), landing(b, fb)
        diffs = []
        for n, x, y, m in a:
            q = [r for r in b if r[0] == n + shift]
            if not q or (la and n > la + 4):
                continue
            if q[0][2] != y or abs(q[0][1] - x) > 2:
                diffs.append((n, x, y, q[0][1], q[0][2]))
        apex_a, apex_b = min(r[2] for r in a[:(la or len(a))]), min(r[2] for r in b[:(lb or len(b))])
        ok = (la is not None and lb is not None and la + shift == lb and apex_a == apex_b and not diffs
              and a[(la or 1) - 1 + 3][2] == b[(lb or 1) - 1 + 3][2])
        bad += not ok
        ea = a[min(len(a), (la or len(a)) + 3) - 1]; eb = b[min(len(b), (lb or len(b)) + 3) - 1]
        print('%-22s %5s %5s %5s %5s %9s %9s %s' % (
            name, '%s/%s' % (fa, fb), '%s/%s' % (apex_a, apex_b), '%s/%s' % (la, lb), '%s/%s' % ((la - fa) if la and fa else None, (lb - fb) if lb and fb else None),
            '%d,%d|%d,%d' % (ea[1], ea[2], eb[1], eb[2]), shift, 'SAME' if ok else 'DIFF %s' % diffs[:3]))
        if verbose:
            for n, x, y, m in a:
                q = [r for r in b if r[0] == n + shift]
                if q:
                    print('   n=%3d MSX x=%3d y=%3d c420=%d | MD x=%3d y=%3d mode=%d' % (n, x, y, m, q[0][1], q[0][2], q[0][3]))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
