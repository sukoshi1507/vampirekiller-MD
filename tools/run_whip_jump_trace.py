#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl et tools/whip_jump_trace_extra.tcl.

Usage : run_whip_jump_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
ROM.rom : ROM de reference, hors du depot (copiee dans un dossier temporaire).
Plans utilises (research/whip-hitbox.md, « Fouet en saut ») : cour 0, sauts verticaux et
avec elan (hold U, hold U+R) avec un fouet (hold S) ; run3, sous la torche A :
  gotox 44 200 approach / wait 20 / whipat 170 up / whipat 154 down / hold U 2 jump / wait 80
(une action par ligne ; whipat est defini par tools/whip_jump_trace_extra.tcl).
Lecture des sorties : research/whip-hitbox.md (section « Fouet en saut »).
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-wj-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    script += (TOOLS / 'vertical_links_trace.tcl').read_text()
    script += '\n' + (TOOLS / 'whip_jump_trace_extra.tcl').read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
