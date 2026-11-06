#!/usr/bin/env python3
"""Verifie en jeu la boite de collision du fouet et la cible d'entrainement (MD).

Usage: check_whip_hitbox.py CORE.so SOURCE_ROM.zip MD_ROM.bin SYMBOLS.txt OUT_DIR

Pilote Genesis Plus GX (libretro) comme tools/verify_play_mode.py, puis lit et ecrit
la RAM systeme 68k aux adresses des symboles player, whip et de la cible. Dans la cour,
la cible testee est courtyard_flame[0] (FLAME_A, meme structure VKTarget) : main.c ne
teste plus test_target dans la cour. Repli sur test_target pour une ROM plus ancienne.
Ecrit uniquement dans OUT_DIR (ne touche ni validation/ ni sgdk/src/).

Boite attendue (sgdk/src/whip.c, vk_whip_bounds, bornes INCLUSIVES) : debout, face
droite x+8..x+32, face gauche x-32..x-8, y-22..y-15. Hit (sgdk/src/targets.c) : pas de
chevauchement si r < t.x ou l >= t.x + width (haut/bas identiques), donc touche iff
-8 < dx <= 32 (droite) et -48 < dx <= -8 (gauche) avec dx = t.x - x, et
-38 < dy <= -15 avec dy = t.y - y (cible 16x16).
Dans la cour, le reset (A+B) pose la cible en FLAME_A (64,144, sgdk/src/main.c) : le
script verifie cette position, puis pose la cible par RAM a une position explicite pour
le parcours et les balayages, sans dependre du spawn.
"""
import argparse
import ctypes as C
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from PIL import Image, ImageChops, ImageDraw  # noqa: E402

SYSTEM_RAM = 2


class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                ('size', C.c_size_t), ('meta', C.c_char_p)]


def s32(value):
    value &= 0xFFFFFFFF
    return value - (1 << 32) if value >= 1 << 31 else value


