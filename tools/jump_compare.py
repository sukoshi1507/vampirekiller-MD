#!/usr/bin/env python3
"""Compare the MSX2 jump trajectory (openMSX trace) with the native MD simulation.

Usage: jump_compare.py MSX_RUN_DIR SIM_CSV MSX_ONSET_FRAME [--col c425] [--sim y] [--count 46]
  MSX_RUN_DIR     : output of tools/jump_trace.tcl (frames.csv, one row per VBLANK).
  SIM_CSV         : output of tools/jump_sim.c (frame,y,x,mode,phase,pose,clock).
  MSX_ONSET_FRAME : frame number of the UP press sample in frames.csv (1257 for still).
Prints a Markdown table: k, MSX value, MD value, difference. MD frame 0 is the press.
"""
import argparse
import csv


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('msx_dir')
    p.add_argument('sim_csv')
    p.add_argument('onset', type=int)
    p.add_argument('--col', default='c425', help='MSX column (c425 = Y, c427 = X)')
    p.add_argument('--sim', default='y', help='sim column (y or x)')
    p.add_argument('--count', type=int, default=46)
    a = p.parse_args()
    msx = {int(r['frame']): r for r in csv.DictReader(open(a.msx_dir + '/frames.csv'))}
    sim = list(csv.DictReader(open(a.sim_csv)))
    print('| k | MSX ' + a.col + ' | MD ' + a.sim + ' | difference |')
    print('|---:|---:|---:|---:|')
    for k in range(a.count):
        m = msx.get(a.onset + k)
        s = sim[k] if k < len(sim) else None
        mv = int(m[a.col]) if m else None
        sv = int(s[a.sim]) if s else None
        diff = (mv - sv) if (mv is not None and sv is not None) else '-'
        print(f'| {k} | {mv if mv is not None else "-"} | {sv if sv is not None else "-"} | {diff} |')


if __name__ == '__main__':
    main()
