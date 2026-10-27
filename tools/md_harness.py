#!/usr/bin/env python3
"""Harnais Mega Drive sans affichage : Genesis Plus GX (libretro) + ROM SGDK.

Charge sgdk/out/rom.bin dans le core libretro Genesis Plus GX via ctypes (aucune
fenetre, aucun son), rejoue un script d'entrees, lit la RAM 68000 par les symboles
de l'ELF (m68k-elf-nm -S sur sgdk/out/release/rom.out, sinon symbol.txt), ecrit les
lectures en JSON (et CSV) et des captures PNG. Ne modifie ni la ROM ni le depot.

Usage :
  python3 tools/md_harness.py SCRIPT.txt [--out DIR] [--rom ROM] [--elf ELF]
         [--symbols symbol.txt] [--core CORE.so] [--dump-at 100,200] [--shot-at 300]
         [--trace N] [--quiet]
  python3 tools/md_harness.py --symbols-only        # liste les variables RAM du jeu

Exemple : python3 tools/md_harness.py tests/md_scenarios/a_courtyard_walk.txt --out scratchpad/md-a
Code de sortie : 0 si toutes les assertions `expect` passent, 1 sinon, 2 sur erreur.

Script (une commande par ligne, `#` = commentaire ; "image" = un appel a retro_run) :
  wait N                 N images sans bouton
  hold BTNS N            BTNS tenus N images, puis relaches (BTNS = START, A+B, RIGHT+C, -)
  tap BTNS [N]           BTNS N images (defaut 4) puis N images relachees : un seul front
                         montant, assez long pour une boucle de jeu occupee (chargement)
  @F BTNS                avance (boutons courants) jusqu'a l'image absolue F, puis tient BTNS
                         (format "image -> boutons" ; `@F -` relache tout)
  until EXPR [max N] [BTNS]   tient BTNS (defaut : rien) jusqu'a EXPR vraie ; echec apres N (600)
  expect EXPR [message]  assertion sur l'etat courant (compte dans le resultat)
  dump [LABEL]           lecture RAM -> dumps.json / dumps.csv
  shot [NOM]             capture -> NOM.png (defaut : frame-<image>.png)
  poke SYMBOLE[+OFF] VALEUR [8|16|32]   ecrit la RAM (defaut 8 bits)
  set NOM EXPR           memorise une valeur (utilisable ensuite dans les EXPR)
  echo EXPR              affiche une valeur
  include FICHIER        execute un autre script (chemin relatif au script courant)
Les EXPR sont du Python evalue sur l'etat : frame, front_mode, game.area, game.room,
game.player.x/.y/.facing/.mode/.crouched/.on_stairs..., player (copie affichee),
whip.timer/.facing/.x/.y, enemies.energy/.invuln/.recoil/.alive (nb actifs),
enemies.enemy[i].active/.type/.x/.y/.dx/.age, target (test_target), flames[0..1],
health.lives/.phase/.timer (C410 ; phase 0 vivant, 1 mort, 2 reapparition, 3 fin de partie),
ram8/ram16/ram32(addr|'symbole'), et les noms memorises par `set`.
Boutons : A B C START UP DOWN LEFT RIGHT (manette 3 boutons, port 1).
Correspondance libretro (Genesis Plus GX) : B=0 (MD B), Y=1 (MD A), START=3,
UP=4, DOWN=5, LEFT=6, RIGHT=7, A=8 (MD C).

Les structures (VKPlayer, VKGame, VKWhip, EnemyState, VKTarget) sont decodees selon la
disposition m68k-elf-gcc ; la taille de chaque symbole est verifiee a l'execution
(un changement de structure dans sgdk/src provoque une erreur explicite).
"""
import argparse
import csv
import ctypes as C
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CORE = Path('/usr/lib/libretro/genesis_plus_gx_libretro.so')
SYSTEM_RAM = 2