class Emulator:
    """Meme chargement libretro que tools/run_headless.py, avec acces RAM en ecriture."""

    def __init__(self, core, rom):
        self.lib = lib = C.CDLL(str(core.resolve()))
        self.format = 2
        self.buttons = set()
        self.frame = None
        self.log = []
        self.count = 0
        self.dir = C.create_string_buffer(str(rom.parent.resolve()).encode())
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
            if command in (9, 31):
                C.cast(data, C.POINTER(C.c_void_p))[0] = C.addressof(self.dir)
                return True
            if command == 3:
                C.cast(data, C.POINTER(C.c_bool))[0] = True
                return True
            if command == 39:
                C.cast(data, C.POINTER(C.c_uint))[0] = 0
                return True
            return command in (11, 16, 18, 35, 37)

        def video(data, width, height, pitch):
            if not data or data == C.c_void_p(-1).value:
                return
            size = 4 if self.format == 1 else 2
            raw = b''.join(C.string_at(data + y * pitch, width * size) for y in range(height))
            self.frame = (width, height, raw)

        def audio(left, right):
            pass

        def batch(data, count):
            return count

        def poll():
            pass

        def input_state(port, device, index, button):
            return int(port == 0 and button in self.buttons)

        self._keep = [Env(environment), Video(video), Audio(audio), Batch(batch),
                      Poll(poll), Input(input_state)]
        for name, cb in zip(('environment', 'video_refresh', 'audio_sample',
                             'audio_sample_batch', 'input_poll', 'input_state'), self._keep):
            getattr(lib, 'retro_set_' + name)(cb)
        lib.retro_load_game.argtypes = [C.POINTER(Game)]
        lib.retro_load_game.restype = C.c_bool
        lib.retro_get_memory_data.argtypes = [C.c_uint]
        lib.retro_get_memory_data.restype = C.c_void_p
        lib.retro_get_memory_size.argtypes = [C.c_uint]
        lib.retro_get_memory_size.restype = C.c_size_t
        lib.retro_init()
        raw = rom.read_bytes()
        self._buffer = C.create_string_buffer(raw)
        game = Game(str(rom.resolve()).encode(), C.cast(self._buffer, C.c_void_p), len(raw), None)
        if not lib.retro_load_game(C.byref(game)):
            raise RuntimeError('Core libretro: chargement refuse')
        lib.retro_set_controller_port_device(0, 1)
        pointer = lib.retro_get_memory_data(SYSTEM_RAM)
        size = lib.retro_get_memory_size(SYSTEM_RAM)
        if not pointer or size != 65536:
            raise RuntimeError('RAM systeme 64 Ko non exposee par le core')
        self.ram = (C.c_uint8 * size).from_address(pointer)  # vue vivante, ecriture possible

    # Genesis Plus GX stocke la RAM 68k avec les octets inverses dans chaque mot.
    def rd8(self, a):
        return self.ram[a ^ 1]

    def rd16(self, a):
        return (self.ram[a ^ 1] << 8) | self.ram[(a + 1) ^ 1]

    def rd32(self, a):
        return s32(sum(self.ram[(a + i) ^ 1] << (24 - 8 * i) for i in range(4)))

    def wr8(self, a, v):
        self.ram[a ^ 1] = v & 255

    def wr32(self, a, v):
        v &= 0xFFFFFFFF
        for i in range(4):
            self.ram[(a + i) ^ 1] = (v >> (24 - 8 * i)) & 255

    def set_symbols(self, player, whip, target):
        self.P, self.W, self.T = player, whip, target

    def snap(self):
        P, W, T = self.P, self.W, self.T
        return {
            'n': self.count, 'x': self.rd8(P), 'y': self.rd8(P + 1), 'facing': self.rd8(P + 2),
            'crouched': self.rd8(P + 7), 'timer': self.rd8(W), 'wfacing': self.rd8(W + 1),
            'wx': self.rd8(W + 2), 'wy': self.rd8(W + 3), 'wcrouched': self.rd8(W + 4),
            'tx': self.rd32(T), 'ty': self.rd32(T + 4), 'tw': self.rd32(T + 8),
            'th': self.rd32(T + 12), 'active': self.rd8(T + 16), 'latched': self.rd8(T + 17),
            'buttons': sorted(self.buttons),
        }

    def poke_target(self, x, y, active=1, latched=0):
        self.wr32(self.T, x)
        self.wr32(self.T + 4, y)
        self.wr8(self.T + 16, active)
        self.wr8(self.T + 17, latched)

    def frames(self, n, keys=(), on_frame=None):
        self.buttons = set(keys)
        snap = None
        for _ in range(n):
            self.lib.retro_run()
            self.count += 1
            snap = self.snap()
            self.log.append(snap)
            if on_frame:
                on_frame(snap)
        self.buttons = set()
        return snap

    def image(self):
        width, height, raw = self.frame
        if self.format == 1:
            return Image.frombytes('RGB', (width, height), raw, 'raw', 'BGRX')
        pixels = []
        for y in range(height):
            row = (C.c_uint16 * width).from_buffer_copy(raw[y * width * 2:(y + 1) * width * 2])
            for v in row:
                if self.format == 2:
                    pixels.append((((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63,
                                   (v & 31) * 255 // 31))
                else:
                    pixels.append((((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31,
                                   (v & 31) * 255 // 31))
        image = Image.new('RGB', (width, height))
        image.putdata(pixels)
        return image


def symbol_offsets(path):
    found = {}
    for match in re.finditer(r'^([0-9a-fA-F]+)\s+[bB]\s+(player|whip|test_target|courtyard_flame)$',
                             path.read_text(), re.M):
        found[match[2]] = int(match[1], 16) & 0xFFFF
    # La cour teste ses flammes (vk_targets_tick sur courtyard_flame), plus test_target.
    if 'courtyard_flame' in found:
        found['test_target'] = found['courtyard_flame']
    missing = {'player', 'whip', 'test_target'} - set(found)
    if missing:
        raise ValueError(f'symboles absents: {sorted(missing)}')
    return found


def wait_idle(emu):
    for _ in range(60):
        if emu.snap()['timer'] == 0:
            return
        emu.frames(1)
    raise RuntimeError('fouet non revenu au repos')


def attack(emu, on_frame=None):
    """Appuie B une image puis suit le fouet jusqu'au retour a timer 0."""
    wait_idle(emu)
    start = len(emu.log)
    emu.frames(1, [0], on_frame)   # B
    while emu.log[-1]['timer'] != 0:
        emu.frames(1, (), on_frame)
        if len(emu.log) - start > 60:
            raise RuntimeError('attaque trop longue')
    return emu.log[start:]


def hit_info(log):
    hits = [s for s in log if s['latched'] == 1]
    return {'hit': bool(hits), 'hit_timer': hits[0]['timer'] if hits else None}


def boot(emu):
    """Titre -> chateau -> cour (FRONT_COURTYARD), ou B est le fouet (voir main.c).

    La sequence de verify_play_mode.py (B depuis le visualiseur) ouvre FRONT_TEST,
    ou B revient au visualiseur et ou l'attaque est sur A : non utilisable ici.
    """
    emu.frames(180)            # logo, passe au titre automatiquement
    emu.frames(5, [3])         # START: titre -> chateau
    emu.frames(5)
    emu.frames(5, [3])         # START: chateau -> cour
    emu.frames(5)
    emu.frames(1, [1, 0])      # A+B: reinitialise la cible (FLAME_A, 64,144 dans la cour)
    emu.frames(3)


def whip_box(x, y, facing):
    """Boite debout INCLUSIVE (x0, y0, x1, y1) : copie de vk_whip_bounds (whip.c)."""
    if facing:
        return (x - 32, y - 22, x - 8, y - 15)
    return (x + 8, y - 22, x + 32, y - 15)


def lash_metrics(mask, x, y, facing):
    """Compte les pixels de sprite (masque != fond) par zone. Coordonnees ecran."""
    bx0, by0, bx1, by1 = whip_box(x, y, facing)
    zone = (x - 48, x + 72, y - 40, y + 8)
    stats = {'box_x': [bx0, bx1], 'box_y': [by0, by1], 'pixels_in_zone': 0,
             'pixels_in_box': 0, 'pixels_outside_box': 0, 'pixels_right_of_box': 0,
             'pixels_left_of_box': 0, 'bbox_zone': None, 'bbox_outside_box': None}
    bbox_all = [10 ** 9, 10 ** 9, -10 ** 9, -10 ** 9]
    bbox_out = list(bbox_all)
    for sy in range(zone[2], zone[3]):
        for sx in range(zone[0], zone[1]):
            if not mask.getpixel((sx, sy - 32)):
                continue
            stats['pixels_in_zone'] += 1
            bbox_all = [min(bbox_all[0], sx), min(bbox_all[1], sy),
                        max(bbox_all[2], sx), max(bbox_all[3], sy)]
            inside = bx0 <= sx <= bx1 and by0 <= sy <= by1
            if inside:
                stats['pixels_in_box'] += 1
            else:
                stats['pixels_outside_box'] += 1
                bbox_out = [min(bbox_out[0], sx), min(bbox_out[1], sy),
                            max(bbox_out[2], sx), max(bbox_out[3], sy)]
                if sx > bx1:
                    stats['pixels_right_of_box'] += 1
                if sx < bx0:
                    stats['pixels_left_of_box'] += 1
    if stats['pixels_in_zone']:
        stats['bbox_zone'] = bbox_all
    if stats['pixels_outside_box']:
        stats['bbox_outside_box'] = bbox_out
    # Lanière hors corps : droite x>=x+8 (face droite) ou gauche x<=x-9 (face gauche).
    lash = [(sx, sy) for sy in range(zone[2], zone[3]) for sx in range(zone[0], zone[1])
            if mask.getpixel((sx, sy - 32)) and ((sx >= x + 8) if not facing else (sx <= x - 9))]
    stats['lash_outside_body_px'] = len(lash)
    if lash:
        xs, ys = [p[0] for p in lash], [p[1] for p in lash]
        stats['bbox_lash_outside_body'] = [min(xs), min(ys), max(xs), max(ys)]
    return stats


def visual_pass(emu, label):
    """Capture les images du fouet pour chaque timer, cible et flammes masquees.

    Fond de reference = image de Simon au repos juste avant l'attaque (meme position,
    meme salle). Le masque = pixels qui changent : lanière + corps d'attaque.
    """
    emu.frames(20)  # flame_burst decroit a 0 : la cible ne reste pas visible
    x0, y0 = emu.log[-1]['x'], emu.log[-1]['y']
    emu.poke_target(x0 + 200, y0 - 24, active=0, latched=0)  # sprite cible masquee
    emu.frames(2)
    wait_idle(emu)
    reference = emu.image().crop((0, 32, 256, 208))
    shots = {}

    def on_frame(s):
        if 3 <= s['timer'] <= 14 and s['timer'] not in shots:
            frame = emu.image()
            crop = frame.crop((0, 32, 256, 208))
            mask = ImageChops.difference(crop, reference).convert('L').point(lambda v: 255 if v else 0)
            shots[s['timer']] = (frame, mask, s)

    attack(emu, on_frame)
    rows = {}
    for timer, (frame, mask, s) in sorted(shots.items(), reverse=True):
        rows[timer] = lash_metrics(mask, s['wx'], s['wy'], s['wfacing'])
        rows[timer]['whip_xy'] = [s['wx'], s['wy']]
        rows[timer]['facing'] = s['wfacing']
    key = shots.get(9)
    if key:
        frame, mask, s = key
        frame.save(emu_out / f'shot-{label}-timer09.png')
        scale = 4
        x, y = s['wx'], s['wy']
        region = (x - 56, y - 44, x + 80, y + 12)
        view = frame.crop(region).resize(((region[2] - region[0]) * scale, (region[3] - region[1]) * scale),
                                         Image.NEAREST)
        bx0, by0, bx1, by1 = whip_box(x, y, s['wfacing'])
        # Rectangle inclusif : le bord droit/bas est la derniere cellule (bx1, by1).
        rect = [(bx0 - region[0]) * scale, (by0 - region[1]) * scale,
                (bx1 + 1 - region[0]) * scale - 1, (by1 + 1 - region[1]) * scale - 1]
        draw = ImageDraw.Draw(view)
        draw.rectangle(rect, outline=(255, 0, 0))
        view.save(emu_out / f'box-{label}-timer09.png')
        overlay = view.copy()
        marks = ImageDraw.Draw(overlay)
        for sy in range(region[1], region[3]):
            for sx in range(region[0], region[2]):
                if mask.getpixel((sx, sy - 32)):
                    px0, py0 = (sx - region[0]) * scale, (sy - region[1]) * scale
                    marks.rectangle([px0, py0, px0 + scale - 1, py0 + scale - 1], fill=(255, 0, 255))
        marks.rectangle(rect, outline=(255, 0, 0))
        overlay.save(emu_out / f'mask-{label}-timer09.png')
    return rows


emu_out = Path('.')


def main():
    global emu_out
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core', type=Path)
    parser.add_argument('source_rom', type=Path)
    parser.add_argument('md_rom', type=Path)
    parser.add_argument('symbols', type=Path)
    parser.add_argument('out', type=Path)
    args = parser.parse_args()
    emu_out = args.out
    emu_out.mkdir(parents=True, exist_ok=True)
    offsets = symbol_offsets(args.symbols)
    emu = Emulator(args.core, args.md_rom)
    emu.set_symbols(offsets['player'], offsets['whip'], offsets['test_target'])
    boot(emu)
    checks, results = [], {}

    def check(name, ok, detail=None):
        checks.append({'name': name, 'ok': bool(ok), 'detail': detail})

    start = emu.snap()
    check('cour: Simon face droite', start['facing'] == 0, {k: start[k] for k in ('x', 'y', 'facing')})
    check('taille cible 16x16 (int 32 bits)', (start['tw'], start['th']) == (16, 16),
          {'tw': start['tw'], 'th': start['th']})
    # La cour pose la cible au reset (A+B) en FLAME_A (64,144) : verifie, sans spawn relatif.
    check('cour: cible au reset en FLAME_A (64,144), active, 16x16',
          (start['tx'], start['ty'], start['active']) == (64, 144, 1),
          {k: start[k] for k in ('tx', 'ty', 'active', 'tw', 'th')})

    # (1) Parcours reel: cible posee par RAM a dx=+40 depuis la position initiale de Simon,
    # puis Simon avance a droite de 7 puis 1 px (cible fixe, repositionnee avant chaque attaque).
    walk = []
    tgt_x, tgt_y = start['x'] + 40, start['y'] - 24
    for step in (0, 7, 1):
        if step:
            emu.frames(step, [7])
            emu.frames(3)
        emu.poke_target(tgt_x, tgt_y, active=1, latched=0)
        s0 = emu.snap()
        log = attack(emu)
        info = hit_info(log)
        walk.append({'walk_right_px': step, 'x': s0['x'], 'dx': s0['tx'] - s0['x'],
                     'target_active_before': s0['active'], **info})
    results['walk_right'] = walk
    check('parcours droit: touche iff -8 < dx <= 32 (dx mesures 40, 33, 32 : miss, miss, hit)',
          all(w['hit'] == (-8 < w['dx'] <= 32) for w in walk) and
          [w['dx'] for w in walk] == [40, 33, 32], walk)

    # (2) Balayage par RAM, face droite: touche iff -8 < dx <= 32 ; dy sans effet ici.
    def sweep(facing_label, deltas, expected, axis):
        rows = []
        for d in deltas:
            wait_idle(emu)
            s = emu.snap()
            if axis == 'x':
                tx, ty = s['x'] + d, s['y'] - 24
            else:
                tx, ty = s['x'] + 20, s['y'] + d
            emu.poke_target(tx, ty, active=1, latched=0)
            log = attack(emu)
            info = hit_info(log)
            rows.append({'d': d, 'hit': info['hit'], 'hit_timer': info['hit_timer'],
                         'expected': expected(d), 'poke_readback': [log[0]['tx'], log[0]['ty']]})
        results[facing_label] = rows
        check(f'{facing_label}: balayage {axis} conforme a la boite',
              all(r['hit'] == r['expected'] for r in rows), [r for r in rows if r['hit'] != r['expected']])

    sweep('sweep_right_dx', [-12, -8, -7, 0, 20, 32, 33, 40, 48],
          lambda d: -8 < d <= 32, 'x')
    sweep('sweep_right_dy', [-40, -38, -37, -24, -15, -14, -8, 0],
          lambda d: -38 < d <= -15, 'y')

    # (3) Fenetre temporelle: cible placee dans la boite au moment ou timer == t.
    window = []
    for t in range(14, -1, -1):
        wait_idle(emu)
        s = emu.snap()
        emu.poke_target(s['x'] + 200, s['y'] - 24, active=1, latched=0)
        fired = {'done': False}

        def hook(snap, t=t, fired=fired, px=s['x'], py=s['y']):
            if not fired['done'] and snap['timer'] == t:
                emu.poke_target(px + 20, py - 24, active=1, latched=0)
                fired['done'] = True

        info = hit_info(attack(emu, hook))
        # Cible deja dans la boite: touche a la premiere image active (timer 10) ou
        # a la premiere image suivant le poke (timer t-1) si t-1 est dans 3..10.
        window.append({'poke_at_timer': t, 'expected': t >= 4,
                       'expected_hit_timer': min(t - 1, 10) if t >= 4 else None,
                       'hit': info['hit'], 'hit_timer': info['hit_timer']})
    results['timer_window'] = window
    check('fenetre timer 3..10 (cible dans la boite: touche iff timer du poke >= 4, hit a min(t-1,10))',
          all(w['hit'] == w['expected'] and w['hit_timer'] == w['expected_hit_timer'] for w in window), window)

    # (4) Gauche: Simon marche a gauche d'un pixel, puis balayage mirror.
    emu.frames(1, [6])
    emu.frames(3)
    sweep('sweep_left_dx', [-60, -48, -47, -30, -9, -8, -7, 0],
          lambda d: -48 < d <= -8, 'x')
    check('gauche: facing = 1', emu.snap()['facing'] == 1, emu.snap()['facing'])

    # (5) Capture: cible masquee, mesure de la lanière face gauche puis droite.
    emu.frames(1, [7])
    emu.frames(3)
    results['visual_right'] = visual_pass(emu, 'right')
    # Face gauche loin du bord de l'ecran (x=0 coupe la lanière a x<0).
    emu.frames(40, [7])
    emu.frames(3)
    emu.frames(1, [6])
    emu.frames(3)
    results['visual_left'] = visual_pass(emu, 'left')

    failed = [c['name'] for c in checks if not c['ok']]
    report = {'core': str(args.core), 'core_sha256': hashlib.sha256(args.core.read_bytes()).hexdigest(),
              'all_passed': not failed, 'failed': failed, 'checks': checks, 'results': results}
    (args.out / 'whip-hitbox-ingame.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'all_passed': report['all_passed'], 'failed': failed}, indent=2))
    for c in checks:
        print(('OK  ' if c['ok'] else 'FAIL'), c['name'])


if __name__ == '__main__':
    main()
