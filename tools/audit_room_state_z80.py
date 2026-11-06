#!/usr/bin/env python3
"""Conservative raw-byte audit of references to MSX2 room-state variables.

These are byte-pattern candidates, NOT confirmed decoded instructions: Z80 data,
inline tables and overlapping instructions can produce false positives.
"""
import argparse
import json
from pathlib import Path
from probe import load_rom, physical_offset
ROOT = Path(__file__).resolve().parents[1]
TARGETS = {0xD000: 'stage', 0xD001: 'room'}
# Opcodes containing absolute 16-bit operands, in normal Z80 encoding.
OPCODES = {0x32: 'LD (nn),A', 0x22: 'LD (nn),HL', 0x3A: 'LD A,(nn)', 0x2A: 'LD HL,(nn)'}
ED_OPCODES = {0x43:'LD (nn),BC',0x53:'LD (nn),DE',0x63:'LD (nn),HL',0x73:'LD (nn),SP',
              0x4B:'LD BC,(nn)',0x5B:'LD DE,(nn)',0x6B:'LD HL,(nn)',0x7B:'LD SP,(nn)'}

def scan(data):
    results = []
    for bank in range(16):
        base=bank*0x2000
        chunk=data[base:base+0x2000]
        for i in range(len(chunk)-3):
            op=chunk[i]
            if op == 0xED and chunk[i+1] in ED_OPCODES:
                opcode=ED_OPCODES[chunk[i+1]]; addr=chunk[i+2]+(chunk[i+3]<<8); ln=4
            elif op in OPCODES:
                opcode=OPCODES[op]; addr=chunk[i+1]+(chunk[i+2]<<8);ln=3
            else:
                continue
            if addr not in TARGETS:
                continue
            offset=base+i
            results.append({'file_offset':f'{offset:05X}', 'bank':bank,
                            'offset_in_bank':f'{i:04X}', 'operand':f'{addr:04X}',
                            'variable':TARGETS[addr],'candidate_opcode':opcode,
                            'candidate_access':'write' if opcode.startswith('LD (') else 'read',
                            'bytes':chunk[i:i+ln].hex(' '),
                            'context':chunk[max(0,i-8):min(len(chunk),i+ln+8)].hex(' '),
                            'status':'raw_pattern_not_confirmed_instruction'})
    return results

def report(data):
    records=scan(data)
    return {'method':'bank-local byte-pattern scan, not disassembly',
            'target_ram_addresses':{'stage':'D000','room':'D001'},
            'candidates':records,
            'summary':{'total':len(records),'write':sum(x['candidate_access']=='write' for x in records),
                       'read':sum(x['candidate_access']=='read' for x in records)},
            'limitations':['Raw matches may occur in data, tables or instruction operands.',
                           'Memory operands alone do not establish adjacency or destination.',
                           'Bank-local offsets do not determine runtime CPU address without mapper state.',
                           'Only runtime execution traces or disassembly verified by control flow can authenticate edges.']}

def main():
    p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('--out',type=Path,default=ROOT/'research/castle-navigation')
    args=p.parse_args(); data=load_rom(args.rom);r=report(data);args.out.mkdir(exist_ok=True,parents=True)
    (args.out/'z80-room-state-candidates.json').write_text(json.dumps(r,indent=2)+'\n')
    lines=['# Références candidates Z80 aux variables de navigation','',
           'Variables du chargeur : `D000` (stage), `D001` (salle).','',
           f"Recherche brute : **{r['summary']['total']} motifs**, dont {r['summary']['write']} écritures candidates et {r['summary']['read']} lectures candidates.", '',
           '**Attention : aucun lien entre deux salles n’est authentifié par ce balayage.**','',
           '| Offset ROM | Banque | Instruction candidate | Variable | Accès |', '|---|---:|---|---|---|']
    for x in r['candidates']:
        lines.append(f"| `{x['file_offset']}` | {x['bank']} | `{x['candidate_opcode']}` | `{x['variable']}` | {x['candidate_access']} |")
    lines+=['','## Suite de l’analyse','',
            '1. Désassembler les banques et suivre les appels vers le chargeur `4F83`.',
            '2. Vérifier les écritures exécutées à `D000/D001` avec état du mapper.',
            '3. Capturer le couple (stage,salle) avant/après une porte et relever la position de Simon.',
            '4. Activer uniquement les liaisons confirmées dans le gestionnaire de salles.','',
            'Les transitions automatiques V27 restent des liaisons de démonstration.']
    (args.out/'Z80_ROOM_STATE_AUDIT.md').write_text('\n'.join(lines)+'\n')
    print(r['summary'])
if __name__=='__main__':main()
