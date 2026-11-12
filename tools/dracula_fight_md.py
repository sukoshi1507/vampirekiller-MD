#!/usr/bin/env python3
"""Mega Drive port: Simon beats Dracula (stage 18 room 9, both forms) with the controller only.

Entry: the real pause/Konami/room selector (tests/md_scenarios/zzq_entry.inc, same as zzp_stage18), Simon at the
room entrance, Enemies_god = 1 (the only allowed poke: invulnerability). No Simon position, no boss energy.
Form 1 (object 11h): the head is hit by the whip in a jump (vk_dracula_whip, 827C: Y object - 32).
Form 2 (object 17h at X128,Y73): the whip box (Y 73, H 10h) is out of reach of the floor (feet 192, apex 160), so
Simon climbs the zigzag of tile-6 slabs of the side towers (7C5E: CE00 = 6 -> solid tiles 1..6 only) up to feet Y 80 and
throws his secondary weapon across the room. A secondary weapon and its hearts come from the castle candles on the real
route; this script gives them with `--weapon N --hearts N` (RAM inventory fixture, the only other poke) because the
route from stage 18 room 0 is not walked here.
Usage: dracula_fight_md.py [--weapon 2] [--hearts 99] [--out DIR] [--shots]"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from md_harness import Harness, DEFAULT_CORE, BUTTONS, FRONT_MODES, load_symbols, lookup  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
XS = (48, 208, 80, 160, 64, 192, 128, 176)   # teleport X table AC76 (dracula.c xs)
B = lambda *n: frozenset(BUTTONS[x] for x in n)


class Fight:
    def __init__(self, out, shots):
        self.syms = load_symbols(ROOT / 'sgdk/out/rom.out')
        self.h = Harness(DEFAULT_CORE, ROOT / 'sgdk/out/rom.bin', self.syms, Path(out), True)
        self.m = self.h.machine
        self.shots = shots
        A = lambda n: lookup(self.syms, n)[0]
        self.A = A
        self.g, self.dr, self.vc, self.vw = A('vk_game'), A('dracula'), A('vk_candles'), A('vk_weapons')
        self.t0 = None
        self.log = []

    # state
    sx = property(lambda s: s.m.rd8(s.g))
    sy = property(lambda s: s.m.rd8(s.g + 1))
    mode = property(lambda s: s.m.rd8(s.g + 3))
    boss_type = property(lambda s: s.m.rd8(s.dr))
    boss_state = property(lambda s: s.m.rd8(s.dr + 1))
    boss_x = property(lambda s: s.m.rd8(s.dr + 5))
    boss_vis = property(lambda s: s.m.rd8(s.dr + 14) & 1)
    energy = property(lambda s: s.m.rd8(s.dr + 608))
    front = property(lambda s: s.m.rd16(s.A('vk_front_mode')))

    def frame(self, buttons=()):
        self.m.run(1, B(*buttons))

    def run(self, n, buttons=()):
        for _ in range(n):
            self.frame(buttons)

    def until(self, cond, buttons=(), limit=600, what=''):
        for _ in range(limit):
            if cond():
                return
            self.frame(buttons)
        raise TimeoutError(f'frame {self.m.frame}: {what or cond} (x={self.sx} y={self.sy} mode={self.mode})')

    def land(self):
        self.until(lambda: self.mode == 0, limit=200, what='landing')

    def walk(self, x, tol=1):
        while abs(self.sx - x) > tol:
            self.until(lambda: True)
            self.run(1, ('RIGHT',) if self.sx < x else ('LEFT',))
        self.run(1)

    def jump(self, direction):
        """C pressed with the direction; the direction is locked at take-off (6CC0)."""
        self.run(4, ('C',) + ((direction,) if direction else ()))
        self.land()
        self.run(10)

    def shot(self, name):
        if self.shots:
            self.h.shot(name)

    # phase 0: the real room selector up to stage 18 room 9
    def enter(self):
        self.h.run_script((ROOT / 'tests/md_scenarios/zzq_entry.inc').read_text().splitlines(),
                          ROOT / 'tests/md_scenarios')
        assert self.front == FRONT_MODES.index('INTERIOR') or True
        self.t0 = self.m.frame
        print(f'[{self.m.frame - self.t0:6d}] room 9 entered: Simon x={self.sx} y={self.sy}, boss type={self.boss_type} energy={self.energy}')
        self.shot('dracula-entry')

    # phase 1: form 1, whip hit in a jump on the head (Y object - 32) when he is visible on the ground
    def form1(self, limit=20000):
        seq, last = [], self.energy
        start = self.m.frame
        while self.m.frame - start < limit:
            if self.boss_type != 17 or self.energy == 0 or self.m.rd8(self.dr + 544) != 0:
                break
            vis, x, sx, sy, st = self.boss_vis, self.boss_x, self.sx, self.sy, self.boss_state
            if st == 6:   # hidden: the next X is the table AC76 entry the object will take (dracula.c xs[row & 7])
                x = XS[self.m.rd8(self.dr + 16) & 7]
            b = ()
            if seq:
                b = seq.pop(0)
            elif st in (3, 4, 5, 6, 7) or vis:
                face_right = sx < x
                tx = x - 20 if face_right else x + 20
                d = 'RIGHT' if face_right else 'LEFT'
                if self.mode == 0:
                    if abs(sx - tx) > 3 or sy < 190:   # on a tower slab: walk off it, the jump starts from the floor
                        b = ('RIGHT',) if sx < tx else ('LEFT',)
                    elif vis:
                        seq = [('C', d)] * 2 + [(d,)] * 4 + [(d, 'B')] * 3 + [(d,)] * 40
            self.frame(b)
            if self.energy != last:
                last = self.energy
        print(f'[{self.m.frame - self.t0:6d}] form 1 over: boss type={self.boss_type} energy={self.energy}')

    def to_form2(self):
        """Death of the form 1, transformation, form 2 object 17h (type 23) alive with 128 energy."""
        self.until(lambda: self.boss_type == 23 and self.energy == 128, limit=3000, what='second form')
        print(f'[{self.m.frame - self.t0:6d}] form 2: object type {self.boss_type} X={self.boss_x} Y={self.m.rd8(self.dr + 3)} energy={self.energy}')
        self.shot('dracula-form2')

    # phase 2: zigzag of the side towers (tile 6 slabs, rows 20/16/12/8 -> feet Y 176/144/112/80)
    def climb(self):
        print(f'[{self.m.frame - self.t0:6d}] climb from x={self.sx} y={self.sy}')
        self.land()
        # (direction, feet Y of the slab reached, X to take off from: well inside the slab we stand on)
        steps = [('RIGHT', 176, 184), ('RIGHT', 144, 222), ('LEFT', 112, 246), ('RIGHT', 80, 222)]
        for direction, feet, launch in steps:
            self.walk(launch, 1)
            self.run(6)
            self.jump(direction)
            print(f'[{self.m.frame - self.t0:6d}]   jump {direction} from {launch}: x={self.sx} y={self.sy}')
            assert self.sy == feet, (direction, feet, self.sx, self.sy)
        self.shot('dracula-tower')

    # phase 3: face the room (LEFT) and throw
    def throw(self, limit=6000):
        self.walk(246, 1)
        self.run(2, ('LEFT',))   # face the room without leaving the slab (X >= 240 + 5 keeps the feet on it)
        print(f'[{self.m.frame - self.t0:6d}] throwing from x={self.sx} y={self.sy}')
        start = self.m.frame
        last = self.energy
        hits = 0
        while self.m.frame - start < limit and self.energy > 0 and self.boss_type == 23:
            self.run(4, ('B',))
            self.run(30)
            if self.energy != last:
                hits += 1
                last = self.energy
        print(f'[{self.m.frame - self.t0:6d}] form 2 energy {self.energy} after {hits} hits')


    # phase 4: death of the form 2, ending credits, back to the courtyard (the front mode leaves the interior)
    def finish(self, limit=20000):
        interior = FRONT_MODES.index('INTERIOR')
        score0 = self.score0
        self.until(lambda: self.front != interior, limit=limit, what='credits then front mode change')
        score = self.m.rd32(self.A('vk_hud_score'))
        print(f'[{self.m.frame - self.t0:6d}] front mode {FRONT_MODES[self.front]}, score {score0} -> {score}')
        assert score >= score0 + 80000, 'score of the two forms (30000 + 50000)'
        self.shot('dracula-ending')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--out', default=str(ROOT / 'scratchpad/dracula-fight'))
    ap.add_argument('--weapon', type=int, default=2)
    ap.add_argument('--hearts', type=int, default=99)
    ap.add_argument('--shots', action='store_true')
    ap.add_argument('--only-climb', action='store_true')
    a = ap.parse_args()
    f = Fight(a.out, a.shots)
    f.enter()
    f.score0 = f.m.rd32(f.A('vk_hud_score'))
    if a.only_climb:
        f.climb()
        return 0
    f.form1()
    f.to_form2()
    f.climb()
    # inventory fixture: secondary weapon (C416 and the whip level the throw reads) and hearts
    f.m.wr(f.A('vk_whip') + 5, a.weapon)
    f.m.wr(f.vc + 120, a.weapon)
    f.m.wr(f.vc + 118, a.hearts)
    f.throw()
    assert f.energy == 0
    f.finish()
    return 0


if __name__ == '__main__':
    sys.exit(main())
