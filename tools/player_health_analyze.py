# Analyse d'une sortie de tools/player_health_trace.tcl.
# Usage : python3 -I tools/player_health_analyze.py <dossier>
# Affiche : changements des octets d'état, changements d'énergie C415 (lue dans ram-c400.bin)
# et écritures avec PC pour les octets d'intérêt.
import csv, struct, sys

d = sys.argv[1]
rows = list(csv.DictReader(open(d + '/frames.csv')))
dump = open(d + '/ram-c400.bin', 'rb').read()
ram = {}
for i in range(0, len(dump), 516):
    f = struct.unpack('<i', dump[i:i+4])[0]
    ram[f] = dump[i+4:i+516]

KEYS = ['c000', 'c001', 'c410', 'c411', 'c413', 'c408', 'c409', 'c40a', 'c40c', 'c40d',
        'c41b', 'c420', 'c004', 'c002', 'c0a7', 'c405', 'c407', 'e600']
print('== Changements d\'octets d\'état (frame, temps, ancien->nouveau)')
prev = None
for r in rows:
    if prev is not None:
        ch = [k for k in KEYS if r[k] != prev[k]]
        if ch:
            print(r['frame'], round(float(r['time']), 2),
                  ' '.join(f'{k}:{prev[k]}->{r[k]}' for k in ch))
    prev = r

print('== Energie C415 (dump par image)')
prev = None
for f in sorted(ram):
    v = ram[f][0x15]
    if prev is not None and v != prev:
        print(f, f'{prev:02X}->{v:02X}')
    prev = v

print('== Ecritures avec PC (C410, C413, C415, C418, C420, C42D, C423)')
for r in csv.DictReader(open(d + '/writes.csv')):
    if r['addr'] in ('C410', 'C413', 'C415', 'C418', 'C420', 'C42D', 'C423'):
        if r['addr'] == 'C42D' and r['pc'] == '75D2':
            continue  # decrement du chronometre d'invulnerabilite, trop bavard
        print(round(float(r['time']), 3), r['pc'], r['addr'], r['value'], 'etat', r['c000'], r['c001'])
