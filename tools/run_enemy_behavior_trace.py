#!/usr/bin/env python3
"""Lance openMSX avec tools/enemy_behavior_trace.tcl sur un plan d'actions.

Usage : [EB_EXTRA=fichier.tcl] run_enemy_behavior_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
EB_EXTRA : script Tcl optionnel charge apres la trace (ex. tools/enemy_spawn_writers.tcl).
ROM.rom : ROM extraite de l'archive de reference (131072 octets), hors du depot.
La ROM est copiee dans un dossier temporaire ; rien n'est ecrit dans le depot
hors des arguments fournis. Ne tue aucun processus openMSX existant.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='eb-trace-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set eb_out {%s}\nset eb_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'enemy_behavior_trace.tcl').read_text()
    extra = os.environ.get('EB_EXTRA')
    if extra:
        script += '\n' + Path(extra).read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'),
               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                       env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
