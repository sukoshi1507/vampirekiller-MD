#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl et tools/door_respawn_trace_extra.tcl (generateurs pendant la porte).

Usage : run_door_respawn_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
ROM.rom : ROM de reference, hors du depot (copiee dans un dossier temporaire).
Plan (stage 2, salle 1) : route p6 de research/door-room7.md jusqu'au stage 2, puis poke C701 3, poke C41B 4 (lien bas de la
salle 0 : salle 1), poke C427 200 / C425 96, hold R 700 (ouverture de la porte, C5AC 2 puis 3). Stage 6, salle 4 : meme route,
puis poke D000 6, D001 4, D002 1, C701 3, C000 4, hold L jusqu'a la salle 4, poke C427 26 / C425 100, hold L 40.
Journaux : door.csv (etat et blocs objets C800-CF80 a chaque image), spawn.csv (ecritures de blocs avec PC), events.txt.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-dr-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'door_respawn_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
