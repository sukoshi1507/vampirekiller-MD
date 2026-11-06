#!/usr/bin/env python3
"""Calendrier d'affichage du port (research/frame-timing.md), mesure sur le harnais MD.

Usage : python3 tools/measure_md_frame_timing.py [--out DIR] [--rom ROM]

Dans Genesis Plus GX (libretro), un run (retro_run) commence par la V-Int puis balaye
l'image : l'image capturee apres le run r est donc l'image balayee apres la V-Int r, comme
l'image N du MSX est balayee apres le VBLANK N (tools/analyze_frame_timing.py). Le tour
de boucle de main.c qui suit la V-Int r (« iteration r ») lit la manette lue par cette
V-Int (JOY_update) ; ce qu'il change en RAM se lit apres le run r. Un bouton tenu pour
la premiere fois au run r0 est lu par l'iteration r0. Chaque mesure est rapportee en
images depuis r0 (ou depuis l'iteration de l'evenement), comme `image - VBLANK de la
lecture` cote MSX.

Mesures : fouet (son, premiere image changee autour de Simon, torche A de la cour
eteinte : flamme en sprite, support en tuiles), marche, saut, A+B (tuiles du support
redessinees sans fouet, pour situer l'ecriture des tuiles par rapport au faisceau) ; hall :
contact (barre du HUD, son 13h, Simon cache) et objet 05 (couleur de fond, pixel (2, 2)).
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import md_harness as H  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]


class Probe:
    def __init__(self, rom, out):
        symbols = H.load_symbols(ROOT / 'sgdk/out/release/rom.out', ROOT / 'sgdk/out/release/symbol.txt')
        self.h = H.Harness(H.DEFAULT_CORE, rom, symbols, out, quiet=True)
        self.m = self.h.machine
        self.psg = H.lookup(symbols, 'vk_psg')[0]

    def script(self, *lines):
        self.h.run_script(list(lines), ROOT / 'tests' / 'md_scenarios')

    def ev(self, expr):
        return self.h.eval(expr)

    def image(self):
        return np.asarray(self.m.image()).astype(int)

    def record(self, frames, buttons, fields):
        """`frames` runs avec `buttons` ; apres chaque run : image, demandes de son, champs."""
        rows = []
        for _ in range(frames):
            self.m.run(1, H.parse_buttons(buttons))
            # VKPsg (psg_core.h) : effect_count a +24.
            rows.append({'run': self.m.frame, 'img': self.image(), 'snd': self.m.rd16(self.psg + 24),
                         **{k: self.ev(e) for k, e in fields.items()}})
        return rows


def changed(rows, box, base):
    """Premier run dont l'image differe de `base` dans la zone `box` (x0, y0, x1, y1)."""
    x0, y0, x1, y1 = box
    return next((r['run'] for r in rows if (r['img'][y0:y1, x0:x1] != base[y0:y1, x0:x1]).any()), None)


def changes(rows, box, base):
    """Runs (relatifs a rien) ou la zone change par rapport a l'image precedente."""
    x0, y0, x1, y1 = box
    out, prev = [], base
    for r in rows:
        if (r['img'][y0:y1, x0:x1] != prev[y0:y1, x0:x1]).any():
            out.append(r['run'])
        prev = r['img']
    return out


def when(rows, test):
    return next((r['run'] for r in rows if test(r)), None)


def sound_runs(rows, before):
    out, last = [], before
    for r in rows:
        if r['snd'] != last:
            out.append(r['run'])
        last = r['snd']
    return out


