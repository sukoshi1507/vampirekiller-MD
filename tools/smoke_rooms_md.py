#!/usr/bin/env python3
"""Crash smoke test on the Mega Drive build: every room of stages 1..18 is entered (room selector
of the boss 15 pool test: RAM entry + room freeze), then played for a few seconds with walking,
jumps, whip and stairs inputs. Fails when the game loop stops (C003/itemPhase no longer moves:
68000 exception screen, infinite loop) or the front mode leaves play other than by a game over.
Not a fidelity test: it only guards against crashes/hangs such as the GCC 16 odd-address loads."""
import argparse, json
from pathlib import Path
from probe import ROOT
from md_harness import Machine, Game_View, load_symbols, lookup, BUTTONS

INPUTS = ([('RIGHT',)] * 50 + [('RIGHT', 'C')] * 4 + [('RIGHT',)] * 20 + [('B',)] * 4 + [()] * 16 +
          [('LEFT',)] * 50 + [('LEFT', 'C')] * 4 + [('UP',)] * 30 + [('DOWN', 'B')] * 6 + [()] * 10 +
          [('DOWN',)] * 30 + [('C',)] * 4 + [()] * 12)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--core', required=True)
    ap.add_argument('--stages', default='1-18')
    ap.add_argument('--shots', action='store_true')
    ap.add_argument('--trace', action='store_true')
    ap.add_argument('--only', type=int, default=-1)
    ap.add_argument('--out', type=Path, default=Path('scratchpad/smoke-rooms'))
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    lo, hi = map(int, a.stages.split('-')) if '-' in a.stages else (int(a.stages),) * 2
    rooms = json.loads((ROOT / 'research/room-freeze-rooms.json').read_text())['rooms']
    syms = load_symbols(elf=ROOT / 'sgdk/out/rom.out')
    m = Machine(a.core, ROOT / 'sgdk/out/rom.bin')
    Game_View(m, syms)
    addr = lambda n: lookup(syms, n)[0]
    g, e, f, p = map(addr, ('vk_game', 'vk_enemies', 'room_freeze', 'vk_player'))
    mode = addr('vk_front_mode')

    def until(fn, n):
        for _ in range(n):
            if fn():
                return True
            m.run(1, set())
        return False

    def entry(stage, room):
        m.wr(g + 16, 1, 32); m.wr(g + 20, room); m.wr(g + 22, stage); m.wr(mode, 4, 16)
        for o, v in ((0, 16), (1, 192), (2, 0), (3, 0), (4, 0), (6, 0), (7, 0), (26, 0), (30, 0)):
            m.wr(g + o, v)
        for i in range(15):
            m.wr(p + i, m.rd8(g + i))
        m.wr(addr('health') + 1, 0); m.wr(addr('health') + 2, 0, 16); m.wr(addr('Enemies_god'), 1)
        m.wr(addr('freeze_drawn'), 0); m.wr(f, 4); m.wr(f + 1, 4); m.wr(f + 2, 0)
        m.wr(addr('freeze_vtimer'), m.rd32(addr('vtimer')), 32)
        return until(lambda: m.rd8(f) == 0 and m.rd8(addr('freeze_drawn')), 120)

    until(lambda: m.rd16(mode) == 1, 2000)
    m.run(4, {BUTTONS['START']}); m.run(4, set()); m.run(4, {BUTTONS['START']}); m.run(6, set())
    if not until(lambda: m.rd16(mode) == 3, 600):
        raise SystemExit('no courtyard after title')
    failures, done, slow = [], 0, []
    for r in rooms:
        s, room = r['stage'], r['room']
        if not lo <= s <= hi or (a.only >= 0 and room != a.only):
            continue
        if not entry(s, room):
            failures.append((s, room, 'entry: room freeze never ended'))
            m.image().save(a.out / f'fail-{s:02d}-{room:02d}.png')
            continue
        last, still, lost, worst = None, 0, 0, None
        where, settle, shop_before = None, 0, 0
        for keys in INPUTS:
            m.wr(addr('Enemies_god'), 1)
            m.run(1, {BUTTONS[k] for k in keys})
            if a.trace and m.frame % 6 == 0: print('  t', m.frame, keys, 'room', m.rd8(g + 20), 'x', m.rd8(g), 'y', m.rd8(g + 1), 'mode', m.rd8(g + 2), 'hp', m.rd8(addr('health')), m.rd8(addr('health') + 1))
            beat = (m.rd8(e + 158), m.rd8(e + 147))   # tick (C003) and itemPhase
            # A lost frame: normal play (alive, no freeze, pause, shop or map) but the loop did not end a pass.
            here = (m.rd8(g + 22), m.rd8(g + 20), m.rd8(addr('health') + 1), m.rd8(f))
            settle = 0 if here != where else settle + 1
            where = here
            if (settle > 40 and last is not None and beat[1] == last[1] and m.rd8(addr('health') + 1) == 0 and m.rd8(f) == 0
                    and not m.rd8(addr('paused')) and not m.rd8(addr('vk_walls') + 348) and not shop_before and m.rd16(mode) == 4):
                lost += 1; worst = worst or (m.frame, m.rd8(g + 20), m.rd8(g), m.rd8(g + 1))
                if a.trace: print('  lost', m.frame, beat, keys)
            shop_before = m.rd8(addr('vk_walls') + 348)   # the turn that closes the merchant leaves 0 with the world still frozen
            still = still + 1 if beat == last else 0
            last = beat
            if still > 60:
                break
        front = m.rd16(mode)
        if lost:
            slow.append((s, room, lost, worst))
        if a.shots:
            m.image().save(a.out / f'room-{s:02d}-{room:02d}.png')
            print(s, room, 'game', m.rd8(g + 22), m.rd8(g + 20), 'x', m.rd8(g), 'y', m.rd8(g + 1), 'front', front, 'frame', m.frame)
        if still > 60:
            failures.append((s, room, f'game loop stopped (front mode {front})'))
            m.image().save(a.out / f'fail-{s:02d}-{room:02d}.png')
            # A dead machine cannot enter the next room: restart from the title.
            m = Machine(a.core, ROOT / 'sgdk/out/rom.bin'); Game_View(m, syms)
            until(lambda: m.rd16(mode) == 1, 2000)
            m.run(4, {BUTTONS['START']}); m.run(4, set()); m.run(4, {BUTTONS['START']}); m.run(6, set())
            until(lambda: m.rd16(mode) == 3, 600)
        done += 1
    print(f'{done} rooms, {len(failures)} failures, {len(slow)} rooms with lost frames')
    for x in slow:
        print('SLOW stage %d room %d: %d lost frames, first at %s (frame, room, x, y)' % x)
    for x in failures:
        print('FAIL stage %d room %d: %s' % x)
    raise SystemExit(1 if failures or slow else 0)


if __name__ == '__main__':
    main()
