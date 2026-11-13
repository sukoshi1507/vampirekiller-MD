"""Compares the port (Genesis Plus GX) with a platform_fall_trace.tcl log case by case (Y, X per frame, both frame alignments).
Usage: VK_MD_CORE=... NF=14 PRESTEP=1 python tools/platform_fall_compare.py LOG.txt   (SHOW=<case> prints one case; JUMP=4 for jump cases)
PRESTEP: the parity 0 cases poke the platform after its step of the pass, so the port gets the value before it."""
import sys, os, re
sys.path.insert(0, 'tools')
from pathlib import Path
import md_harness as H
root = Path('.').resolve()
symbols = H.load_symbols(root / 'sgdk/out/release/rom.out', root / 'sgdk/out/release/symbol.txt')
h = H.Harness(Path(os.environ['VK_MD_CORE']), root / 'sgdk/out/rom.bin', symbols, root / 'scratchpad/platform_fall_md', True)
lines = []
for l in Path('tests/md_scenarios/zzm_stage10_stalactite.txt').read_text().splitlines():
    lines.append(l)
    if l.startswith('wait 10'): break
h.run_script(lines, Path('tests/md_scenarios'))
m = h.machine
A = lambda n: H.lookup(symbols, n)[0]
pl, g, eg = A('vk_platforms'), A('vk_game'), A('Enemies_god')
# parse MSX
cases = []
for l in open(sys.argv[1]):
    if l.startswith('CASE'):
        d = dict(re.findall(r'(\w+)=(-?\d+)', l)); cases.append((d, []))
    else:
        mm = re.match(r'(\d+) y=(\d+) x=(\d+) c420=(\d+) c428=(\d+) c439=(\d+) px=(\d+) dx=(\d+)', l)
        if mm and cases: v = list(map(int, mm.groups())); cases[-1][1].append((v[1], v[2], v[3] == 4, v[5], v[6]))
def port_run(d, off, n=int(os.environ.get("NF","24"))):
    if m.rd8(g+20) != 4: print("ROOM", m.rd8(g+20), "at case", d)
    m.wr(eg, 1)
    m.wr(g + 0, 40); m.wr(g + 1, 144); m.wr(g + 2, 0)
    m.run(12, frozenset())
    m.wr(eg, 1)
    for _ in range(off): m.run(1, frozenset())
    dxs = int(d['dx']) - 256 if int(d['dx']) > 127 else int(d['dx'])
    px, cnt = int(d['px']), int(d['cnt'])
    if int(d['par']) == 0 and os.environ.get('PRESTEP'): px = (px - dxs) & 255; cnt -= 1
    m.wr(pl + 2, px); m.wr(pl + 3, int(d['dx'])); m.wr(pl + 6, cnt); m.wr(pl+14, 0)
    m.wr(g + 0, int(d['x0'])); m.wr(g + 1, int(d['y0'])); m.wr(g + 2, 0)
    out = []
    for i in range(n):
        m.run(1, (frozenset({H.BUTTONS['RIGHT'], H.BUTTONS['C']}) if i < int(os.environ.get('JUMP','0')) else frozenset({H.BUTTONS['RIGHT']})) if os.environ.get('JUMP') else frozenset()); m.wr(eg, 1)
        out.append((m.rd8(g + 1), m.rd8(g + 0), m.rd8(g + 2) == 4, m.rd8(pl + 14), m.rd8(pl + 2)))
        if out[-1][0] >= 212 and out[-1][0] < 250: break
    return out
tot = {}
res = []
for d, rows in cases:
    best = None
    for off in (0, 1):
        pr = port_run(d, off)
        for s in (-1, 0, 1):
            mis = 0
            for i, r in enumerate(rows[:min(len(pr)+1, int(os.environ.get("NF","24")))]):
                j = i + s
                if 0 <= j < len(pr):
                    if (pr[j][0], pr[j][1]) != (r[0], r[1]): mis += 1
            key = (mis, off, s)
            if best is None or key < best[0]: best = (key, pr)
    res.append((d, best[0]))
    print(d['y0'], d['d'], d['par'], 'mis,off,shift', best[0])
if os.environ.get('SHOW'):
    k = int(os.environ['SHOW']); d, rows = cases[k]
    pr = port_run(d, 0)
    for i in range(min(len(pr),70)): print(i, rows[i] if i < len(rows) else None, pr[i])
if os.environ.get('SHOW'):
    print([m.rd8(pl+i) for i in range(16)], [m.rd8(g+i) for i in range(20)])
