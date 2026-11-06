#!/usr/bin/env python3
"""Recherche statique des operandes D000/D001 et C000 dans la ROM de reference (lecture seule).

Usage : door_room7_scan.py ROM.rom
Affiche, pour chaque occurrence de l'adresse little-endian (00 D0, 01 D0, 00 C0), l'offset fichier,
l'octet d'opcode precedent et les octets voisins. Aucune execution de code.
"""
import sys

rom = open(sys.argv[1], 'rb').read()
targets = {'D000': b'\x00\xd0', 'D001': b'\x01\xd0', 'C000': b'\x00\xc0', 'C41B': b'\x1b\xc4'}
for name, pat in targets.items():
    i = rom.find(pat)
    hits = []
    while i != -1:
        if i >= 1:
            hits.append(i)
        i = rom.find(pat, i + 1)
    print(f'{name}: {len(hits)} occurrences')
    for h in hits:
        ctx = rom[max(0, h - 3):h + 4].hex(' ')
        print(f'  off={h:06X} bank={h // 0x2000:02d} op={rom[h-1]:02X} ctx=[{ctx}]')