BUTTONS = {'B': 0, 'A': 1, 'START': 3, 'UP': 4, 'DOWN': 5, 'LEFT': 6, 'RIGHT': 7, 'C': 8}
FRONT_MODES = ['LOGO', 'TITLE', 'CASTLE', 'COURTYARD', 'INTERIOR', 'VIEWER', 'TEST', 'SOUND']

# Tailles attendues (octets) des globales decodees : garde-fou contre une derive des structures.
EXPECTED_SIZES = {'player': 14, 'game': 20, 'whip': 6, 'enemy_state': 56,
                  'test_target': 18, 'courtyard_flame': 36, 'front_mode': 2, 'health': 4}
# Noms C actuels des globales partagees (sgdk/src/frontend.h, actors.h) ; les scenarios
# gardent les noms courts. test_target et courtyard_flame sont statiques (room_view.c).
SYMBOL_ALIASES = {'game': 'vk_game', 'player': 'vk_player', 'whip': 'vk_whip',
                  'enemy_state': 'vk_enemies', 'front_mode': 'vk_front_mode'}


class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                ('size', C.c_size_t), ('meta', C.c_char_p)]


def parse_buttons(text):
    if text in ('-', '', 'NONE'):
        return frozenset()
    out = set()
    for name in text.upper().split('+'):
        if name not in BUTTONS:
            raise ValueError(f'bouton inconnu: {name} (connus: {", ".join(BUTTONS)})')
        out.add(BUTTONS[name])
    return frozenset(out)


def load_symbols(elf=None, symbol_txt=None):
    """{nom: (offset RAM, taille ou None)} pour les symboles en RAM 68k (0xFF0000)."""
    table = {}
    if elf and Path(elf).exists() and shutil.which('m68k-elf-nm'):
        text = subprocess.run(['m68k-elf-nm', '-S', '-n', str(elf)], check=True,
                              capture_output=True, text=True).stdout
        for line in text.splitlines():
            parts = line.split()
            if len(parts) == 4 and parts[2] in 'bBdD':
                address, size = int(parts[0], 16), int(parts[1], 16)
                if (address & 0xFF0000) == 0xFF0000:
                    table[parts[3]] = (address & 0xFFFF, size)
        if table:
            return table
    if symbol_txt and Path(symbol_txt).exists():
        rows = []
        for match in re.finditer(r'^([0-9a-fA-F]{8})\s+([bBdD])\s+(\S+)', Path(symbol_txt).read_text(), re.M):
            address = int(match[1], 16)
            if (address & 0xFF0000) == 0xFF0000:
                rows.append((address & 0xFFFF, match[3]))
        rows.sort()
        for i, (address, name) in enumerate(rows):
            nxt = rows[i + 1][0] if i + 1 < len(rows) else None
            table[name] = (address, (nxt - address) if nxt is not None else None)
        return table
    raise FileNotFoundError('ni ELF (rom.out) ni symbol.txt : construire la ROM (make)')


def lookup(symbols, name):
    if name in symbols:
        return symbols[name]
    if SYMBOL_ALIASES.get(name) in symbols:
        return symbols[SYMBOL_ALIASES[name]]
    # LTO renomme parfois les statiques (nom.lto_priv.N) : un seul candidat accepte.
    hits = [k for k in symbols if k.split('.')[0] == name]
    if len(hits) == 1:
        return symbols[hits[0]]
    raise KeyError(f'symbole RAM absent ou ambigu: {name} {hits}')


