#!/usr/bin/env python3
"""Generate NON-RUNTIME stage 17-18 room records, definitions and isolated state.

Inputs: archived 5A49/5B1B Z80 slot snapshots in references/castle_objects_z80.json.
Field mapping verified using existing 275 candle + 71 fixed object definitions.
Tiles: static 4F83-normal-path decoder verified by 79 prior ROM/Z80 maps;
this is NOT execution of enemy/interaction routines.
No runtime progression activation, no inferred object at zero-type slot.
"""
import argparse
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path
from prepare_stage12_candles import verified_reference, definition
from prepare_stage12_fixed_objects import validated, decode

ROOT=Path(__file__).resolve().parents[1]
GEN=ROOT/'sgdk/src/generated'
REF=ROOT/'references/castle_objects_z80.json'

def write(path, text, check):
    if check:
        if not path.is_file() or path.read_text()!=text: raise AssertionError('Generator mismatch: '+str(path))
    else:
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(text)

def render(stage, rows):
    count=12 if stage==17 else 10
    assert [r['room'] for r in rows]==list(range(count))
    candle=[]; fixed=[]; keys=[]; slots=[]; cs=[0]; fs=[0]; ks=[0]; starts=[0]; unknown=[]
    for row in rows:
        room=row['room']
        seen=set()
        for cat in ('candles','other_objects'):
            for item in row[cat]:
                raw=bytes.fromhex(item['raw'])
                slot=int(item['slot'],16)
                assert len(raw)==16 and slot not in seen and slot%16==0 and 0xC420<=slot<=0xC5F0
                assert (raw[0],raw[1],raw[2],raw[5])==(item['type'],item['y'],item['x'],item['kind'])
                seen.add(slot)
                slots.append((room,slot,raw,cat))
                if cat=='candles' and raw[0]==1:
                    candle.append((room,definition(item)))
                elif raw[0]==0x83:
                    obj=decode(item)
                    # White key follows separate item path; do not assert that fixed object is collectable here.
                    if obj[3]==0x18: keys.append((room,obj))
                    else: fixed.append((room,obj))
                else: unknown.append((room,slot,'unclassified original slot'))
        cs.append(len(candle));fs.append(len(fixed));ks.append(len(keys));starts.append(len(slots))
    # Do not impose guessed totals; verify the original records and the generated outputs.




    hdr=f'''/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage {stage} DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE{stage}_OBJECTS_H
#define VK_STAGE{stage}_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE{stage}_ROOMS {count}
#define VK_STAGE{stage}_CANDLES {len(candle)}
#define VK_STAGE{stage}_FIXED {len(fixed)}
#define VK_STAGE{stage}_WHITE_KEYS {len(keys)}
#define VK_STAGE{stage}_RAW {len(slots)}
typedef struct {{ unsigned short slot; VKByte raw[16]; }} VKStage{stage}RawSlot;
extern const VKCandleDef vk_stage{stage}_candles[VK_STAGE{stage}_CANDLES];
extern const VKCandleDef vk_stage{stage}_fixed[VK_STAGE{stage}_FIXED];
extern const VKCandleDef vk_stage{stage}_white_keys[VK_STAGE{stage}_WHITE_KEYS];
extern const VKStage{stage}RawSlot vk_stage{stage}_raw[VK_STAGE{stage}_RAW];
extern const VKByte vk_stage{stage}_candle_first[VK_STAGE{stage}_ROOMS+1];
extern const VKByte vk_stage{stage}_fixed_first[VK_STAGE{stage}_ROOMS+1];
extern const VKByte vk_stage{stage}_white_key_first[VK_STAGE{stage}_ROOMS+1];
extern const VKByte vk_stage{stage}_raw_first[VK_STAGE{stage}_ROOMS+1];
#endif
'''
    c=f'/* Literal Z80 slot record data, stage {stage}. See LIVRAISON for provenance. */\n#include "stage{stage}_objects.h"\n'
    for kind,values,starts_vec in [('candles',candle,cs),('fixed',fixed,fs),('white_keys',keys,ks)]:
        c+=f'const VKCandleDef vk_stage{stage}_{kind}[VK_STAGE{stage}_{"CANDLES" if kind=="candles" else ("FIXED" if kind=="fixed" else "WHITE_KEYS")}] = {{\n'
        for room,fields in values:
            x,y,mode,typ,content=fields
            c+=f'    {{{x}, {y}, {mode}, 0x{typ:02X}, 0x{content:02X}}}, /* room {room} */\n'
        c+='};\n'
        c+=f'const VKByte vk_stage{stage}_{"candle" if kind=="candles" else ("fixed" if kind=="fixed" else "white_key")}_first[VK_STAGE{stage}_ROOMS+1] = {{{", ".join(map(str,starts_vec))}}};\n'
    c+=f'const VKStage{stage}RawSlot vk_stage{stage}_raw[VK_STAGE{stage}_RAW] = {{\n'
    for room,slot,raw,cat in slots:
        c+=f'    {{0x{slot:04X}, {{{", ".join("0x%02X"%v for v in raw)}}}}}, /* room {room}, {cat} */\n'
    c+='};\n'
    c+=f'const VKByte vk_stage{stage}_raw_first[VK_STAGE{stage}_ROOMS+1] = {{{", ".join(map(str,starts))}}};\n'
    scene_h=f'''/* Independent room-state facade for stage {stage} only; not SGDK progression. */
#ifndef VK_STAGE{stage}_SCENE_H
#define VK_STAGE{stage}_SCENE_H
#include "generated/stage{stage}_foundations.h"
#include "generated/stage{stage}_objects.h"
typedef struct {{ VKByte candles[VK_STAGE{stage}_ROOMS]; VKByte fixed[VK_STAGE{stage}_ROOMS]; }} VKStage{stage}Scene;
void vk_stage{stage}_scene_reset(VKStage{stage}Scene *s);
int vk_stage{stage}_scene_reset_room(VKStage{stage}Scene *s, unsigned room);
unsigned vk_stage{stage}_scene_candle_count(unsigned room);
unsigned vk_stage{stage}_scene_fixed_count(unsigned room);
unsigned vk_stage{stage}_scene_white_key_count(unsigned room);
unsigned vk_stage{stage}_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage{stage}_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage{stage}_scene_fixed(unsigned room, unsigned index);
const VKCandleDef *vk_stage{stage}_scene_white_key(unsigned room, unsigned index);
const VKStage{stage}RawSlot *vk_stage{stage}_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage{stage}_scene_map(unsigned room);
const VKRoomLink *vk_stage{stage}_scene_link(unsigned room);
int vk_stage{stage}_scene_present(const VKStage{stage}Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage{stage}_scene_mark(VKStage{stage}Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
'''
    prefix=f'vk_stage{stage}_'
    scene_c=f'''/* Stage {stage} room data access/persistence; never performs a gameplay action. */
#include "stage{stage}_scene.h"
void {prefix}scene_reset(VKStage{stage}Scene *s) {{ unsigned r; if (!s) return; for(r=0;r<VK_STAGE{stage}_ROOMS;r++) {{s->candles[r]=0;s->fixed[r]=0;}} }}
int {prefix}scene_reset_room(VKStage{stage}Scene *s,unsigned r) {{if(!s||r>=VK_STAGE{stage}_ROOMS)return 0;s->candles[r]=0;s->fixed[r]=0;return 1;}}
unsigned {prefix}scene_candle_count(unsigned r) {{ return r<VK_STAGE{stage}_ROOMS ? vk_stage{stage}_candle_first[r+1]-vk_stage{stage}_candle_first[r]:0; }}
unsigned {prefix}scene_fixed_count(unsigned r) {{ return r<VK_STAGE{stage}_ROOMS ? vk_stage{stage}_fixed_first[r+1]-vk_stage{stage}_fixed_first[r]:0; }}
unsigned {prefix}scene_white_key_count(unsigned r) {{ return r<VK_STAGE{stage}_ROOMS ? vk_stage{stage}_white_key_first[r+1]-vk_stage{stage}_white_key_first[r]:0; }}
unsigned {prefix}scene_raw_count(unsigned r) {{ return r<VK_STAGE{stage}_ROOMS ? vk_stage{stage}_raw_first[r+1]-vk_stage{stage}_raw_first[r]:0; }}
const VKCandleDef *{prefix}scene_candle(unsigned r,unsigned i) {{return i<{prefix}scene_candle_count(r)?&vk_stage{stage}_candles[vk_stage{stage}_candle_first[r]+i]:0;}}
const VKCandleDef *{prefix}scene_fixed(unsigned r,unsigned i) {{return i<{prefix}scene_fixed_count(r)?&vk_stage{stage}_fixed[vk_stage{stage}_fixed_first[r]+i]:0;}}
const VKCandleDef *{prefix}scene_white_key(unsigned r,unsigned i) {{return i<{prefix}scene_white_key_count(r)?&vk_stage{stage}_white_keys[vk_stage{stage}_white_key_first[r]+i]:0;}}
const VKStage{stage}RawSlot *{prefix}scene_raw(unsigned r,unsigned i) {{return i<{prefix}scene_raw_count(r)?&vk_stage{stage}_raw[vk_stage{stage}_raw_first[r]+i]:0;}}
const VKByte *{prefix}scene_map(unsigned r) {{return r<VK_STAGE{stage}_ROOMS?vk_stage{stage}_tiles[r]:0;}}
const VKRoomLink *{prefix}scene_link(unsigned r) {{return r<VK_STAGE{stage}_ROOMS?&vk_stage{stage}_raw_links[r]:0;}}
int {prefix}scene_present(const VKStage{stage}Scene *s,unsigned r,unsigned i,int fix) {{ unsigned n=fix?{prefix}scene_fixed_count(r):{prefix}scene_candle_count(r);if(!s||i>=n)return 0;return !((fix?s->fixed[r]:s->candles[r])&(1u<<i)); }}
int {prefix}scene_mark(VKStage{stage}Scene *s,unsigned r,unsigned i,int fix) {{ unsigned n=fix?{prefix}scene_fixed_count(r):{prefix}scene_candle_count(r);if(!s||i>=n)return 0;if(fix)s->fixed[r]|=(VKByte)(1u<<i);else s->candles[r]|=(VKByte)(1u<<i);return 1; }}
'''
    test=f'''#include <assert.h>
#include <stdio.h>
#include "stage{stage}_scene.h"
int main(void) {{
 VKStage{stage}Scene scene;
 unsigned room,i,nc=0,nf=0,nk=0,nr=0;
 vk_stage{stage}_scene_reset(&scene);
 for(room=0;room<VK_STAGE{stage}_ROOMS;room++) {{
   unsigned c=vk_stage{stage}_scene_candle_count(room),f=vk_stage{stage}_scene_fixed_count(room),k=vk_stage{stage}_scene_white_key_count(room),r=vk_stage{stage}_scene_raw_count(room);
   assert(vk_stage{stage}_scene_link(room)==&vk_stage{stage}_raw_links[room]);
   assert(vk_stage{stage}_scene_map(room)==vk_stage{stage}_tiles[room]);
   assert(c<=8&&f<=8);
   nc+=c;nf+=f;nk+=k;nr+=r;
   for(i=0;i<c;i++) {{assert(vk_stage{stage}_scene_candle(room,i));assert(vk_stage{stage}_scene_present(&scene,room,i,0));assert(vk_stage{stage}_scene_mark(&scene,room,i,0));assert(!vk_stage{stage}_scene_present(&scene,room,i,0));}}
   for(i=0;i<f;i++) {{assert(vk_stage{stage}_scene_fixed(room,i));assert(vk_stage{stage}_scene_present(&scene,room,i,1));assert(vk_stage{stage}_scene_mark(&scene,room,i,1));assert(!vk_stage{stage}_scene_present(&scene,room,i,1));}}
   for(i=0;i<k;i++) {{const VKCandleDef *key=vk_stage{stage}_scene_white_key(room,i);assert(key && key->kind==0x18);}}
   for(i=0;i<r;i++) assert(vk_stage{stage}_scene_raw(room,i));
   assert(!vk_stage{stage}_scene_candle(room,c));assert(!vk_stage{stage}_scene_fixed(room,f));assert(!vk_stage{stage}_scene_raw(room,r));
   assert(!vk_stage{stage}_scene_mark(&scene,room,c,0));assert(!vk_stage{stage}_scene_mark(&scene,room,f,1));
 }}
 assert(nc=={len(candle)} && nf=={len(fixed)} && nk=={len(keys)} && nr=={len(slots)});
 assert(!vk_stage{stage}_scene_white_key(VK_STAGE{stage}_ROOMS,0));
 assert(!vk_stage{stage}_scene_map(VK_STAGE{stage}_ROOMS));
 assert(!vk_stage{stage}_scene_link(VK_STAGE{stage}_ROOMS));
 assert(!vk_stage{stage}_scene_reset_room(&scene,VK_STAGE{stage}_ROOMS));
 assert(!vk_stage{stage}_scene_mark(&scene,VK_STAGE{stage}_ROOMS,0,0));
 assert(vk_stage{stage}_scene_reset_room(&scene,VK_STAGE{stage}_ROOMS-1));
 vk_stage{stage}_scene_reset(&scene);
 for(room=0;room<VK_STAGE{stage}_ROOMS;room++) if(vk_stage{stage}_scene_candle_count(room)) assert(vk_stage{stage}_scene_present(&scene,room,0,0));
 puts("stage{stage} scene/object test: PASS");return 0;
}}
'''
    runner=f'''#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage{stage}_objects.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${{CC:-cc}} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Isgdk/src/generated \\
 tests/test_stage{stage}_scene.c sgdk/src/stage{stage}_scene.c \\
 sgdk/src/generated/stage{stage}_objects.c sgdk/src/generated/stage{stage}_foundations.c -o "$BIN"
"$BIN"
'''
    report={'stage':stage,'room_count':count,'source':'VK-gpt.zip scratchpad/castle-candles/work/tool.json, archived 5A49+5B1B execution','candle_count':len(candle),'fixed_count':len(fixed),'white_keys':len(keys),'raw_slots':len(slots),'unknown_slots':[{'room':r,'slot':f'0x{slot:04X}','note':note} for r,slot,note in unknown], 'raw_sha256':hashlib.sha256(b''.join(r[2] for r in slots)).hexdigest(),'candle_offsets':'x=+2 y=+1 mode=+4 kind=+5 content=+9 (275 historical matches)','fixed_offsets':'x=+2 y=+1 kind=+4 content=+13 (71 historical matches)', 'white_key_source':'type 83h, kind 18h; read-only item-path data','collision':'ROM normal-path tile decoder verified for stages 1-11; same expansion used for stage map; not dynamic collision simulation','limits':['no enemy type/AI extraction','no real gameplay scene activation','white keys are read-only definitions; not wired to item pickup','no engine object interactions','no compiled SGDK ROM or visual/sonic verification']}
    return {GEN/f'stage{stage}_objects.h':hdr,GEN/f'stage{stage}_objects.c':c,ROOT/f'sgdk/src/stage{stage}_scene.h':scene_h,ROOT/f'sgdk/src/stage{stage}_scene.c':scene_c,ROOT/f'tests/test_stage{stage}_scene.c':test,ROOT/f'tests/run_stage{stage}_scene.sh':runner,ROOT/f'research/stage{stage}-gameplay-audit.json':json.dumps(report,indent=2,ensure_ascii=False)+'\n'},report

def main(check):
    rows=json.loads(REF.read_text())
    assert verified_reference(rows)==275
    assert sum(len(v) for _,v in validated())==5 # stage12 precedent also validates 71 earlier
    for stage in (17,18):
        stage_rows=sorted((r for r in rows if r['stage']==stage),key=lambda r:r['room'])
        outputs,report=render(stage,stage_rows)
        for path,s in outputs.items():write(path,s,check)
        if not check:(ROOT/f'tests/run_stage{stage}_scene.sh').chmod(0o755)
        print('stage',stage,'candles',report['candle_count'],'fixed',report['fixed_count'],'raw',report['raw_slots'],'unknown',len(report['unknown_slots']))
    print('275+71 historical layout references: PASS')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args();main(a.check)