def report(name, r0, items):
    print('%s (r0 = %d)' % (name, r0))
    for label, run in items:
        print('    %-46s : %s' % (label, 'non vu' if run is None else 'run %d (+%d)' % (run, run - r0)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--out', type=Path, default=ROOT / 'scratchpad' / 'frame-timing' / 'md')
    parser.add_argument('--rom', type=Path, default=ROOT / 'sgdk' / 'out' / 'rom.bin')
    args = parser.parse_args()
    p = Probe(args.rom, args.out)
    p.script('include boot_to_courtyard.inc')

    def simon_box(left=0):
        x, y = p.ev('game.player.x'), p.ev('game.player.y')
        return (max(left, x - 44), max(32, y - 40), min(256, x + 44), min(224, y + 2))

    # Fouet sur la torche A (cour, salle 0, x 44 comme le scenario i) ; Simon tourne a droite.
    p.script('poke vk_game+0 44', 'wait 30')
    base, before = p.image(), p.m.rd16(p.psg + 24)
    r0 = p.m.frame + 1
    fields = {'lit': 'flames[0].active', 'timer': 'whip.timer'}
    rows = p.record(2, 'B', fields) + p.record(26, '-', fields)
    out = when(rows, lambda r: not r['lit'])
    snd = sound_runs(rows, before)
    torch = (64, 144, 80, 160)   # flamme (sprite 16 x 16) et cellules du support (tuiles)
    report('Fouet sur la torche A', r0, [
        ('son du fouet demande (iteration)', snd[0] if snd else None),
        ('Simon : premiere image changee', changed(rows, (0, 120, 60, 200), base)),
        ('torche eteinte en RAM (iteration)', out),
        ('son du coup demande (iteration)', snd[1] if len(snd) > 1 else None),
    ])
    print('    torche (x 64..79, y 144..159) change aux runs :', [r - r0 for r in changes(rows, torch, base)])
    print('    whip.timer :', [(r['run'] - r0, r['timer']) for r in rows[:16]])

    # A+B : torches rallumees (vk_reset_training_target) sans fouet : tuiles du support
    # (y 144..151, juste sous la flamme) et flamme (sprite) dans la meme iteration.
    p.script('wait 30', 'hold A 3')
    base = p.image()
    r0 = p.m.frame + 1
    rows = p.record(1, 'A+B', {'lit': 'flames[0].active'}) + p.record(6, 'A', {'lit': 'flames[0].active'})
    report('A+B : torche A rallumee', r0, [
        ('flamme rallumee en RAM (iteration)', when(rows, lambda r: r['lit'])),
        ('message ligne 26 (tuiles, y 208..215)', changed(rows, (8, 208, 248, 216), base)),
    ])
    torch_rows = changes(rows, (64, 144, 80, 160), base)
    print('    torche change aux runs :', [r - r0 for r in torch_rows])
    for r in rows[:3]:
        z = r['img'][144:160, 64:80]
        print('      +%d : pixels non noirs %d, rouges/jaunes (flamme) %d' % (
            r['run'] - r0, int((z.sum(axis=2) > 0).sum()), int(((z[..., 0] > 150) & (z[..., 2] < 120)).sum())))

    # Marche puis saut.
    p.script('wait 30')
    base = p.image()
    box = simon_box()
    r0 = p.m.frame + 1
    xs = p.ev('game.player.x')
    rows = p.record(12, 'RIGHT', {'x': 'game.player.x'})
    report('Marche (DROITE)', r0, [('X change en RAM (iteration)', when(rows, lambda r: r['x'] != xs)),
                                   ('Simon : premiere image changee', changed(rows, box, base))])
    print('    X :', [(r['run'] - r0, r['x']) for r in rows])
    p.script('wait 30')
    base = p.image()
    box = simon_box()
    r0 = p.m.frame + 1
    rows = p.record(3, 'C', {'y': 'game.player.y'}) + p.record(9, '-', {'y': 'game.player.y'})
    report('Saut (C)', r0, [('Y change en RAM (iteration)', when(rows, lambda r: r['y'] != rows[0]['y'])),
                            ('Simon : premiere image changee', changed(rows, box, base))])
    print('    Y :', [(r['run'] - r0, r['y']) for r in rows])

    # Hall : contact naturel d'un ennemi (barre du HUD, son 13h, Simon cache).
    p.script("until front_mode == MODES['INTERIOR'] max 3000 RIGHT", 'wait 40')
    energy0 = p.ev('enemies.energy')
    rows = []
    while len(rows) < 1500:
        rows += p.record(1, '-', {'energy': 'enemies.energy', 'invuln': 'enemies.invuln',
                                  'hidden': 'enemies.hidden'})
        hit = next((i for i, r in enumerate(rows) if r['energy'] < energy0), None)
        if hit is not None and len(rows) > hit + 12:
            break
    if hit is not None:
        r_hit = rows[hit]['run']
        prev = rows[hit - 1]['img']
        after = rows[hit:]
        report('Contact (iteration de la baisse d\'energie)', r_hit, [
            ('barre du HUD (tuiles, y 8..31)', changed(after, (56, 8, 128, 32), prev)),
            ('son 13h demande (iteration)', next((r['run'] for i, r in enumerate(after)
                                                 if r['snd'] != rows[hit - 1 + i]['snd']), None)),
            ('hidden (78CC) en RAM (iteration)', when(after, lambda r: r['hidden'])),
            ('premiere image changee sous le HUD', changed(after, (0, 32, 256, 224), prev)),
        ])
        print('    hidden :', [(r['run'] - r_hit, r['hidden']) for r in after[:12]])

    # Objet 05 pose sous Simon (scenario t) : eclair (C43E, 7604) -> couleur de fond.
    p.script('wait 200', 'poke vk_game+0 136', 'wait 2', 'poke vk_candles+7 128', 'poke vk_candles+8 176',
             'poke vk_candles+9 0x05', 'poke vk_candles+10 0xFF', 'poke vk_candles+11 0',
             'poke vk_candles+6 3')
    rows = p.record(12, '-', {'flash': 'enemies.flash', 'border': 'enemies.border',
                              'drop': "ram8('vk_candles+6')"})
    r0 = when(rows, lambda r: r['drop'] == 0)
    report('Objet 05 (iteration du ramassage)', r0, [
        ('flash (C43E) en RAM (iteration)', when(rows, lambda r: r['flash'])),
        ('border = 0Eh en RAM (iteration)', when(rows, lambda r: r['border'])),
        ('pixel (2, 2) eclaire (image)', when(rows, lambda r: tuple(r['img'][2, 2]) != (0, 0, 0))),
    ])
    p.m.close()


if __name__ == '__main__':
    main()
