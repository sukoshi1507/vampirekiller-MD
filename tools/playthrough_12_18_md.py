#!/usr/bin/env python3
"""Continuous playthrough of the late stages (12..18) on the Mega Drive build: no restart of the game between rooms,
controller inputs only after the initial RAM entry of a segment.

Two modes:
  play    (default) replays tests/md_playthrough/playthrough_12_18.txt frame by frame: cold boot, RAM entry at the
          start of the stage, then the inputs. At each checkpoint it checks stage and room; at each frame the front
          mode and the game loop (C003 tick and itemPhase). It fails when a checkpoint is missed, when no checkpoint
          is reached for STALL_PROGRESS frames, when the front mode leaves play, or when the loop freezes.
          --video FILE.mp4 also writes the run (60 fps, H.264, yuv420p, x2 nearest neighbour; ffmpeg, frames piped).
  record  (--record) searches the inputs again: beam search over controller macros of 5 frames (12 pad combinations),
          the expansions spread over forked workers with their own emulator, branching on save states. A leg is a room
          exit (left/right/up/down), a stage 12 secret passage (crouch + UP at the BBCD point, walked to by search),
          the exit door (key + door), then the boss (walk to it and whip). Dead ends (an arrival from which the next
          leg has no path) are backtracked: other arrivals (several heights, lower then upper floor), other routes.

RAM help, all applied identically in record and play, none of it moves Simon:
  - Enemies_god = 1 at every frame: invincibility; the test checks transitions, not combat;
  - white key (items.keys |= 1, 'K') at the start of each stage: the key blocks are hidden (revealed by the whip on a
    candle) and are not what this test is about; Simon still has to reach the door and open it;
  - from stage 13: object 0D (high jump, 'J', see Game.high_jump): picked in the early stages of a real game, skipped
    here by the RAM entry; some ledges of the late stages are out of reach of the basic jump.
Replay file lines: 'B stage' (cold boot + RAM entry), 'H n KEYS' (n frames held), 'M n KEYS' (macro of the search:
n frames then a save/load round trip, see Game.roundtrip), 'K', 'J', 'C stage room' (checkpoint), 'E' (end of a
segment). Same ROM + same core = same frames."""
import argparse, ctypes as C, shutil, subprocess, sys, time, zlib
from pathlib import Path
from probe import ROOT
from md_harness import Machine, Game_View, load_symbols, lookup, BUTTONS

NAMES = {v: k for k, v in BUTTONS.items()}
REPLAY = ROOT / 'tests/md_playthrough/playthrough_12_18.txt'
DRACULA_FIGHT = False  # the stage 18 fight is not solved by this test (fight() has the attempt), see record()
STALL_BEAT = 150      # frames with C003 tick and itemPhase unchanged = frozen game loop
STALL_PROGRESS = 4500  # frames between two checkpoints
LINKS = {}            # (stage, room) -> (up, down, left, right)
ENTRIES = {12: (16, 176, 0), 13: (56, 176, 0), 14: (200, 176, 1), 15: (232, 176, 1), 16: (232, 176, 1), 17: (232, 80, 1), 18: (232, 176, 1)}   # kStageEntries (stage_exits.h)


def load_links():
    import re
    text = (ROOT / 'sgdk/src/generated/late_runtime.inc').read_text()
    block = text.split('late_links[7][12]={', 1)[1].split('};', 1)[0]
    stages = re.findall(r'\{((?:\{[^{}]*\},?)+)\}', block)
    for s, body in enumerate(stages):
        for r, t in enumerate(re.findall(r'\{([^{}]*)\}', body)):
            LINKS[(12 + s, r)] = tuple(int(v) for v in t.split(',')[:4])
    return text


