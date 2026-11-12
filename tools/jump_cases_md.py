#!/usr/bin/env python3
"""Mega Drive port: Simon's jump from the same (stage, room, X, Y, direction) points as tools/run_jump_cases.py (MSX),
one line per frame: `name n x= y= mode= phase=`. n = 1 is the first frame run with the jump key down.
Usage: jump_cases_md.py OUTFILE [CASES.json]  (VK_MD_CORE = libretro core, default genesis_plus_gx)."""
import json, os, sys
from pathlib import Path
from probe import ROOT
from md_harness import Machine, Game_View, load_symbols, lookup, BUTTONS, DEFAULT_CORE
from run_jump_cases import DEFAULT

C431 = 320   # offsetof(VKCandles, c431), checked with gcc on candles.h


def run_cases(cases, core):
    syms = load_symbols(elf=ROOT / 'sgdk/out/rom.out')
    m = Machine(core, ROOT / 'sgdk/out/rom.bin')
    Game_View(m, syms)
    addr = lambda n: lookup(syms, n)[0]
    g, f, p, mode = addr('vk_game'), addr('room_freeze'), addr('vk_player'), addr('vk_front_mode')

    def until(fn, n):
        for _ in range(n):
            if fn():
                return True
            m.run(1, set())
        return False

    until(lambda: m.rd16(mode) == 1, 2000)
    m.run(4, {BUTTONS['START']}); m.run(4, set()); m.run(4, {BUTTONS['START']}); m.run(6, set())
    if not until(lambda: m.rd16(mode) == 3, 600):
        raise SystemExit('no courtyard after title')
    out = []
    for name, stage, room, x, y, d, high, frames, walk in cases:
        m.wr(g + 16, 1, 32); m.wr(g + 20, room); m.wr(g + 22, stage); m.wr(mode, 4, 16)
        m.wr(g + 0, x); m.wr(g + 1, y)
        for o in list(range(2, 15)) + list(range(23, 36)):   # no stairs, recoil, jump state left by the previous case
            m.wr(g + o, 0)
        for i in range(15):
            m.wr(p + i, m.rd8(g + i))
        m.wr(addr('health') + 1, 0); m.wr(addr('health') + 2, 0, 16); m.wr(addr('Enemies_god'), 1)
        m.wr(addr('freeze_drawn'), 0); m.wr(f, 4); m.wr(f + 1, 4); m.wr(f + 2, 0)
        m.wr(addr('freeze_vtimer'), m.rd32(addr('vtimer')), 32)
        if not until(lambda: m.rd8(f) == 0 and m.rd8(addr('freeze_drawn')), 120):
            raise SystemExit('entry failed ' + name)
        dirs = {'L': {BUTTONS['LEFT']}, 'R': {BUTTONS['RIGHT']}, 'N': set(), 'U': {BUTTONS['UP']},
                'D': {BUTTONS['DOWN']}}[d]
        c431 = addr('vk_candles') + C431
        for i in range(40):   # settle on the floor, as the MSX run does
            m.wr(c431, 16 if high == 1 else 0)
            if i == 30:
                m.wr(g + 0, x); m.wr(g + 1, y)
            m.run(1, set())
        for i in range(0 if d in 'UD' else walk):   # UP held early would mount the stairs before X, Y are set
            m.wr(c431, 16 if high == 1 else 0)
            m.run(1, dirs)
        m.wr(g + 0, x); m.wr(g + 1, y)
        for n in range(1, frames + 1):
            m.wr(c431, 16 if high == 1 else 0)
            m.wr(addr('Enemies_god'), 1)
            m.run(1, dirs | ({BUTTONS['C']} if n <= 4 and d not in 'UD' and high != 2 else set()))
            out.append('%s %d x=%d y=%d mode=%d phase=%d' % (name, n, m.rd8(g), m.rd8(g + 1), m.rd8(g + 3), m.rd8(g + 4)))
    m.close()
    return out


def main():
    cases = ([(c['name'], c['stage'], c['room'], c['x'], c['y'], c['dir'], c.get('high', 0), c.get('frames', 120), c.get('walk', 8)) for c in json.loads(Path(sys.argv[2]).read_text())]
             if len(sys.argv) > 2 else [c + (120, 8) for c in DEFAULT])
    lines = run_cases(cases, os.environ.get('VK_MD_CORE', DEFAULT_CORE))
    Path(sys.argv[1]).write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
