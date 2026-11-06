#!/usr/bin/env python3
"""Predit les escaliers de Vampire Killer (MSX2) a partir des cartes de salles de la ROM.

Lecture statique, aucune emulation. Regles prouvees par le code Z80 (dump CPU
openMSX, scratchpad/stairs/runs/r3) et par traces (runs r4, r6/r7, r8 de
research/stairs.md) :
  - Les marches sont les tuiles 12 (0x0C, montee vers la droite) et 13 (0x0D,
    montee vers la gauche) de la carte 32x24 (fonction 7D2F, index
    ((y-16)>>3)*32 + (x>>3)). Une cellule vaut (x, y) = (8*col, 8*row + 16).
  - Entree par le bas (touche HAUT, routine 6B7D/6BDA) : pour la tuile 12,
    Simon est place en X = cellule.x au niveau des pieds y = cellule.y + 8
    (sonde 7CDB/7D05 sur la rangee y-8). Pour la tuile 13, X = cellule.x + 8.
  - Montee : tant que HAUT ou DROITE est tenu (6DE0 -> 6E84 pour C421=1),
    1 px par tranche de 2 images sur X et Y. Descente : BAS ou GAUCHE (6E4B).
    Relacher la touche arrete Simon sur l'escalier.
  - Sortie haute : sonde de sol 7B88 (solide sur (x+-5, y)), donc arret sur le
    sol. Prediction de sortie : (tx + 16, ty - 8) pour une volee montante
    vers la droite, dont la cellule haute est (tx, ty). Verifie sur les volees
    (48,184)->(112,128) et (144,120)->(176,96) de la salle 1 du stage 1.
  - Solidite (7C5E) : (tuile - 1) < limite du stage ; limite 4 pour les stages
    1 a 9 (table 7C78, stage 0 = 2), voir research/stairs.md.

Ce script ne sait pas (encore) predire les entrees par le haut (BAS, sonde 7C8B
sur la tuile 4) ni les sorties par une porte de salle. Ces cas sont marques.

Usage : analyze_stairs.py [ROM]   (defaut : scratchpad/stairs/...rom du depot)
"""
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import extract_rooms as er  # noqa: E402
from probe import load_rom  # noqa: E402
from decode_room_links import ROOM_COUNTS_AUDIT  # noqa: E402

DEFAULT = ROOT / 'scratchpad' / 'stairs' / 'Vampire Killer (Japan, Europe) (En).rom'


def cells_of(tilemap, tile):
    out = []
    for r in range(2, 24):
        for c in range(32):
            if tilemap[r * 32 + c] == tile:
                out.append((8 * c, 8 * r + 16))
    return out


def chains(cells, step):
    """Regroupe les cellules en volees : voisine = (x+step, y+8)."""
    left = set(cells)
    flights = []
    while left:
        start = min(left, key=lambda p: (p[1], p[0]))
        chain = [start]
        left.discard(start)
        cur = start
        while True:
            nxt = (cur[0] + step, cur[1] + 8)
            if nxt in left:
                chain.append(nxt)
                left.discard(nxt)
                cur = nxt
            else:
                break
        flights.append(chain)
    return flights


def predict(stage, room, tilemap):
    rows = []
    for tile, step, name in ((12, -8, 'montee droite'), (13, 8, 'montee gauche')):
        # Sur la carte, une volee montante a droite descend vers la gauche
        # (x diminue quand y augmente), donc voisine = (x-8, y+8) pour 12.
        for chain in chains(cells_of(tilemap, tile), step):
            bottom = max(chain, key=lambda p: p[1])
            top = min(chain, key=lambda p: p[1])
            if tile == 12:
                entry = (bottom[0], bottom[1] + 8)
                exit_ = (top[0] + 16, top[1] - 8)
            else:
                entry = (bottom[0] + 8, bottom[1] + 8)
                exit_ = (top[0] - 16, top[1] - 8)
            rows.append({'stage': stage, 'room': room, 'tile': tile, 'dir': name,
                         'cells': len(chain), 'bottom_cell': bottom, 'top_cell': top,
                         'entry_feet': entry, 'exit_pred': exit_})
    return rows


def main():
    rom = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
    data = load_rom(rom)
    print('stage\troom\ttile\tdir\tcells\tbottom_cell\ttop_cell\tentry_feet(x,y)\texit_pred(x,y)')
    for stage in range(1, 19):
        for room in range(ROOM_COUNTS_AUDIT[stage]):
            _, _, tilemap = er.decode_room(data, stage, room)
            for r in predict(stage, room, tilemap):
                print('%d\t%d\t%d\t%s\t%d\t%s\t%s\t(%d,%d)\t(%d,%d)' % (
                    r['stage'], r['room'], r['tile'], r['dir'], r['cells'],
                    r['bottom_cell'], r['top_cell'], r['entry_feet'][0], r['entry_feet'][1],
                    r['exit_pred'][0], r['exit_pred'][1]))


if __name__ == '__main__':
    main()
