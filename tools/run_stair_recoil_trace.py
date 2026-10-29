#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl et tools/stair_recoil_trace_extra.tcl.

Usage : [VK_TRIG="{2 0} {18 0}"] run_stair_recoil_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
ROM.rom : ROM de reference, hors du depot (copiee dans un dossier temporaire).
VK_TRIG : coups forces sur l'escalier {C42B mortel} (voir stair_recoil_trace_extra.tcl).
Plans utilises (research/player-health.md, research/stairs.md) :
  recul : hold R 1500 s0 / hold R 1500 s1 / hold R 1500 s2 / wait 1200
  escalier : idem puis wait 30, hold R 1500 h0, wait 30, gotox 48 200 g48, puis des
  hold U 4 / hold D 4 suivis de wait 60 a 120 ; teleportation au pied de la volee C
  (poke 0xC427 240, wait 1, poke 0xC425 96, wait 90).
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-sr-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'stair_recoil_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
