#!/usr/bin/env python3
"""Desassemblage Z80 d'une fenetre de la ROM de reference (lecture seule). Depend de z80dis==1.0.6.

Usage : disasm_rom_window.py ROM.rom FILE_BASE_HEX START_HEX END_HEX WINDOW_HEX
FILE_BASE = debut de la banque dans le fichier (banque * 0x2000), WINDOW = adresse CPU de cette fenetre.
Exemples : banque 0 en 4000 : FILE_BASE=0 WINDOW=4000 ; banque 1 en 6000 : FILE_BASE=2000 WINDOW=6000.
Le mapping des fenetres 6000 et 8000 est une hypothese de lecture ; verifier par trace avant d'en deduire une adresse en jeu.
"""
import sys
from z80dis import z80

rom = open(sys.argv[1], 'rb').read()
base, start, end, window = (int(x, 16) for x in sys.argv[2:6])
a = start
while a < end:
    o = base + (a - window)
    ins = z80.decode(rom[o:o + 4], a)
    print(f"{a:04X} {rom[o:o + ins.len].hex(' '):12s} {z80.disasm(ins)}")
    a += ins.len
