#!/usr/bin/env python3
"""Convert original Z80 candle slots stage 12 into proven VKCandleDef fields.

Proof: identical byte offsets reproduce 275 pre-existing stage 0-11 definitions.
No conversion of other object slots. No SGDK gameplay activation.
"""
import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT/'references/castle_objects_z80.json'
PREVIOUS = ROOT/'sgdk/src/generated/candles_md.c'
HEADER = ROOT/'sgdk/src/generated/stage12_candles.h'
SOURCE = ROOT/'sgdk/src/generated/stage12_candles.c'


def definition(item):
    raw=bytes.fromhex(item['raw'])
    assert len(raw)==16 and raw[0]==1
    assert (raw[1],raw[2],raw[5])==(item['y'],item['x'],item['kind'])
    return (raw[2],raw[1],raw[4],raw[5],raw[9])


def verified_reference(records):
    original=re.split(r'const VKCandleDef vk_candle_defs\[\d+\] = \{',PREVIOUS.read_text(),maxsplit=1)[1].split('};',1)[0]
    matcher=re.compile(r'\{\s*(\d+),\s*(\d+),\s*(\d+),\s*0x([a-fA-F0-9]+),\s*0x([a-fA-F0-9]+)\s*\},\s*/\* stage (\d+) room (\d+) \*/')
    existing=defaultdict(list)
    for m in matcher.finditer(original):
        x,y,mode,kind,content,stage,room=m.groups()
        if int(stage)>11:continue
        existing[int(stage),int(room)].append((int(x),int(y),int(mode),int(kind,16),int(content,16)))
    expected=defaultdict(list)
    for row in records:
        if row['stage']>11: continue
        expected[row['stage'],row['room']].extend(definition(item) for item in row['candles'])
    assert set(existing)==set(k for k,v in expected.items() if v), 'reference room coverage differs'
    for key, values in expected.items():
        if sorted(values)!=sorted(existing.get(key, [])):
            raise ValueError(f'old stage reference mismatch: {key}')
    count=sum(len(v) for v in expected.values())
    assert count==275, f'expected 275 precedents, found {count}'
    return count


def outputs():
    records=json.loads(REF.read_text())
    precedent=verified_reference(records)
    rows=sorted((r for r in records if r['stage']==12),key=lambda r:r['room'])
    assert [r['room'] for r in rows]==list(range(12))
    definitions=[]; starts=[0]
    for row in rows:
        definitions.extend((row['room'],definition(item)) for item in row['candles'])
        starts.append(len(definitions))
    assert len(definitions)==30
    h='''/* Stage 12 candle definitions from archived Z80 execution 5A49/5B1B.
 * Byte offsets validated against 275 preceding definitions (stages 0..11).
 * NO stage 12 gameplay hook; non-candle slots are not represented here. */
#ifndef VK_STAGE12_CANDLES_H
#define VK_STAGE12_CANDLES_H
#include "candles_md.h"
#define VK_STAGE12_CANDLE_COUNT 30
extern const VKCandleDef vk_stage12_candle_defs[VK_STAGE12_CANDLE_COUNT];
extern const VKByte vk_stage12_candle_first[13];
#endif
'''
    c='''/* Literal Z80 candle fields: X=+2, Y=+1, mode=+4, kind=+5, content=+9.
 * Exact field mapping checked against all 275 existing stage 0..11 candles. */
#include "stage12_candles.h"
const VKCandleDef vk_stage12_candle_defs[VK_STAGE12_CANDLE_COUNT] = {\n'''
    for room, (x,y,mode,kind,content) in definitions:
        c+=f'    {{ {x}, {y}, {mode}, 0x{kind:02X}, 0x{content:02X} }}, /* room {room} */\n'
    c+='};\nconst VKByte vk_stage12_candle_first[13] = {'+', '.join(map(str,starts))+'};\n'
    return {HEADER:h,SOURCE:c}, precedent

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    rendered,count=outputs()
    for p,s in rendered.items():
        if args.check:
            if not p.exists() or p.read_text()!=s:raise SystemExit('MISMATCH '+str(p))
        else: p.write_text(s)
    print(f'Stage 12: 30 candle defs; byte-field interpretation agrees with {count} old Z80 candle defs: PASS')
