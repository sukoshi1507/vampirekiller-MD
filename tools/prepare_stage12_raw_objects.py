#!/usr/bin/env python3
"""Export literal stage 12 objects from archived original Z80 CPU/mapper traces.

No runtime interpretation of the 16-byte records is attempted.
Usage: python tools/prepare_stage12_raw_objects.py INPUT_Z80_JSON [--check]
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
HEADER=ROOT/'sgdk/src/generated/stage12_object_records.h'
SOURCE=ROOT/'sgdk/src/generated/stage12_object_records.c'
REPORT=ROOT/'research/stage12-object-records.json'

def render(src):
    stage=[v for v in src if v.get('stage')==12]
    if sorted(v['room'] for v in stage)!=list(range(12)):
        raise ValueError('expected 12 unique rooms')
    stage.sort(key=lambda x:x['room'])
    rows=[]; bounds=[0]; room_list=[]
    for room in stage:
        slots=set()
        for cat in ('candles','other_objects'):
            for item in room[cat]:
                raw=bytes.fromhex(item['raw'])
                slot=int(item['slot'],16)
                if len(raw)!=16 or slot in slots or not (0xC420<=slot<=0xC5F0) or (slot & 0xF):
                    raise ValueError('invalid record in room '+str(room['room']))
                slots.add(slot)
                if (raw[0],raw[1],raw[2],raw[5]) != (item['type'],item['y'],item['x'],item['kind']):
                    raise ValueError('record metadata mismatch')
                rows.append({'room':room['room'],'slot':slot,'raw':raw.hex(),'category':cat})
        bounds.append(len(rows))
        room_list.append({'room':room['room'],'count':len(slots),'slots':sorted(slots)})
    c_head='''/* Literal Z80 output (C420..C5FF) recorded in VK-gpt.zip. NOT interpreted or wired to SGDK. */
#ifndef VK_STAGE12_OBJECT_RECORDS_H
#define VK_STAGE12_OBJECT_RECORDS_H
#include "../room_manager.h"
#define VK_STAGE12_RAW_OBJECT_COUNT %d
#define VK_STAGE12_RAW_ROOM_COUNT 12
/* slot is the original Z80 object-slot address; raw is the unchanged 16-byte slot. */
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage12RawObject;
extern const VKStage12RawObject vk_stage12_raw_objects[VK_STAGE12_RAW_OBJECT_COUNT];
extern const VKByte vk_stage12_raw_object_start[13];
#endif
''' % len(rows)
    c_src='''/* Original 16-byte slots from archived Z80 execution. Data only; no semantic inference. */
#include "stage12_object_records.h"
const VKStage12RawObject vk_stage12_raw_objects[VK_STAGE12_RAW_OBJECT_COUNT] = {
'''
    for r in rows:
        v=bytes.fromhex(r['raw'])
        c_src+='    {0x%04X, {%s}}, /* room %d, %s */\n' % (r['slot'],', '.join('0x%02X'%b for b in v),r['room'],r['category'])
    c_src+='};\nconst VKByte vk_stage12_raw_object_start[13] = {'+', '.join(str(x) for x in bounds)+'};\n'
    report={'provenance':'VK-gpt.zip: scratchpad/castle-candles/work/tool.json; tools/castle_room_objects.py Z80 runtime 5A49 + 5B1B','scope':'Original slot bytes only; no gameplay hooks or item/drop inference','stage':12,'room_count':12,'record_count':len(rows),'candle_records':sum(x['category']=='candles' for x in rows),'other_records':sum(x['category']=='other_objects' for x in rows),'raw_sha256':hashlib.sha256(b''.join(bytes.fromhex(r['raw']) for r in rows)).hexdigest(),'records':rows,'rooms':room_list,'noninterpreted_nonzero_slots':[{'room':r['room'],'slot':r['slot'],'raw':r['raw']} for r in rows if bytes.fromhex(r['raw'])[0]==0]}
    return ((HEADER,c_head),(SOURCE,c_src),(REPORT,json.dumps(report,indent=2,ensure_ascii=False)+'\n'))

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('reference');a.add_argument('--check',action='store_true');args=a.parse_args()
    outputs=render(json.loads(Path(args.reference).read_text()))
    for path,data in outputs:
        if args.check:
            if not path.exists() or path.read_text()!=data: raise SystemExit('MISMATCH '+str(path))
        else: path.write_text(data)
    print('stage12 raw slots:',len(json.loads(REPORT.read_text())['records']),'PASS')
