#!/usr/bin/env python3
"""openMSX : aigle (0F) et puce (23h) du stage 11 dans une salle forcee, Simon cloue par poke a chaque image.

Usage : [KEEP=1] run_eagle_flea_trace.py OUTDIR ROOM X Y FRAMES [WHIP_PERIOD]
Force D000 = 11, D001 = ROOM, D002 = [1E6A + 11], C000 = 4 (comme tools/species_trace.tcl), cloue Simon en (X, Y) (C427, C425),
ecrit OUTDIR/log.txt : une ligne par image (Simon, energie C415, invulnerabilite C42D, fouet, CF0C/CF0D/CF10, R, puis les blocs C800 non vides :
type, etat, Y, X, vitesses 8.8, pose, +C, PV, drapeaux). KEEP=1 : Simon reste cloue apres un coup (l'energie est remise a 32 sous 8).
WHIP_PERIOD > 0 : ESPACE (fouet) appuye 3 images toutes les WHIP_PERIOD images. Lecture seule hors ces pokes."""
import os, subprocess, sys, tempfile
from pathlib import Path
from probe import load_rom, reference_rom

TOOLS = Path(__file__).resolve().parent


def main():
    out = Path(sys.argv[1]).resolve()
    room, x, y, frames = (int(v) for v in sys.argv[2:6])
    whip = int(sys.argv[6]) if len(sys.argv) > 6 else 0
    rom = load_rom(reference_rom())
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='eagle-flea-') as tmp:
        tmp = Path(tmp)
        (tmp / 'game.rom').write_bytes(rom)
        head = ('set ef_out {%s}\nset ef_room %d\nset ef_d2 %d\nset ef_x %d\nset ef_y %d\nset ef_frames %d\nset ef_whip %d\n'
                'set c415 32\nset ef_keep %d\n' % (out, room, rom[0x1E6A + 11], x, y, frames, whip, int(os.environ.get('KEEP', '0'))))
        (tmp / 'trace.tcl').write_text(head + (TOOLS / 'eagle_flea_trace.tcl').read_text())
        env = dict(os.environ, OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        with (out / 'openmsx.log').open('w') as log:
            subprocess.run(['/usr/bin/openmsx', '-control', 'stdio', '-machine', 'C-BIOS_MSX2', '-cart', str(tmp / 'game.rom'),
                            '-script', str(tmp / 'trace.tcl')], stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                           env=env, timeout=900)
    print('done' if (out / 'done.txt').is_file() else 'NO DONE MARKER')


if __name__ == '__main__':
    main()
