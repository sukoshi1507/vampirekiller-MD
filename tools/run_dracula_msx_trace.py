#!/usr/bin/env python3
"""openMSX: Simon is driven by the keyboard matrix only (walk, UP jump, SPACE whip) in stage 18 room 9.
Usage: run_dracula_msx_trace.py OUTDIR [FRAMES [BOSS4 [CYCLE [X]]]]   BOSS4=1 pokes C418=4, BOSS4=2 pokes CE16=1 (both reach the second form quickly); DM_WALK = passes of walking per cycle."""
import os, subprocess, sys, tempfile
from pathlib import Path
from probe import load_rom, reference_rom

TOOLS = Path(__file__).resolve().parent


def main():
    out = Path(sys.argv[1]).resolve()
    frames = int(sys.argv[2]) if len(sys.argv) > 2 else 3000
    boss4 = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    cycle = int(sys.argv[4]) if len(sys.argv) > 4 else 80
    out.mkdir(parents=True, exist_ok=True)
    rom = load_rom(reference_rom())
    with tempfile.TemporaryDirectory(prefix='dracula-') as tmp:
        tmp = Path(tmp)
        (tmp / 'game.rom').write_bytes(rom)
        head = 'set dm_out {%s}\nset dm_frames %d\nset dm_boss4 %d\nset dm_cycle %d\nset dm_d002 %d\nset dm_walk %d\nset dm_room %d\n' % (
            out, frames, boss4, cycle, rom[0x1E6A + 18], int(os.environ.get('DM_WALK', 30)), int(os.environ.get('DM_ROOM', 9)))
        if len(sys.argv) > 5:
            head += 'set dm_x %d\n' % int(sys.argv[5])
        (tmp / 'trace.tcl').write_text(head + (TOOLS / 'dracula_msx_trace.tcl').read_text())
        env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        with (out / 'openmsx.log').open('w') as log:
            subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2', '-cart', str(tmp / 'game.rom'),
                            '-script', str(tmp / 'trace.tcl')], stdin=subprocess.DEVNULL, stdout=log,
                           stderr=subprocess.STDOUT, env=env, timeout=1500)
    print('trace written to', out / 'dracula.txt')


if __name__ == '__main__':
    main()
