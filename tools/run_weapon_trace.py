#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl et tools/weapon_trace_extra.tcl
(armes secondaires : dague, hache, croix, eau benite ; research/secondary-weapons.md).

Usage : run_weapon_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
ROM.rom : ROM de reference, hors du depot (copiee dans un dossier temporaire).
Plan : actions de vertical_links_trace.tcl (hold, wait, poke, gotox) plus « item OBJ »
(objet pose sous Simon, vrai ramassage 7FE2 -> 8D30) et « shots N » (instantanes VRAM).
Exemple (hall salle 0) : hold R 1500 s0 / hold R 1500 s1 / hold R 1500 s2 / hold R 1500 s3 /
poke 0xC415 32 / item 1B / wait 20 / hold S 2 dague / wait 60 ...
Sorties : weapon.csv, weapon_ev.csv (voir l'en-tete du .tcl), shots/.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-weapon-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'weapon_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