def parse_exits():
    import re
    text = (ROOT / 'sgdk/src/generated/stage_exits.h').read_text()
    late = text.split('kLateRoomItems', 1)[1]
    rows = re.findall(r'\{(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\}', late)
    door = {}
    for i, r in enumerate(rows):
        door[12 + i // 12] = (int(r[4]), int(r[5]), int(r[6]))   # room, x, y of the door
    return door


class Game:
    def __init__(self, core, video=None):
        self.core = core
        self.syms = load_symbols(elf=ROOT / 'sgdk/out/rom.out')
        self.m = Machine(core, ROOT / 'sgdk/out/rom.bin')
        Game_View(self.m, self.syms)
        a = lambda n: lookup(self.syms, n)[0]
        self.a = a
        self.g, self.e, self.f, self.p = a('vk_game'), a('vk_enemies'), a('room_freeze'), a('vk_player')
        self.mode, self.god, self.items = a('vk_front_mode'), a('Enemies_god'), a('items')
        self.health, self.boss, self.sec = a('health'), a('vk_boss'), a('vk_secret')
        self.cand = a('vk_candles')
        self.rm = a('room_map')
        self.video = None
        self.log = []          # replay lines
        self.pending = None    # [buttons, frames] being run-length encoded
        self.nsize = self.m.lib.retro_serialize_size()
        if video:
            self.start_video(video)

    def reboot(self):
        """Cold start of the console (a new segment of the replay starts from a fresh boot, as when recorded)."""
        self.m.close()
        self.m = Machine(self.core, ROOT / 'sgdk/out/rom.bin')
        Game_View(self.m, self.syms)

    # ---- video -----------------------------------------------------------------------------
    def start_video(self, path):
        if not shutil.which('ffmpeg'):
            raise SystemExit('ffmpeg missing: --video needs it')
        self.m.run(1, set())
        w, h = self.m.image().size
        self.video = subprocess.Popen(
            ['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', f'{w}x{h}',
             '-r', '60', '-i', '-', '-vf', 'scale=iw*2:ih*2:flags=neighbor', '-c:v', 'libx264',
             '-pix_fmt', 'yuv420p', '-crf', '26', '-tune', 'animation', '-preset', 'fast', str(path)], stdin=subprocess.PIPE)

    def end_video(self):
        if self.video:
            self.video.stdin.close(); self.video.wait(); self.video = None

    def frame(self, buttons):
        self.m.run(1, buttons)
        if self.video:
            self.video.stdin.write(self.m.image().tobytes())

    # ---- state -----------------------------------------------------------------------------
    def rd(self, a): return self.m.rd8(a)
    @property
    def stage(self): return self.rd(self.g + 22)
    @property
    def room(self): return self.rd(self.g + 20)
    @property
    def px(self): return self.rd(self.g)
    @property
    def py(self): return self.rd(self.g + 1)
    @property
    def front(self): return self.m.rd16(self.mode)
    @property
    def frozen(self): return self.rd(self.f) != 0
    def beat(self): return (self.rd(self.e + 158), self.rd(self.e + 147))

    def save(self):
        buf = (C.c_uint8 * self.nsize)(); self.m.lib.retro_serialize(buf, self.nsize); return bytes(buf)

    def load(self, st):
        self.m.lib.retro_unserialize(st, self.nsize)

    def entry(self, stage, room, x=None, y=None, facing=None):
        """Initial entry by RAM (same as tools/smoke_rooms_md.py): the only teleport of the test."""
        m, a, g = self.m, self.a, self.g
        m.wr(g + 16, 1, 32); m.wr(g + 20, room); m.wr(g + 22, stage); m.wr(self.mode, 4, 16)
        ex, ey, ef = ENTRIES.get(stage, (16, 176, 0))
        for o, v in ((0, ex if x is None else x), (1, ey if y is None else y), (2, ef if facing is None else facing), (3, 0), (4, 0), (6, 0), (7, 0), (26, 0), (30, 0)):
            m.wr(g + o, v)
        for i in range(15):
            m.wr(self.p + i, m.rd8(g + i))
        m.wr(self.health + 1, 0); m.wr(self.health + 2, 0, 16); m.wr(self.god, 1)
        m.wr(a('freeze_drawn'), 0); m.wr(self.f, 4); m.wr(self.f + 1, 4); m.wr(self.f + 2, 0)
        m.wr(a('freeze_vtimer'), m.rd32(a('vtimer')), 32)
        for _ in range(120):
            if m.rd8(self.f) == 0 and m.rd8(a('freeze_drawn')):
                return True
            self.step(frozenset(), 1, log=False)
        return False

    def boot(self):
        m = self.m
        for _ in range(2000):
            if m.rd16(self.mode) == 1: break
            self.frame(frozenset())
        for keys, n in (({BUTTONS['START']}, 4), (set(), 4), ({BUTTONS['START']}, 4), (set(), 6)):
            for _ in range(n): self.frame(keys)
        for _ in range(600):
            if m.rd16(self.mode) == 3: return
            self.frame(frozenset())
        raise SystemExit('no courtyard after title')

    def step(self, buttons, n, log=True):
        """n frames with `buttons`; god mode rewritten each frame (see the module docstring)."""
        buttons = frozenset(buttons)
        if log:
            if self.pending and self.pending[0] == buttons: self.pending[1] += n
            else:
                self.flush(); self.pending = [buttons, n]
        for _ in range(n):
            self.m.wr(self.god, 1)
            self.frame(buttons)

    def roundtrip(self):
        """Save + load of the emulator. The search branches on save states, and the core keeps a few bytes outside its
        state (found: 5 bytes differ after a load), which is enough to make a long input sequence diverge. So every
        macro of the recorded path is followed by this same round trip, in the recording and in the replay."""
        self.load(self.save())

    def macro(self, buttons, n, log=True):
        if log:
            self.flush()
            self.log.append(f"M {n} {'+'.join(sorted(NAMES[b] for b in buttons)) or '-'}")
        self.step(buttons, n, log=False)
        self.roundtrip()

    def flush(self):
        if self.pending:
            keys = '+'.join(sorted(NAMES[b] for b in self.pending[0])) or '-'
            self.log.append(f'H {self.pending[1]} {keys}'); self.pending = None

    def give_key(self, jump=False):
        self.flush(); self.log.append('K')
        self.m.wr(self.items, self.m.rd8(self.items) | 1)
        if jump:
            self.log.append('J'); self.high_jump()

    def high_jump(self):
        # Object 0D (jump of 21 steps instead of 19, C431 bit 4 = vk_candles.c431, +170): picked up in the early stages
        # of a real game, which this test skips by entering at stage 12. Some ledges of the late stages are out of reach
        # of the basic jump by 1 or 2 pixels. Written again at each stage start (the stage change clears it).
        self.m.wr(self.cand + 170, self.m.rd8(self.cand + 170) | 0x10)

    def checkpoint(self):
        self.flush(); self.log.append(f'C {self.stage} {self.room}')


# ---- recording: beam search --------------------------------------------------------------------
ACTS = [frozenset(), {7}, {6}, {7, 8}, {6, 8}, {8}, {4}, {5}, {4, 7}, {4, 6}, {5, 7}, {5, 6}, {0}]
MACRO = 5
BEST = [1e9, 0, 0, 0, 0]


def key_of(G):
    m, g = G.m, G.g
    return (G.stage, G.room, G.px, G.py, m.rd8(g + 2), m.rd8(g + 3), m.rd8(g + 8), m.rd8(g + 9), m.rd8(g + 14),
            m.rd8(g + 26), tuple(m.rd8(G.sec + i) for i in range(8)), m.rd8(G.e + 18 * 7 + 1),
            zlib.crc32(bytes(m.ram[G.rm:G.rm + 768])))


_CTX = None
WORKERS = 7


def _expand(st):
    """In a forked worker: the 12 macros from a state; returns what the main process needs to rank them."""
    G, goal, h, valid = _CTX
    out = []
    for ai, act in enumerate(ACTS):
        G.load(st); G.step(act, MACRO, log=False)
        if goal(G): out.append((ai, 1, (G.px, G.py))); continue
        if not valid(G): out.append((ai, 0)); continue
        out.append((ai, 2, key_of(G), h(G), (G.px // 8, G.py // 16), (G.px, G.py, G.room)))
    return out


def _advance(args):
    st, ai = args
    G = _CTX[0]
    G.load(st); G.step(ACTS[ai], MACRO, log=False)
    return G.save()


def search(G, goal, h, valid, width=24, depth=260, want=10, extra=8):
    """Beam search (expansions spread over forked workers, each with its own emulator copy); returns the list of
    distinct goal hits [(path, arrival)] (several arrival heights, because the first one can be a dead end such as
    an alcove closed on the next room)."""
    import multiprocessing as mp
    global _CTX
    _CTX = (G, goal, h, valid)
    pool = mp.get_context('fork').Pool(WORKERS)
    try:
        beam = [(h(G), G.save(), ())]
        seen, hits, first = set(), [], None
        for d in range(depth):
            cands = []
            results = pool.map(_expand, [st for _, st, _ in beam])
            for (hv, st, path), res in zip(beam, results):
                for r in res:
                    ai = r[0]
                    if r[1] == 1:
                        arr = r[2]
                        if all(abs(arr[0] - a[0]) > 24 or abs(arr[1] - a[1]) > 12 for _, a in hits):
                            hits.append((path + (ai,), arr))
                            first = d if first is None else first
                    elif r[1] == 2:
                        if r[2] in seen: continue
                        seen.add(r[2])
                        cands.append((r[3] + 0.001 * len(path), path + (ai,), st, ai, r[4]))
                        if r[3] < BEST[0]: BEST[:] = [r[3], r[5][0], r[5][1], r[5][2], len(path)]
            if hits and (len(hits) >= want or d >= first + extra):
                return hits, d
            if not cands:
                return hits, d
            cands.sort(key=lambda c: c[0])
            chosen, per = [], {}
            for c in cands:
                if per.get(c[4], 0) >= 2: continue
                per[c[4]] = per.get(c[4], 0) + 1
                chosen.append(c)
                if len(chosen) >= width: break
            states = pool.map(_advance, [(c[2], c[3]) for c in chosen])
            beam = [(c[0], stt, c[1]) for c, stt in zip(chosen, states)]
        return hits, depth
    finally:
        pool.terminate()


# Stage 12 secret passages (research/secret-passages.md, table BBCD): (stage, source room) -> (destination, X, Y).
# Crouch (DOWN) within 16 px of the point then press UP: 64 frames later Simon is in the destination room.
SECRETS = {(12, 0): (3, 88, 168), (12, 1): (4, 184, 136), (12, 2): (11, 152, 168), (12, 3): (0, 88, 168),
           (12, 4): (1, 184, 136), (12, 5): (8, 88, 168), (12, 7): (10, 56, 168), (12, 8): (5, 88, 168),
           (12, 10): (7, 56, 168), (12, 11): (2, 152, 168)}


# Links the search proved to be dead ends (found by the recorder, kept so that it does not search them again):
# 12: 4 -> 5 by the right edge arrives at x 10 in a closed alcove of room 5 (floor y 144, wall to its right).
# 17: 10 -> 11 by the left edge arrives at the right of room 11, a bottomless shaft (falls to room 8): room 11 is
#     entered from room 8 by its up link, on the stairs.
AVOID = {(12, 4, 5), (17, 10, 11)}


def routes(stage, a, b, limit=9, cap=60):
    """Paths of the link graph from room a to room b, shortest first: [(next room, direction index), ...]. A room may
    be crossed twice (the same room is often entered at another height: stage 15 goes room 0 -> 1 -> 0 to climb)."""
    out = []

    def walk(r, path, used, visits):
        if r == b:
            out.append(list(path)); return
        if len(path) >= limit: return
        edges = list(enumerate(LINKS[(stage, r)]))
        if (stage, r) in SECRETS: edges.insert(0, (4, SECRETS[(stage, r)][0]))
        for di, t in edges:
            if t != 255 and (r, t, di) not in used and visits.get(t, 0) < 2 and (stage, r, t) not in AVOID:
                walk(t, path + [(t, di)], used | {(r, t, di)}, {**visits, t: visits.get(t, 0) + 1})
    walk(a, [], frozenset(), {a: 1})
    return sorted(out, key=len)[:cap]


def route(stage, a, b):
    """Shortest list of (room, direction index) from room a to room b in the link graph of the stage."""
    from collections import deque
    prev, q = {a: None}, deque([a])
    while q:
        r = q.popleft()
        if r == b: break
        edges = list(enumerate(LINKS[(stage, r)]))
        if (stage, r) in SECRETS: edges.insert(0, (4, SECRETS[(stage, r)][0]))
        for di, t in edges:
            if t != 255 and t not in prev:
                prev[t] = (r, di); q.append(t)
    out = []
    while prev.get(b):
        r, di = prev[b]; out.append((b, di)); b = r
    return out[::-1]   # [(next room, direction used from the room before)]


def alive(G):
    return G.front == 4 and G.rd(G.health + 1) == 0


def settled(G):
    return not G.frozen and G.front == 4 and G.rd(G.items + 2) == 0


def wait_settled(G, limit=400):
    for _ in range(limit):
        if settled(G): return True
        G.step(frozenset(), 1)
    return False


class Stop(Exception):
    pass


def leg_goal(G, s, leg, droom_xy):
    """(goal, heuristic, valid) of a leg ('dir', room, next)."""
    kind, cur, nxt = leg
    if kind == 'door':
        dx, dy = droom_xy
        return (lambda g: g.rd(g.items + 2) != 0,
                lambda g: abs(g.px - dx) + max(0, dy - g.py, g.py - (dy + 40)),
                lambda g: g.stage == s and g.room == cur and alive(g))
    if kind == 'secret':
        _, sx, sy = SECRETS[(s, cur)]
        return (lambda g: (g.px - sx) % 256 < 16 and (g.py - 4 - sy) % 256 < 16 and g.rd(g.g + 8) == 0,
                lambda g: abs(g.px - sx) + abs(g.py - 2 - sy - 8),
                lambda g: g.stage == s and g.room == cur and alive(g))
    h = {'up': lambda g: g.py, 'down': lambda g: 255 - g.py, 'left': lambda g: g.px, 'right': lambda g: 255 - g.px}[kind]
    return (lambda g: g.stage == s and g.room == nxt, h, lambda g: g.stage == s and g.room in (cur, nxt) and alive(g))


def apply_leg(G, s, leg, path):
    kind, cur, nxt = leg
    for ai in path: G.macro(ACTS[ai], MACRO)
    if kind == 'secret':
        G.step(frozenset({5}), 10); G.step(frozenset({4}), 2)   # crouch, then UP (new press) in the zone
        for _ in range(200):
            if G.room == nxt: break
            G.step(frozenset(), 1)
        if G.room != nxt: return False
    if kind == 'door': return True
    if kind != 'secret' and (G.stage, G.room) != (s, nxt):
        print(f'    replay of the found path ends in stage {G.stage} room {G.room} x {G.px} y {G.py} instead of room {nxt}', flush=True)
        return False
    if not wait_settled(G): return False
    G.checkpoint()
    return True


FAILED = set()


def run_legs(G, s, legs, i, droom_xy, width, shots, depth=0):
    """Depth-first over the candidate arrivals of each leg; restores the emulator and the log when a leg dead-ends.
    Rooms have an upper and a lower floor and a side exit can be left on either; the next room often goes on from one
    of them only. So a side exit is searched for the lower floor first and, when none of its arrivals leads on, for the
    upper floor (arrival at y <= 100)."""
    if i == len(legs): return True
    leg = legs[i]
    st0 = G.save(); G.flush(); logn = len(G.log)
    tag = f's{s}-{leg[0]}-r{leg[1]}to{leg[2]}'
    fkey = (tuple(legs[i:]), zlib.crc32(st0))
    if fkey in FAILED:      # same leg from the very same emulator state (a route sharing a failed prefix)
        return False
    goal, h, valid = leg_goal(G, s, leg, droom_xy)
    tried = []
    for tier in (('low', 'high') if leg[0] in ('left', 'right') else (None,)):
        if tier:
            goal_t = lambda g, goal=goal, tier=tier: goal(g) and ((g.py <= 100) == (tier == 'high'))
            h_t = (lambda g, h=h: h(g) + 1.5 * max(0, g.py - 80)) if tier == 'high' else h
        else:
            goal_t, h_t = goal, h
        t = time.time(); BEST[:] = [1e9, 0, 0, 0, 0]
        G.load(st0)
        hits, d = search(G, goal_t, h_t, valid, width=width)
        print('  ' * depth + f'{tag} [{tier}]: {len(hits)} candidates, search {time.time() - t:.0f}s, best {BEST}', flush=True)
        if not hits and time.time() - t < 3:
            print('  ' * depth + f'  root state: stage {G.stage} room {G.room} x {G.px} y {G.py} front {G.front} health {[G.rd(G.health + k) for k in range(3)]} '
                  f'frozen {G.rd(G.f)} door {G.rd(G.items + 2)} player {[G.rd(G.g + k) for k in range(15)]}', flush=True)
        for path, arr in hits:
            G.load(st0); G.pending = None; del G.log[logn:]
            if apply_leg(G, s, leg, path) and run_legs(G, s, legs, i + 1, droom_xy, width, shots, depth + 1):
                return True
            print('  ' * depth + f'  {tag}: arrival {arr} leads nowhere, next candidate', flush=True)
    G.load(st0); G.pending = None; del G.log[logn:]
    G.m.image().save(shots / f'fail-{tag}.png')
    FAILED.add(fkey)
    return False


def fight(G, s, shots, limit=6000):
    """Boss room: walk toward the boss, turn to it and whip it (B tapped every 12 frames), controller only; the
    invincibility (Enemies_god) holds. The boss's orb is picked by walking where the boss was.
    Stage 18 (Dracula, a big face in the middle of the room): walk left up to the face, whip it (and jump now and then
    for its upper boxes); the end is the credits, i.e. the front mode leaving play ('X' then 'F n' in the replay)."""
    last = s == 18
    if last:
        G.flush(); G.log.append('X')
        limit = 24000
        dr = lookup(G.syms, 'dracula')[0]
    for i in range(limit):
        if last:
            if G.front != 4:
                G.flush(); G.log.append(f'F {G.front}')
                return True
        elif G.stage != s: return True
        if last:
            keys = set()
            if G.px > 200: keys.add(6)
            elif i % 14 == 0: keys.add(6)           # turn to the face
            elif i % 14 in (1, 2): keys.add(0)      # whip
            if i % 90 == 60: keys.add(8)            # jump
            G.step(frozenset(keys), 1)
            if i % 300 == 0:
                print(f'    dracula fight {i}: simon {G.px},{G.py} dracula energy {G.rd(dr + 608)} front {G.front}', flush=True)
            continue
        bx = G.m.rd16(G.boss + 10) >> 8
        if G.rd(G.boss + 21) == 0: bx = 0x78         # dead boss: the orb (vk_boss_orb_touch, X 78h) is where to walk, not where the boss fell
        d = bx - G.px
        toward = 7 if d > 0 else 6
        keys = set()
        perched = G.py < 192                        # on a block of the room: the whip passes over the boss
        if abs(d) > 30 or perched: keys.add(toward) # walk off the block toward the boss
        elif i % 12 == 0: keys.add(toward)          # turn to the boss
        elif i % 12 in (1, 2): keys.add(0)          # whip (B pressed)
        if i % 150 == 100 and not perched: keys.add(8)  # a jump now and then
        G.step(frozenset(keys), 1)
        if i % 300 == 0:
            print(f'    fight {i}: simon {G.px},{G.py} boss x {bx} y {G.m.rd16(G.boss + 8) >> 8} in_room {G.rd(G.boss)} '
                  f'energy {G.rd(G.boss + 21)}', flush=True)
    G.m.image().save(shots / 'boss-fail.png')
    return False


def record(G, lo, hi, width, shots):
    lo = max(12, lo)
    door = parse_exits()
    G.boot()
    assert G.entry(lo, 0)
    G.flush(); G.log.append(f'B {lo}')   # boot + RAM entry marker (replayed by the code, not by inputs)
    G.step(frozenset(), 30)
    stage = lo
    while stage <= hi:
        G.give_key(jump=stage >= 13); G.checkpoint()
        droom, dx, dy = door[stage]
        s = stage
        st_stage = G.save(); G.flush(); logn = len(G.log)
        ok = False
        for rt in routes(s, G.room, droom):
            legs, cur = [], G.room
            for nxt, di in rt:
                legs.append((('up', 'down', 'left', 'right', 'secret')[di], cur, nxt)); cur = nxt
            legs.append(('door', droom, droom))
            print(f'stage {s} route {[l[2] for l in legs[:-1]]}', flush=True)
            if run_legs(G, s, legs, 0, (dx, dy), width, shots):
                ok = True; break
            G.load(st_stage); G.pending = None; del G.log[logn:]
        if not ok:
            raise SystemExit(f'RECORD STOP stage {s}: no route found (see fail-*.png)')
        for _ in range(600):
            if G.stage != s or G.room != droom: break
            G.step(frozenset(), 1)
        for _ in range(400):
            if settled(G): break
            G.step(frozenset(), 1)
        if G.stage == s:
            print(f'stage {s}: boss room {G.room}, fight', flush=True)
            G.checkpoint()
            if s == 18 and not DRACULA_FIGHT:
                # Dracula's room is reached by the inputs; the whip does not reach the face from where Simon can stand
                # (see the report): the replay stops here, in the room, after a few seconds of the intro.
                for _ in range(300): G.step(frozenset(), 1)
                break
            if not fight(G, s, shots):
                raise SystemExit(f'RECORD STOP boss stage {s} (room {G.room}): boss not beaten (see boss-fail.png)')
            if s == 18: break
            for _ in range(400):
                if settled(G): break
                G.step(frozenset(), 1)
        stage = G.stage
    if G.front == 4: G.checkpoint()
    G.flush(); G.log.append('E')

def play(G, replay, shots):
    """Replays the recorded inputs and checks checkpoints, front mode and game loop."""
    lines = [l.strip() for l in replay.read_text().splitlines() if l.strip() and not l.startswith('#')]
    last_beat, still, since_cp, cps, started, booted, ending = None, 0, 0, 0, time.time(), False, False

    def fail(msg):
        G.m.image().save(shots / 'playthrough-fail.png')
        G.end_video()
        raise SystemExit(f'FAIL frame {G.m.frame} stage {G.stage} room {G.room} x {G.px} y {G.py} front {G.front}: {msg}')

    for ln in lines:
        w = ln.split()
        if w[0] == 'B':
            if booted: G.reboot()
            booted = True
            G.boot()
            if not G.entry(int(w[1]) if len(w) > 1 else 12, 0): fail('entry: room freeze never ended')
        elif w[0] == 'K':
            G.m.wr(G.items, G.m.rd8(G.items) | 1)
        elif w[0] == 'J':
            G.high_jump()
        elif w[0] == 'X':
            ending = True           # the last boss: the credits follow, the front mode and the loop checks stop
        elif w[0] == 'F':
            if G.front != int(w[1]): fail(f'expected front mode {w[1]} after the last boss')
            print(f'  last boss beaten: front mode {G.front} (credits done) at frame {G.m.frame}', flush=True)
        elif w[0] == 'C':
            if (G.stage, G.room) != (int(w[1]), int(w[2])):
                fail(f'checkpoint missed: expected stage {w[1]} room {w[2]}')
            cps += 1; since_cp = 0
            print(f'  stage {G.stage} room {G.room} reached at frame {G.m.frame}', flush=True)
        elif w[0] in ('H', 'M'):
            keys = frozenset() if w[2] == '-' else frozenset(BUTTONS[k] for k in w[2].split('+'))
            for _ in range(int(w[1])):
                G.step(keys, 1, log=False)
                since_cp += 1
                beat = G.beat()
                still = still + 1 if beat == last_beat else 0
                last_beat = beat
                if ending:
                    if since_cp > 4 * STALL_PROGRESS: fail('last boss: no end after too many frames')
                    if G.front != 4: break
                    continue
                if still > STALL_BEAT: fail('game loop frozen (C003 tick and itemPhase unchanged)')
                if since_cp > STALL_PROGRESS: fail('no progress between two checkpoints')
                if G.front != 4: fail('unexpected front mode')
            if w[0] == 'M': G.roundtrip()
        elif w[0] == 'E':
            pass                    # end of a segment; the next 'B' boots again
    print(f'playthrough OK: {cps} checkpoints, {G.m.frame} frames, {time.time() - started:.0f} s')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--core', required=True)
    ap.add_argument('--record', action='store_true')
    ap.add_argument('--replay', type=Path, default=REPLAY)
    ap.add_argument('--video', type=Path)
    ap.add_argument('--out', type=Path, default=Path('scratchpad/playthrough'))
    ap.add_argument('--width', type=int, default=24)
    ap.add_argument('--last', type=int, default=18, help='record: last stage to play (the replay ends on entering the next one)')
    ap.add_argument('--start', type=int, default=12, help='record: first stage (RAM entry), to probe a later stage alone')
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    load_links()
    G = Game(a.core, a.video)
    if a.record:
        try:
            record(G, a.start, a.last, a.width, a.out)
        finally:
            G.flush()
            a.replay.parent.mkdir(parents=True, exist_ok=True)
            a.replay.write_text('\n'.join(G.log) + '\n')
        print('recorded', a.replay)
        return
    try:
        play(G, a.replay, a.out)
    finally:
        G.end_video()


if __name__ == '__main__':
    main()
