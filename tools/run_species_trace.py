#!/usr/bin/env python3
"""openMSX: which species (C800 types, D700 kinds) really exist in each room of stages 12-18.

Usage: run_species_trace.py OUTDIR [FRAMES_PER_ROOM [WALK(0/1) [FIRST_STAGE LAST_STAGE]]]
Forces each room (D000 = stage, D002 = [1E6A + stage], D001 = room, C000 = 4), samples C800/D700 at every
VBLANK and writes OUTDIR/species.txt plus OUTDIR/species.json: {"stage:room": {"C": [raw types], "D": [kinds]}}
keyed by the room actually observed (Simon may walk out of the forced room). C800 types are raw (bit 7 cleared);
D700 kinds are the byte 0 of the D700 slots (4 = bones of raw 11, 9 = shot of raw 16, 1 = F0 projectile);
'poses' gives the C800 +11 values seen per raw type. SP_X=64 places Simon left of C0 so that the 9DE7 channel runs,
SP_ONLY=r1,r2 restricts the rooms, SP_DBG=1 logs the channel state."""
import json, os, re, shutil, subprocess, sys, tempfile
from pathlib import Path
from probe import load_rom, reference_rom
from decode_room_links import decode

TOOLS = Path(__file__).resolve().parent


def main():
    out = Path(sys.argv[1]).resolve()
    frames = int(sys.argv[2]) if len(sys.argv) > 2 else 900
    walk = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    first, last = (int(sys.argv[4]), int(sys.argv[5])) if len(sys.argv) > 5 else (12, 18)
    out.mkdir(parents=True, exist_ok=True)
    rom = load_rom(reference_rom())
    graphs = decode(rom)
    rooms = [(s, r, rom[0x1E6A + s]) for s in range(first, last + 1) for r in range(len(graphs[s]['rooms']))
             if not os.environ.get('SP_ONLY') or r in [int(x) for x in os.environ['SP_ONLY'].split(',')]]
    with tempfile.TemporaryDirectory(prefix='species-') as tmp:
        tmp = Path(tmp)
        (tmp / 'game.rom').write_bytes(rom)
        head = 'set sp_out {%s}\nset sp_frames %d\nset sp_walk %d\nset sp_rooms {%s}\n' % (
            out, frames, walk, ' '.join('{%d %d %d}' % q for q in rooms))
        if os.environ.get('SP_X'):
            head += 'set sp_x %d\n' % int(os.environ['SP_X'])
        if os.environ.get('SP_DBG'):
            head += 'set sp_dbg 1\n'
        (tmp / 'trace.tcl').write_text(head +(TOOLS / 'species_trace.tcl').read_text())
        env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        with (out / 'openmsx.log').open('w') as log:
            subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2', '-cart', str(tmp / 'game.rom'),
                            '-script', str(tmp / 'trace.tcl')], stdin=subprocess.DEVNULL, stdout=log,
                           stderr=subprocess.STDOUT, env=env, timeout=3000)
    res = {}
    for line in (out / 'species.txt').read_text().splitlines():
        m = re.match(r'(\d+) (\d+) ([CD]) (\d+) frame=(\d+)', line)
        if m:
            res.setdefault('%s:%s' % m.group(1, 2), {'C': [], 'D': []})[m.group(3)].append(int(m.group(4)))
    for line in (out / 'species.txt').read_text().splitlines():
        m = re.match(r'POSE (\d+) (\d+) (\d+) (\d+) ', line)
        if m and '%s:%s' % m.group(1, 2) in res:
            res['%s:%s' % m.group(1, 2)].setdefault('poses', {}).setdefault(m.group(3), []).append(int(m.group(4)))
    for v in res.values():
        v['C'] = sorted(set(v['C'])); v['D'] = sorted(set(v['D']))
        v['poses'] = {k: sorted(set(p)) for k, p in v.get('poses', {}).items()}
    (out / 'species.json').write_text(json.dumps(res, indent=1, sort_keys=True))
    print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER', len(res), 'rooms observed')


if __name__ == '__main__':
    main()
