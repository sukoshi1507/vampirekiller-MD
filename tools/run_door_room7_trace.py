#!/usr/bin/env python3
"""Lance openMSX avec la trace de la porte de la salle 7 (hall, stage 1).

Usage : run_door_room7_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
Enchaine tools/vertical_links_trace.tcl puis tools/door_room7_trace.tcl.
ROM.rom : ROM extraite de l'archive de reference, hors du depot ; elle est copiee dans un dossier temporaire.
Journaux : frames.csv, frames_door.csv, writes.csv, events.txt, done.txt.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-door-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = ('set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan))
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'door_room7_trace.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