class Machine:
    """Core libretro charge par ctypes : images, boutons, RAM 68k (lecture/ecriture), capture."""

    def __init__(self, core, rom):
        self.lib = lib = C.CDLL(str(Path(core).resolve()))
        self.format = 2
        self.buttons = frozenset()
        self.frame_buffer = None
        self.frame = 0
        self._dir = C.create_string_buffer(str(Path(rom).resolve().parent).encode())
        Env = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
        Video = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
        Audio = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
        Batch = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
        Poll = C.CFUNCTYPE(None)
        Input = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)

        def environment(command, data):
            if command == 10:  # SET_PIXEL_FORMAT
                self.format = C.cast(data, C.POINTER(C.c_int))[0]
                return self.format in (0, 1, 2)
            if command in (9, 31):  # SYSTEM / SAVE directory
                C.cast(data, C.POINTER(C.c_void_p))[0] = C.addressof(self._dir)
                return True
            if command == 3:  # GET_CAN_DUPE
                C.cast(data, C.POINTER(C.c_bool))[0] = True
                return True
            if command == 39:
                C.cast(data, C.POINTER(C.c_uint))[0] = 0
                return True
            return command in (11, 16, 18, 35, 37)

        def video(data, width, height, pitch):
            if data and data != C.c_void_p(-1).value:
                self.frame_buffer = (C.string_at(data, pitch * height), width, height, pitch)

        self._callbacks = [Env(environment), Video(video), Audio(lambda l, r: None),
                           Batch(lambda d, n: n), Poll(lambda: None),
                           Input(lambda port, dev, idx, btn: int(port == 0 and btn in self.buttons))]
        for name, cb in zip(('environment', 'video_refresh', 'audio_sample', 'audio_sample_batch',
                             'input_poll', 'input_state'), self._callbacks):
            getattr(lib, 'retro_set_' + name)(cb)
        lib.retro_load_game.argtypes = [C.POINTER(Game)]
        lib.retro_load_game.restype = C.c_bool
        lib.retro_get_memory_data.argtypes = [C.c_uint]
        lib.retro_get_memory_data.restype = C.c_void_p
        lib.retro_get_memory_size.argtypes = [C.c_uint]
        lib.retro_get_memory_size.restype = C.c_size_t
        lib.retro_init()
        raw = Path(rom).read_bytes()
        self._rom = C.create_string_buffer(raw)
        game = Game(str(Path(rom).resolve()).encode(), C.cast(self._rom, C.c_void_p), len(raw), None)
        if not lib.retro_load_game(C.byref(game)):
            raise RuntimeError('core libretro : chargement de la ROM refuse')
        lib.retro_set_controller_port_device(0, 1)  # RETRO_DEVICE_JOYPAD
        pointer, size = lib.retro_get_memory_data(SYSTEM_RAM), lib.retro_get_memory_size(SYSTEM_RAM)
        if not pointer or size != 0x10000:
            raise RuntimeError('RAM 68k (64 Ko) non exposee par le core')
        self.ram = (C.c_uint8 * size).from_address(pointer)

    def close(self):
        self.lib.retro_unload_game()
        self.lib.retro_deinit()

    def run(self, n=1, buttons=None):
        if buttons is not None:
            self.buttons = buttons
        for _ in range(n):
            self.lib.retro_run()
            self.frame += 1

    # Genesis Plus GX range la RAM 68k mot par mot en ordre hote : octet a l'index a ^ 1.
    def rd8(self, a):
        return self.ram[(a & 0xFFFF) ^ 1]

    def rd16(self, a):
        return (self.rd8(a) << 8) | self.rd8(a + 1)

    def rd32(self, a):
        return (self.rd16(a) << 16) | self.rd16(a + 2)

    def wr(self, a, value, bits=8):
        for i in range(bits // 8):
            self.ram[((a + i) & 0xFFFF) ^ 1] = (value >> (bits - 8 - 8 * i)) & 0xFF

    def image(self):
        from PIL import Image
        if self.frame_buffer is None:
            raise RuntimeError('aucune image video recue')
        raw, width, height, pitch = self.frame_buffer
        if self.format == 1:  # XRGB8888
            return Image.frombytes('RGB', (width, height), raw, 'raw', 'BGRX', pitch)
        import numpy as np
        words = np.frombuffer(raw, dtype='<u2').reshape(height, pitch // 2)[:, :width].astype(np.uint32)
        if self.format == 2:  # RGB565
            r, g, b = (words >> 11) & 31, (words >> 5) & 63, words & 31
            rgb = np.stack([r * 255 // 31, g * 255 // 63, b * 255 // 31], axis=-1)
        else:  # 0RGB1555
            r, g, b = (words >> 10) & 31, (words >> 5) & 31, words & 31
            rgb = np.stack([r * 255 // 31, g * 255 // 31, b * 255 // 31], axis=-1)
        return Image.fromarray(rgb.astype(np.uint8), 'RGB')


def s8(v):
    return v - 256 if v >= 128 else v


def s16(v):
    return v - 65536 if v >= 32768 else v


def s32(v):
    return v - (1 << 32) if v >= 1 << 31 else v


class Game_View:
    """Decodage des globales du jeu (dispositions m68k-elf-gcc, alignement 2)."""

    def __init__(self, machine, symbols):
        self.m = machine
        self.symbols = symbols
        self.addr = {}
        for name, expected in EXPECTED_SIZES.items():
            try:
                address, size = lookup(symbols, name)
            except KeyError:
                continue
            if size is not None and size != expected:
                raise RuntimeError(f'{name}: taille {size} au lieu de {expected} octets : '
                                   'mettre a jour le decodage dans tools/md_harness.py')
            self.addr[name] = address

    def player_at(self, a):
        m = self.m
        keys = ('x', 'y', 'facing', 'mode', 'phase', 'clock', 'pose', 'crouched', 'on_stairs',
                'stair_anim', 'stair_dir', 'stair_len', 'stair_glide', 'jump_frames')
        return {k: m.rd8(a + i) for i, k in enumerate(keys)}

    def target_at(self, a):
        m = self.m
        return {'x': s32(m.rd32(a)), 'y': s32(m.rd32(a + 4)), 'width': s32(m.rd32(a + 8)),
                'height': s32(m.rd32(a + 12)), 'active': m.rd8(a + 16), 'hit_latched': m.rd8(a + 17)}

    def snapshot(self):
        m, A = self.m, self.addr
        state = {'frame': m.frame, 'buttons': sorted(k for k, v in BUTTONS.items() if v in m.buttons)}
        if 'front_mode' in A:
            mode = m.rd16(A['front_mode'])
            state['front_mode'] = mode
            state['front_mode_name'] = FRONT_MODES[mode] if mode < len(FRONT_MODES) else '?'
        if 'game' in A:
            a = A['game']
            state['game'] = {'player': self.player_at(a), 'area': m.rd32(a + 14),
                             'room': m.rd8(a + 18), 'collision_verified': m.rd8(a + 19)}
        if 'player' in A:
            state['player'] = self.player_at(A['player'])
        if 'whip' in A:
            a = A['whip']
            state['whip'] = dict(zip(('timer', 'facing', 'x', 'y', 'crouched', 'level'),
                                     (m.rd8(a + i) for i in range(6))))
        if 'enemy_state' in A:
            a = A['enemy_state']
            enemies = []
            for i in range(4):
                e = a + 12 * i
                enemies.append({'active': m.rd8(e), 'exiting': m.rd8(e + 1), 'type': m.rd8(e + 2),
                                'extraPhase': m.rd8(e + 3), 'x': s16(m.rd16(e + 4)), 'y': m.rd8(e + 6),
                                'dx': s8(m.rd8(e + 7)), 'sub': m.rd8(e + 8), 'age': m.rd16(e + 10)})
            b = a + 48
            state['enemies'] = {'enemy': enemies, 'alive': sum(1 for e in enemies if e['active']),
                                'energy': m.rd8(b), 'invuln': m.rd8(b + 1), 'invulnPhase': m.rd8(b + 2),
                                'recoil': m.rd8(b + 3), 'recoilDir': s8(m.rd8(b + 4)),
                                'parity': m.rd8(b + 5), 'type4Frames': m.rd8(b + 6),
                                'type4Side': m.rd8(b + 7)}
        if 'health' in A:
            a = A['health']   # VKHealth (health.h), statique de main.c
            state['health'] = {'lives': m.rd8(a), 'phase': m.rd8(a + 1), 'timer': m.rd16(a + 2)}
        if 'test_target' in A:
            state['target'] = self.target_at(A['test_target'])
        if 'courtyard_flame' in A:
            state['flames'] = [self.target_at(A['courtyard_flame'] + 18 * k) for k in range(2)]
        return state


def ns(value):
    if isinstance(value, dict):
        return SimpleNamespace(**{k: ns(v) for k, v in value.items()})
    if isinstance(value, list):
        return [ns(v) for v in value]
    return value


def flatten(prefix, value, out):
    if isinstance(value, dict):
        for k, v in value.items():
            flatten(f'{prefix}.{k}' if prefix else k, v, out)
    elif isinstance(value, list) and value and isinstance(value[0], dict):
        for i, v in enumerate(value):
            flatten(f'{prefix}[{i}]', v, out)
    else:
        out[prefix] = '+'.join(value) if isinstance(value, list) else value
    return out


class Harness:
    def __init__(self, core, rom, symbols, out, quiet=False):
        self.machine = Machine(core, rom)
        self.symbols = symbols
        self.view = Game_View(self.machine, symbols)
        self.out = Path(out)
        self.out.mkdir(parents=True, exist_ok=True)
        self.dumps, self.checks, self.vars = [], [], {}
        self.quiet = quiet
        self.trace_every = 0
        self.trace_rows = []

    def log(self, *args):
        if not self.quiet:
            print(*args)

    def state(self):
        return self.view.snapshot()

    def env(self):
        m = self.machine

        def addr(x):
            # 'symbole' ou 'symbole+OFF' (OFF decimal ou 0x..), sinon une adresse.
            if not isinstance(x, str):
                return x
            name, _, off = x.partition('+')
            return lookup(self.symbols, name)[0] + (int(off, 0) if off else 0)
        env = {k: ns(v) for k, v in self.state().items()}
        env.update(ram8=lambda a: m.rd8(addr(a)), ram16=lambda a: m.rd16(addr(a)),
                   ram32=lambda a: m.rd32(addr(a)), abs=abs, min=min, max=max, any=any, all=all,
                   len=len, enumerate=enumerate, range=range, sorted=sorted, MODES={n: i for i, n in enumerate(FRONT_MODES)})
        env.update(self.vars)
        return env

    def eval(self, expr):
        # L'environnement sert de globales : les comprehensions y voient game, enemies...
        env = self.env()
        env['__builtins__'] = {}
        return eval(expr, env)

    def step(self, n, buttons=None):
        for _ in range(n):
            self.machine.run(1, buttons)
            buttons = None
            if self.trace_every and self.machine.frame % self.trace_every == 0:
                self.trace_rows.append(flatten('', self.state(), {}))

    def dump(self, label=None):
        snap = self.state()
        snap['label'] = label or f'frame-{self.machine.frame}'
        self.dumps.append(snap)
        self.log(f"[{self.machine.frame:6d}] dump {snap['label']}: "
                 f"mode={snap.get('front_mode_name')} area={snap['game']['area']} room={snap['game']['room']} "
                 f"x={snap['game']['player']['x']} y={snap['game']['player']['y']} "
                 f"energy={snap['enemies']['energy']} alive={snap['enemies']['alive']}"
                 if 'game' in snap and 'enemies' in snap else f'[{self.machine.frame}] dump {snap["label"]}')
        return snap

    def shot(self, name=None):
        name = name or f'frame-{self.machine.frame:06d}'
        path = self.out / (name if name.endswith('.png') else name + '.png')
        self.machine.image().save(path)
        self.log(f'[{self.machine.frame:6d}] shot {path}')
        return path

    def run_line(self, line, number):
        words = line.split()
        cmd, args = words[0].lower(), words[1:]
        m = self.machine
        if cmd.startswith('@'):
            target = int(cmd[1:])
            if target < m.frame:
                raise ValueError(f'ligne {number}: image {target} deja passee ({m.frame})')
            self.step(target - m.frame)
            m.buttons = parse_buttons(args[0] if args else '-')
        elif cmd == 'wait':
            self.step(int(args[0]), frozenset())
        elif cmd == 'hold':
            self.step(int(args[1]), parse_buttons(args[0]))
            m.buttons = frozenset()
        elif cmd == 'tap':
            # Un seul front montant ; 4 images tenues pour ne pas tomber dans une image
            # ou la boucle du jeu ne lit pas la manette (chargement de scene, VDP coupe).
            n = int(args[1]) if len(args) > 1 else 4
            self.step(n, parse_buttons(args[0]))
            self.step(n, frozenset())
        elif cmd == 'until':
            rest = line.split(None, 1)[1]
            limit, held = 600, frozenset()
            match = re.match(r'(.*?)\s+max\s+(\d+)(?:\s+(\S+))?\s*$', rest)
            if match:
                rest, limit = match[1], int(match[2])
                held = parse_buttons(match[3]) if match[3] else frozenset()
            m.buttons = held
            for _ in range(limit + 1):
                if self.eval(rest):
                    break
                self.step(1)
            else:
                m.buttons = frozenset()
                raise TimeoutError(f'ligne {number}: `{rest}` toujours faux apres {limit} images')
            m.buttons = frozenset()
            self.log(f'[{m.frame:6d}] until {rest}: ok')
        elif cmd == 'expect':
            rest = line.split(None, 1)[1]
            expr, _, message = rest.partition(' # ')
            try:
                ok = bool(self.eval(expr))
                error = None
            except Exception as exc:  # expression invalide : echec explicite
                ok, error = False, repr(exc)
            self.checks.append({'line': number, 'frame': m.frame, 'expr': expr.strip(),
                                'message': message.strip(), 'ok': ok, 'error': error})
            print(f"[{m.frame:6d}] {'OK  ' if ok else 'FAIL'} {expr.strip()}"
                  + (f'  ({message.strip()})' if message else '') + (f'  {error}' if error else ''))
        elif cmd == 'dump':
            self.dump(args[0] if args else None)
        elif cmd == 'shot':
            self.shot(args[0] if args else None)
        elif cmd == 'poke':
            name, _, offset = args[0].partition('+')
            address = (lookup(self.symbols, name)[0] if not name.isdigit() and not name.startswith('0x')
                       else int(name, 0)) + (int(offset, 0) if offset else 0)
            m.wr(address, int(args[1], 0), int(args[2]) if len(args) > 2 else 8)
        elif cmd == 'set':
            self.vars[args[0]] = self.eval(line.split(None, 2)[2])
        elif cmd == 'include':
            path = (self.script_dir / args[0]).resolve()
            self.run_script(path.read_text().splitlines(), path.parent)
        elif cmd == 'echo':
            print(f'[{m.frame:6d}] {line.split(None, 1)[1]} = {self.eval(line.split(None, 1)[1])}')
        else:
            raise ValueError(f'ligne {number}: commande inconnue {cmd}')

    script_dir = Path('.')

    def run_script(self, lines, directory=None):
        previous = self.script_dir
        if directory is not None:
            self.script_dir = Path(directory)
        try:
            for number, raw in enumerate(lines, 1):
                stripped = raw.strip()
                # `#` debute un commentaire, sauf apres expect ou ` # ` introduit le message.
                line = stripped if stripped.lower().startswith('expect') else stripped.split('#', 1)[0].strip()
                if line:
                    self.run_line(line, number)
        finally:
            self.script_dir = previous

    def save(self):
        (self.out / 'dumps.json').write_text(json.dumps(self.dumps, indent=1) + '\n')
        rows = [flatten('', d, {}) for d in self.dumps]
        for name, data in (('dumps.csv', rows), ('trace.csv', self.trace_rows)):
            if data:
                fields = list(dict.fromkeys(k for r in data for k in r))
                with open(self.out / name, 'w', newline='') as handle:
                    writer = csv.DictWriter(handle, fields)
                    writer.writeheader()
                    writer.writerows(data)
        result = {'frames': self.machine.frame, 'checks': self.checks,
                  'passed': all(c['ok'] for c in self.checks)}
        (self.out / 'result.json').write_text(json.dumps(result, indent=1) + '\n')
        return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('script', nargs='?', type=Path, help='fichier de commandes (voir ci-dessus)')
    parser.add_argument('--out', type=Path, default=ROOT / 'scratchpad' / 'md-harness')
    parser.add_argument('--rom', type=Path, default=ROOT / 'sgdk' / 'out' / 'rom.bin')
    parser.add_argument('--elf', type=Path, default=ROOT / 'sgdk' / 'out' / 'release' / 'rom.out')
    parser.add_argument('--symbols', type=Path, default=ROOT / 'sgdk' / 'out' / 'release' / 'symbol.txt')
    parser.add_argument('--core', type=Path, default=DEFAULT_CORE)
    parser.add_argument('--dump-at', default='', help='images absolues ou lire la RAM, ex. 100,200')
    parser.add_argument('--shot-at', default='', help='images absolues a capturer, ex. 300,400')
    parser.add_argument('--trace', type=int, default=0, help='ligne trace.csv toutes les N images')
    parser.add_argument('--symbols-only', action='store_true', help='liste les symboles RAM et quitte')
    parser.add_argument('--quiet', action='store_true')
    args = parser.parse_args(argv)

    symbols = load_symbols(args.elf, args.symbols)
    if args.symbols_only:
        for name, (address, size) in sorted(symbols.items(), key=lambda kv: kv[1][0]):
            print(f'FF{address:04X} {size if size is not None else "?":>5} {name}')
        return 0
    if not args.script and not (args.dump_at or args.shot_at):
        parser.error('script ou --dump-at/--shot-at requis')
    if not args.core.exists():
        parser.error(f'core absent: {args.core}')
    harness = Harness(args.core, args.rom, symbols, args.out, args.quiet)
    harness.trace_every = args.trace
    lines = args.script.read_text().splitlines() if args.script else []
    marks = sorted({(int(f), 'dump') for f in args.dump_at.split(',') if f} |
                   {(int(f), 'shot') for f in args.shot_at.split(',') if f})
    # Les points --dump-at/--shot-at sont inseres comme lignes `@F` sans changer les boutons.
    if marks:
        lines = [f'__mark {f} {kind}' for f, kind in marks] if not lines else lines
    status = 0
    try:
        if lines and lines[0].startswith('__mark'):
            for line in lines:
                _, frame, kind = line.split()
                harness.step(int(frame) - harness.machine.frame)
                harness.dump() if kind == 'dump' else harness.shot()
        else:
            pending = list(marks)
            original_step = harness.step

            def step_with_marks(n, buttons=None):
                end = harness.machine.frame + n
                while pending and pending[0][0] <= end:
                    frame, kind = pending.pop(0)
                    if frame >= harness.machine.frame:
                        original_step(frame - harness.machine.frame, buttons)
                        buttons = None
                        harness.dump() if kind == 'dump' else harness.shot()
                original_step(end - harness.machine.frame, buttons)
            harness.step = step_with_marks
            harness.run_script(lines, args.script.parent if args.script else None)
    except (TimeoutError, ValueError, KeyError) as exc:
        print(f'ERREUR: {exc}', file=sys.stderr)
        harness.checks.append({'frame': harness.machine.frame, 'expr': 'script', 'ok': False,
                               'error': str(exc)})
        try:
            harness.shot('error')
        except RuntimeError:
            pass
        status = 2
    result = harness.save()
    passed = sum(c['ok'] for c in result['checks'])
    print(f"{passed}/{len(result['checks'])} assertions OK, {result['frames']} images -> {args.out}")
    harness.machine.close()
    if status:
        return status
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
