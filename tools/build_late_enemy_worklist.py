#!/usr/bin/env python3
"""Build an exact per-room list of late fixed enemy records still without AI.

Reads the archived output of ROM routine 6154 and the verified dispatch table.
No assumptions about the meaning of enemy type, initialization or behavior.
Run: python tools/build_late_enemy_worklist.py [--check]
"""
import argparse
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'research/late-fixed-enemies-z80.json'
DISPATCH = ROOT / 'research/late-enemy-update-dispatch.json'
OUT = ROOT / 'research/late-enemy-unported-worklist.json'
MD = ROOT / 'research/late-enemy-unported-worklist.md'
PORTABLE = {0x0B, 0x0D, 0x0E}
EXPECTED = {12:0, 13:9, 14:15, 15:10, 16:6, 17:4, 18:3}


def build():
    raw = json.loads(SOURCE.read_text())
    dispatch = json.loads(DISPATCH.read_text())
    assert raw['rom_sha256'] == dispatch['rom_sha256']
    addresses = {int(x['type'],16):int(x['update_cpu'],16) for x in dispatch['objects']}
    assert set(addresses) == {9, 11, 13, 14, 16, 18}
    unresolved=[];counts={};implemented=0
    for stage in raw['stages']:
        number=stage['stage']
        counts[number]=0
        for room_index, room in enumerate(stage['rooms']):
            for item in room:
                kind=item['raw_type'] & 0x7f
                if not kind: continue
                assert kind in addresses, (number, room_index, kind)
                if kind in PORTABLE:
                    implemented+=1
                    continue
                assert item['x']==(item['raw'][1]&15)*16
                assert item['y']==item['raw'][1]&0xf0
                unresolved.append({
                    'stage':number,'room':room_index,'slot':item['slot'],
                    'raw':item['raw'],'type':f'{kind:02X}',
                    'handler_cpu':f'{addresses[kind]:04X}',
                    'x':item['x'],'y':item['y'],
                    'identifier':item['id'],
                    'status':'NON_PORTE',
                })
                counts[number]+=1
    assert counts==EXPECTED, counts
    assert implemented==46, implemented
    assert len(unresolved)==47, len(unresolved)
    assert Counter(x['type'] for x in unresolved)=={'09':13,'10':28,'12':6}, Counter(x['type'] for x in unresolved)
    payload={'rom_sha256':raw['rom_sha256'],'source_routine':'6154',
             'dispatch_table':'9942','implemented_existing':implemented,
             'unsupported':len(unresolved),'per_stage':{str(k):v for k,v in counts.items()},
             'records':unresolved,
             'note':'Positions and dispatch proven by ROM; no behavior inferred or implemented.'}
    lines=['# 47 ennemis fixes non portés — stages 12 à 18','',
        'Généré depuis `research/late-fixed-enemies-z80.json` (routine 6154) et',
        '`research/late-enemy-update-dispatch.json` (table 9942).',
        '**Cette liste ne valide ni les animations, ni les dégâts, ni les comportements.**','',
        '| Stage | Salle | Slot | Type | Routine Z80 | X | Y | ID |',
        '|---:|---:|---:|---|---|---:|---:|---:|']
    for x in unresolved:
        lines.append(f"| {x['stage']} | {x['room']} | {x['slot']} | `{x['type']}h` | `{x['handler_cpu']}` | {x['x']} | {x['y']} | {x['identifier']} |")
    lines += ['',f"Total : **{len(unresolved)}** entrées sans IA portées. Routines `AF5F` (13 entrées), `ADF8` (28) et `BE70` (6).",'']
    return json.dumps(payload,ensure_ascii=False,indent=2)+'\n', '\n'.join(lines)

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    data,md=build()
    if args.check:
        assert OUT.read_text()==data
        assert MD.read_text()==md
    else:
        OUT.write_text(data)
        MD.write_text(md)
    print('47 remaining fixed enemies: stage and ROM-dispatch cross-check PASS')
