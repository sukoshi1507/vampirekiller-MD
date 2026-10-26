#!/usr/bin/env python3
"""Produce a ROM-grounded 18-stage room inventory and Stage 1 edge probes.

Wall/ground probes indicate geometric *possibility*, never actual door links.
"""
import argparse
import json
from pathlib import Path
from probe import load_rom
from extract_rooms import decode_room

ROOT = Path(__file__).resolve().parents[1]

def solid(tile, limit):
    return ((tile - 1) & 255) < limit

def sample(room, x, y):
    index = (((y - 16) & 248) << 2) + (x >> 3)
    return room[index] if index < 768 else 0

def passable(room, side, feet, limit):
    # Original four wall probes at x +/- 8, feet - 2, -10, -18, -26.
    x = 8 if side == 'left' else 248
    wall = any(solid(sample(room, x, feet - 2 - 8 * row), limit) for row in range(4))
    ground = any(solid(sample(room, (x + off) & 255, feet), limit) for off in (5, -5))
    return not wall, ground

def build(rom):
    catalogue = json.loads((ROOT/'assets/manifest.json').read_text())
    stage_counts = {}
    records = []
    for rec in catalogue['rooms']:
        stage, index = rec['stage'], rec['room']
        stage_counts[stage] = max(stage_counts.get(stage, 0), index + 1)
        records.append({'stage':stage,'room':index,'graphics_status':rec['status'],
                        'map_rom_offset':rec['map_file_offset'],'unresolved_tiles':rec['unresolved_tile_ids'],
                        'links_status':'not_verified','stairs_status':'not_verified'})
    assert len(records) == 153
    probes = []
    limits = [6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6]
    # Decode collision limit table directly from ROM via its documented values
    # reported separately: stage-1 limit from player assets C table.
    import re
    src = (ROOT/'sgdk/src/generated/player_assets.c').read_text()
    values = re.search(r'vk_collision_limits\[19\]\s*=\s*\{(.*?)\}', src, re.S)
    if not values: raise ValueError('Collision limit table missing')
    limits = [int(x) for x in re.findall(r'\b\d+\b', values.group(1))]
    assert len(limits) == 19
    stage1maps = [decode_room(rom,1,r)[2] for r in range(8)]
    for room_id, tilemap in enumerate(stage1maps):
        for side in ('left','right'):
            candidates = []
            for feet in range(48,201,8):
                clear,ground = passable(tilemap,side,feet,limits[1])
                if clear:
                    candidates.append({'feet_y':feet,'supported':ground})
            probes.append({'room':room_id,'side':side,'potential_y':candidates,
                           'candidate_count':len(candidates),
                           'status':'geometry_only_not_a_verified_exit'})
    return {'rom_reference_sha256':catalogue['rom_sha256'],
            'stage_count':18,'room_records':len(records),'viewer_rooms':catalogue['viewer_rooms'],
            'stage_room_counts':stage_counts,'rooms':records,
            'stage1_edge_geometry':probes,
            'notes':['Geometry alone cannot identify a door or adjacent room.',
                     'Stage 1 rooms 1-7 have two hidden top rows still to confirm in the existing SGDK collision resource.',
                     'The V27 sequential debug graph is not a verified MSX2 topology.']}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('rom',type=Path);ap.add_argument('--out',type=Path,default=ROOT/'research/castle-navigation')
    args=ap.parse_args(); report=build(load_rom(args.rom));args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'navigation-audit.json').write_text(json.dumps(report,indent=2)+'\n')
    lines=['# Audit ROM de la carte de Vampire Killer MSX2','',
           f"- {report['room_records']} salles dans {report['stage_count']} stages ; {report['viewer_rooms']} fonds affichables.",
           '- Connexions authentifiées : **aucune** par cet audit statique.',
           '- Le graphe horizontal V27 reste un outil de test et non la topologie originale.','',
           '## Distribution par stage','', '| Stage | Salles |', '|---:|---:|']
    lines += [f'| {s} | {n} |' for s,n in sorted(report['stage_room_counts'].items())]
    lines += ['', '## Sondes sur les bords du stage 1','',
              '| Salle | Bord | Positions non bloquées |', '|---:|---|---:|']
    lines += [f"| {p['room']} | {p['side']} | {p['candidate_count']} |" for p in report['stage1_edge_geometry']]
    lines += ['', 'Ces positions indiquent uniquement une ouverture géométrique possible. Une porte, un escalier ou une connexion ne peut pas être déduite de ces sondes seules.']
    (args.out/'NAVIGATION_AUDIT.md').write_text('\n'.join(lines)+'\n')
    print(f"Audited {report['room_records']} ROM records and {len(report['stage1_edge_geometry'])} Stage 1 edge samples")
if __name__=='__main__':main()
