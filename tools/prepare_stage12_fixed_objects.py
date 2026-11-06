#!/usr/bin/env python3
"""Generate stage-12 fixed objects from archived 5A49/5B1B Z80 output.

Validate layout against all 71 preexisting C definitions from stages 0..11.
Type 18h (white key) has a separate engine pathway and is deliberately not
converted into VKCandleDef. All non-83h records are left uninterpreted.
"""
import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / 'references/castle_objects_z80.json'
OLD = ROOT / 'sgdk/src/generated/candles_md.c'
HEADER = ROOT / 'sgdk/src/generated/stage12_fixed_objects.h'
SOURCE = ROOT / 'sgdk/src/generated/stage12_fixed_objects.c'


def decode(item):
    raw = bytes.fromhex(item['raw'])
    assert len(raw) == 16 and raw[0] == 0x83
    assert raw[4] in (0x17, 0x18, 0x19), f'Unclassified fixed object {raw[4]:02x}'
    return (raw[2], raw[1], 0, raw[4], raw[13])


def validated():
    text = OLD.read_text().split('const VKCandleDef vk_fixed_object_defs[71] = {', 1)[1].split('};', 1)[0]
    pat = re.compile(r'\{\s*(\d+),\s*(\d+),\s*(\d+),\s*0x([\da-fA-F]+),\s*0x([\da-fA-F]+)\s*\},\s*/\* stage (\d+) room (\d+) \*/')
    existing = defaultdict(list)
    for m in pat.finditer(text):
        x,y,mode,kind,content,stage,room = m.groups()
        existing[int(stage),int(room)].append((int(x),int(y),int(mode),int(kind,16),int(content,16)))
    records = json.loads(REF.read_text())
    earlier = defaultdict(list)
    stage12 = []
    for row in records:
        vals = []
        for obj in row['other_objects']:
            raw = bytes.fromhex(obj['raw'])
            if raw[0] != 0x83:
                continue
            v = decode(obj)
            # Type 18h (white key) is managed by items.c, not candles.c.
            if v[3] == 0x18:
                continue
            vals.append(v)
        if row['stage'] <= 11:
            earlier[row['stage'],row['room']].extend(vals)
        if row['stage'] == 12:
            stage12.append((row['room'],vals))
    assert sum(map(len,existing.values())) == 71
    assert sum(map(len,earlier.values())) == 71
    for key in set(existing) | set(earlier):
        assert sorted(existing[key]) == sorted(earlier[key]), f'Z80 fixed object mismatch: {key}'
    assert sorted(room for room,_ in stage12) == list(range(12))
    stage12.sort()
    assert sum(len(v) for _,v in stage12) == 5
    return stage12


def output():
    rows = validated()
    starts = [0]
    defs = []
    for room,vals in rows:
        defs.extend((room,v) for v in vals)
        starts.append(len(defs))
    h = '''/* Fixed objects from original Z80 5A49/5B1B output, stage 12 only.
 * Byte layout checked against 71 earlier objects. Not wired to gameplay. */
#ifndef VK_STAGE12_FIXED_OBJECTS_H
#define VK_STAGE12_FIXED_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE12_FIXED_OBJECT_COUNT 5
extern const VKCandleDef vk_stage12_fixed_object_defs[VK_STAGE12_FIXED_OBJECT_COUNT];
extern const VKByte vk_stage12_fixed_object_first[13];
#endif
'''
    c = '''/* Z80 fields: X=+2, Y=+1, type=+4, chest content=+13.
 * All earlier 71 VKCandleDef fixed objects match. */
#include "stage12_fixed_objects.h"
const VKCandleDef vk_stage12_fixed_object_defs[VK_STAGE12_FIXED_OBJECT_COUNT] = {\n'''
    for room,(x,y,mode,kind,content) in defs:
        c += f'    {{ {x}, {y}, {mode}, 0x{kind:02X}, 0x{content:02X} }}, /* room {room} */\n'
    c += '};\nconst VKByte vk_stage12_fixed_object_first[13] = {' + ', '.join(map(str, starts)) + '};\n'
    return {HEADER:h,SOURCE:c}


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    for path,contents in output().items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit('MISMATCH '+str(path))
        else:
            path.write_text(contents)
    print('Stage 12: 5 fixed objects; 71 historic Z80 fixed objects match: PASS')
