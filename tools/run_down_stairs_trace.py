#!/usr/bin/env python3
"""Lance openMSX avec la trace des liens verticaux plus les ecritures de C420/C421.

Usage : run_down_stairs_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
Le script enchaine tools/vertical_links_trace.tcl et tools/down_stairs_trace_extra.tcl.
ROM.rom : ROM extraite de l'archive de reference (131072 octets), hors du depot.
Rien n'est ecrit dans le depot : la ROM est copiee dans un dossier temporaire.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='dn-vl-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = ('set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan))
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'down_stairs_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
