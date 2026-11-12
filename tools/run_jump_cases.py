#!/usr/bin/env python3
"""openMSX: Simon's jump from given (stage, room, X, Y, direction) points, one line per hook of 4028 (tools/jump_cases_trace.tcl).
Usage: run_jump_cases.py OUTDIR [CASES.json]; CASES = [{"name","stage","room","x","y","dir":"L|R|N","high":0}].
Default cases: the stage 14/15 jumps of the controller routes and flat-ground jumps. Needs the reference ROM (never committed)."""
import json, os, subprocess, sys, tempfile
from pathlib import Path
from probe import load_rom, reference_rom

TOOLS = Path(__file__).resolve().parent
DIRS = {'L': 16, 'R': 128, 'N': 0, 'U': 32, 'D': 64}   # U: UP held all along (stairs; UP is also the MSX jump key), D: DOWN held (descent)
DEFAULT = [
    ('s15r1_to_block', 15, 1, 160, 80, 'R', 0), ('s15r1_block_to_ledge', 15, 1, 215, 112, 'R', 0),
    ('s15r3_to_block', 15, 3, 136, 176, 'L', 0), ('s15r4_to_upper', 15, 4, 104, 112, 'L', 0),
    ('s14r7_to_upper', 14, 7, 232, 112, 'L', 0), ('s14r5_down', 14, 5, 159, 112, 'L', 0),
    ('s15r8_hole', 15, 8, 137, 176, 'L', 0),
    ('flat_still', 15, 3, 200, 176, 'N', 0), ('flat_left', 15, 3, 200, 176, 'L', 0), ('flat_right', 15, 3, 100, 176, 'R', 0),
    ('flat_high_still', 15, 3, 200, 176, 'N', 1), ('flat_high_left', 15, 3, 200, 176, 'L', 1),
]


def main():
    out = Path(sys.argv[1]).resolve()
    out.mkdir(parents=True, exist_ok=True)
    cases = ([(c['name'], c['stage'], c['room'], c['x'], c['y'], c['dir'], c.get('high', 0), c.get('frames', 120), c.get('walk', 8)) for c in json.loads(Path(sys.argv[2]).read_text())]
             if len(sys.argv) > 2 else [c + (120, 8) for c in DEFAULT])
    rom = load_rom(reference_rom())
    lst = ' '.join('{%s %d %d %d %d %d %d %d %d %d}' % (n, s, r, x, y, DIRS[d], h, wk, rom[0x1E6A + s], fr) for n, s, r, x, y, d, h, fr, wk in cases)
    with tempfile.TemporaryDirectory(prefix='jc-') as tmp:
        tmp = Path(tmp)
        (tmp / 'game.rom').write_bytes(rom)
        (tmp / 't.tcl').write_text('set jc_out {%s}\nset jc_cases {%s}\n' % (out, lst) + (TOOLS / 'jump_cases_trace.tcl').read_text())
        env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        with (out / 'openmsx.log').open('w') as log:
            subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2', '-cart', str(tmp / 'game.rom'),
                            '-script', str(tmp / 't.tcl')], stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=600)
    print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')


if __name__ == '__main__':
    main()
