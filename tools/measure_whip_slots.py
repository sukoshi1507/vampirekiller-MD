#!/usr/bin/env python3
"""Lance openMSX pour mesurer les emplacements de fouet (voir trace_whip_slots.tcl).

Usage : measure_whip_slots.py NIVEAU SORTIE_DIR [SECONDES]
  NIVEAU : 0, 1, 2 ou 3 (C416 force a cette valeur a 41 s), ou -1 (aucun forcage).
La ROM de reference est verifiee par SHA-256 (tools/probe.py). Sortie : slots.csv et done.txt dans
SORTIE_DIR. Variables OPENMSX et OPENMSX_SYSTEM_DATA a definir si openMSX n'est pas au chemin par
defaut (scratchpad). Aucune ecriture dans le depot hors SORTIE_DIR.
"""
import os, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from probe import load_rom, reference_rom  # noqa: E402  (verifie le SHA-256)

SCRATCH = Path(os.environ.get('VK_SCRATCH', '/tmp/claude-1000/-home-sukoshi-Documents-githubsuko-vampirekiller-MD/13d81fab-4688-46e1-bcca-3d7f6368905e/scratchpad'))
OPENMSX = Path(os.environ.get('OPENMSX', SCRATCH / 'openmsx/openmsx-21.0/derived/x86_64-linux-opt/bin/openmsx'))
SYSDATA = Path(os.environ.get('OPENMSX_SYSTEM_DATA', SCRATCH / 'omsx-share'))
TCL = Path(os.environ.get('VK_TCL', HERE / 'trace_whip_slots.tcl'))


def main():
    level, outdir = int(sys.argv[1]), Path(sys.argv[2])
    seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 100.0
    if level not in (-1, 0, 1, 2, 3):
        sys.exit('niveau attendu : -1, 0, 1, 2 ou 3')
    outdir.mkdir(parents=True, exist_ok=True)
    data = load_rom(reference_rom())
    with tempfile.TemporaryDirectory(prefix='vk-slots-') as tmp:
        tmp = Path(tmp)
        (tmp / 'game.rom').write_bytes(data)
        head = 'set vk_output {%s}\nset vk_level %d\nset vk_end %s\n' % (outdir.resolve(), level, seconds)
        (tmp / 'trace.tcl').write_text(head + TCL.read_text())
        env = dict(os.environ, OPENMSX_SYSTEM_DATA=str(SYSDATA), OPENMSX_USER_DATA=str(tmp / 'user'),
                   SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        with (outdir / 'openmsx.log').open('w') as log:
            subprocess.run([str(OPENMSX), '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                            '-cart', str(tmp / 'game.rom'), '-script', str(tmp / 'trace.tcl')],
                           stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, env=env,
                           timeout=int(seconds * 6) + 120, check=False)
    print('done', outdir)


if __name__ == '__main__':
    main()
