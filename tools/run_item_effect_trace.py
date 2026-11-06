#!/usr/bin/env python3
"""Lance openMSX avec tools/vertical_links_trace.tcl, tools/simon_blink_trace_extra.tcl et
tools/item_effect_trace_extra.tcl (effet d'un objet ramasse sur les ennemis).

Usage : run_item_effect_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
ROM.rom : ROM de reference, hors du depot (copiee dans un dossier temporaire).
Plans utilises (research/castle-candles.md 9.8, livres 10 et 11) :
  - hall salle 6, vrai ramassage du livre 10 de la bougie (112,128) au fouet ;
  - hall salle 0 avec des ennemis a l'ecran : « item 10 », « item 11 », puis « item 05 »
    (temoin : 7806 detruit tous les ennemis).
Sorties : items.csv, items_ev.csv (voir l'en-tete du .tcl), shots/ (rendu SCREEN 5).
"""
import os, shutil, subprocess, sys, tempfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
rom = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
plan = Path(sys.argv[3]).read_text()
timeout = int(sys.argv[4]) if len(sys.argv) > 4 else 900
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='vk-item-') as tmp:
    tmp = Path(tmp)
    shutil.copy(rom, tmp / 'game.rom')
    script = 'set vk_out {%s}\nset vk_plan {%s}\n' % (out, plan)
    for name in ('vertical_links_trace.tcl', 'simon_blink_trace_extra.tcl', 'item_effect_trace_extra.tcl'):
        script += '\n' + (TOOLS / name).read_text()
    (tmp / 'trace.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (out / 'openmsx.log').open('w') as log:
        subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=timeout)
print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')
