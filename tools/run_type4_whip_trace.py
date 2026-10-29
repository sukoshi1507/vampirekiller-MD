#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl et tools/type4_whip_trace_extra.tcl (fouet et contact du type 4).

Usage : run_type4_whip_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S], puis
check_type4_whip.py OUTDIR/t4.csv.
Plan utilise (research/enemy-behavior.md, « Type 4 : fouet ») : chemin de
run_armor_stage_trace.py jusqu'a la salle 0 du stage 2 (sans les poke d'armure), puis par coup :
poke C415 32, poke C427 X, poke C425 96 (wait 1 entre deux), hold L ou R 1, hold S 6, wait 14 ;
40 coups a X 200 regard a gauche, 40 a X 48 a droite, 25 a X 200 a droite, 25 a X 48 a gauche.

import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-ar-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'type4_whip_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
