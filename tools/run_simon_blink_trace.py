#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl et tools/simon_blink_trace_extra.tcl.

Usage : run_simon_blink_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
ROM.rom : ROM de reference, hors du depot (copiee dans un dossier temporaire).
Plan utilise (research/castle-candles.md 9.8) : cour vers le hall (hold R 1500 s0/s1/s2),
wait 600 (contact naturel d'un ennemi : C42D), puis shots 20 / item 09 / wait 400 /
shots 20 / item 08 / wait 400.
Analyse : tools/analyze_simon_blink.py OUTDIR.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-blink-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'simon_blink_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
