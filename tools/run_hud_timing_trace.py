#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl, tools/hud_shield_capture.tcl et
tools/hud_timing_trace.tcl (case d'objets du HUD au ramassage : 8F23, 8F55, 8EE6).

Usage : run_hud_timing_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
Plan utilise (research/castle-candles.md 9.8) : entree au hall (hold R 1500 s0..s2),
energie pleine, gotox 70, fouet sur la bougie 09 (112, 144), gotox 140 pour ramasser
l'objet tombe, puis wait 700.
Sorties : box.csv, box.bin, calls.csv (hud_timing_trace.tcl), state.csv, events.txt.
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-hb-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    for name in ('vertical_links_trace.tcl', 'hud_shield_capture.tcl', 'hud_timing_trace.tcl'):
        script += '\n' + (TOOLS / name).read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
