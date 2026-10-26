#!/usr/bin/env python3
"""Conservative contextual triage of D000/D001 writes, no runtime claims."""
from pathlib import Path
import argparse, json
from probe import load_rom
from audit_room_state_z80 import scan

# Classifications based on the surrounding opcode stream. These are evidence
# grades, NOT proof of reachable transitions or decoded destination edges.
REVIEWS = {
    0x00332: ('stage_write_candidate', 'conditional branch; LD (D000),A writes FF following CP 06; no room destination established', 'medium'),
    0x00DFA: ('initialization', 'XOR A followed by a series of zero stores to C007/CF3D/D001/D002, then INC A and store D000', 'high'),
    0x00E07: ('initialization', 'same contiguous initialization block as 00DFA: stage reset to 1', 'high'),
    0x01E3D: ('stage_select_candidate', 'CP 13, JR C, INC/zero A then LD (D000),A; stage boundary clamp candidate', 'medium'),
    0x01E59: ('room_reset_candidate', 'LD A,(D000), indexed read through 4061, LD (D002),A then XOR A and LD (D001),A', 'high'),
    0x030D6: ('room_reset_candidate', 'XOR A and repeated store-zero sequence including (D001)', 'high'),
    0x1B987: ('room_selection_candidate', 'AND 0F / CP 0F / RET Z precede LD (D001),A; carry set before RET', 'medium'),
    0x1CE2B: ('table_false_positive_likely', 'region consists of patterned pointer/data words, 22 01 D0 overlaps table entries', 'low'),
    0x1D0E6: ('table_false_positive_likely', 'region contains repeated patterned nibble values and 22 01 D0, not verified code', 'low'),
}

def classify(data):
    writes=[c for c in scan(data) if c['candidate_access']=='write']
    seen={int(c['file_offset'],16) for c in writes}
    if seen != set(REVIEWS):
        raise ValueError(f'Review table out of sync; missing={seen-set(REVIEWS)}, stale={set(REVIEWS)-seen}')
    result=[]
    for c in writes:
        offset=int(c['file_offset'],16)
        role,note,confidence=REVIEWS[offset]
        result.append({**c,'triage':role,'context_confidence':confidence,'rationale':note,
                       'candidate_cpu_address_if_mapped':[
                           f'{window+(offset%0x2000):04X}'
                           for window in ([0x4000] if offset < 0x2000 else [0x6000,0x8000,0xA000])],
                       'transition_edge_confirmed':False,
                       'dynamic_trace_available':False})
    return {'method':'manual contextual triage with original ROM byte checks',
            'writes':result,'verified_edges':[],
            'caution':'Likely code is not proof of execution, destination or mapper bank. Debug-only navigation must stay separate.'}

def main():
    p=argparse.ArgumentParser(); p.add_argument('rom',type=Path)
    p.add_argument('--out',type=Path,default=Path('research/castle-navigation'))
    a=p.parse_args();report=classify(load_rom(a.rom));a.out.mkdir(parents=True,exist_ok=True)
    (a.out/'z80-room-write-triage.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    lines=['# Z80 — tri contextuel des écritures de stage/salle','',
           'Ce rapport va au-delà du balayage binaire en examinant les instructions voisines.',
           '**Aucune destination de salle n’est pour autant authentifiée.**','',
           '| Offset ROM | Type probable | Confiance du contexte | Raisonnement |', '|---|---|---|---|']
    for row in report['writes']:
        lines.append(f"| `{row['file_offset']}` | `{row['triage']}` | {row['context_confidence']} | {row['rationale']} |")
    lines.extend(['','## Conclusion pratique','',
       '- `00DFA`, `00E07`, `01E59`, `030D6` correspondent à des séquences de réinitialisation plausibles, et non à des arêtes prouvées.',
       '- `1B987` est la piste la plus utile pour une sélection effective de salle, mais demande le suivi du flot et du mapper.',
       '- `1CE2B`, `1D0E6` doivent rester exclus des écritures fiables tant que ces plages de données ne sont pas prouvées exécutables.',
       '- Ne pas remplacer les connexions expérimentales par ces hypothèses.','',
       '## Capture dynamique requise','',
       'Tracer `D000`, `D001`, PC et les banques actives sur chaque écriture ; relever l’état précédent, le suivant, ainsi que la position du personnage.',
       'Vérifier les effets de portes et escaliers en jeu avant d’inscrire un lien dans le graphe.',''])
    (a.out/'Z80_ROOM_WRITE_TRIAGE.md').write_text('\n'.join(lines))
    print(f"Triaged {len(report['writes'])} writes; {len(report['verified_edges'])} authenticated transitions")
if __name__=='__main__':main()
