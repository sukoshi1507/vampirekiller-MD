#!/usr/bin/env python3
"""Prepare stage-12 collision bytes and room-object positions using documented evidence.

No Z80 emulation. Collision: opcode-equivalent normal path of 4F83/4FAF,
validated against all 79 previously Z80-derived maps (stages 1-11).
Object positions: historical CPU+mapper harness results recorded in
research/castle-candles.md; object types/contents are NOT recorded there.
Never assert that this establishes room runtime, drops or boss behavior.
Usage: python tools/prepare_stage12_gameplay_data.py /path/to/reference.rom
"""
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from probe import load_rom
from extract_rooms import decode_room

ROOMS = 12
COLLISION_SIZE = 768

def former_stage_map(path):
    text = path.read_text().split('=', 1)[1].split('};', 1)[0]
    return bytes(int(n) for n in re.findall(r'\b\d+\b', text))


def check_precedents(rom):
    compared = 0
    summaries = []
    for stage in range(1, 12):
        filename = f'stage{stage}_' + ('collision' if stage == 1 else 'maps') + '.c'
        previous = former_stage_map(ROOT / 'sgdk/src/generated' / filename)
        if len(previous) % COLLISION_SIZE:
            raise ValueError('Unexpected source length: ' + filename)
        count = len(previous)//COLLISION_SIZE
        generated = b''.join(bytes(decode_room(rom, stage, i)[2]) for i in range(count))
        if generated != previous:
            index = next(i for i, (a, b) in enumerate(zip(generated, previous)) if a != b)
            raise AssertionError(f'Collision translation mismatch stage {stage} byte {index}')
        compared += len(previous)
        summaries.append({'stage':stage, 'rooms': count, 'bytes_equal':len(previous)})
    return compared, summaries


def historical_positions():
    text=(ROOT / 'research/castle-candles.md').read_text()
    entries={}
    for line in text.splitlines():
        m=re.match(r'^\| 12 \(chateau 12\) \| (\d+) \| ([^|]+) \| ([^|]+) \|',line)
        if not m:
            continue
        room=int(m.group(1))
        if room in entries:
            raise ValueError('Duplicate room ' + str(room))
        def parse(field):
            return [(int(x,16),int(y,16)) for x,y in re.findall(r'\(([0-9A-F]{2}),([0-9A-F]{2})\)', field)]
        entries[room]={'candles':parse(m.group(2)), 'other_objects':parse(m.group(3))}
    if set(entries) != set(range(ROOMS)):
        raise ValueError('Historical object table missing rooms')
    return entries


def write_c_arrays(entries):
    header='''/* Stage 12 staging data only; no SGDK runtime hookup. */
#ifndef VK_STAGE12_GAMEPLAY_DATA_H
#define VK_STAGE12_GAMEPLAY_DATA_H
#include "../room_manager.h"
#define VK_STAGE12_CANDLE_POSITION_COUNT 30
#define VK_STAGE12_OTHER_OBJECT_POSITION_COUNT 5
/* X,Y position; no unproven type, drop or behavior is assigned. */
typedef struct { VKByte x, y; } VKStage12Position;
extern const VKStage12Position vk_stage12_candle_positions[VK_STAGE12_CANDLE_POSITION_COUNT];
extern const VKStage12Position vk_stage12_other_object_positions[VK_STAGE12_OTHER_OBJECT_POSITION_COUNT];
extern const VKByte vk_stage12_candle_start[13];
extern const VKByte vk_stage12_other_object_start[13];
/* Tile bytes are the validated normal 4F83 decode; runtime collision semantics remain to verify. */
const VKByte *vk_stage12_collision_tiles(VKByte room);
#endif
'''
    strings=['/* Positions reproduced from existing historical 5A49/5B1B harness documentation.',
             ' * NO claim about object identity, drop, activation or gameplay validation. */',
             '#include "stage12_gameplay_data.h"',
             '#include "stage12_foundations.h"']
    for kind,label,macro in [('candles','candle','CANDLE'),('other_objects','other_object','OTHER_OBJECT')]:
        flat=[];starts=[]
        for room in range(ROOMS):
            starts.append(len(flat))
            flat.extend(entries[room][kind])
        starts.append(len(flat))
        if len(flat) != (30 if kind=='candles' else 5):
            raise AssertionError(f'Unexpected {kind} total {len(flat)}')
        strings += [f'const VKStage12Position vk_stage12_{label}_positions[VK_STAGE12_{macro}_POSITION_COUNT] = {{']
        for room in range(ROOMS):
            for x,y in entries[room][kind]:
                strings.append(f'    {{{x},{y}}}, /* room {room} */')
        strings += ['};',f'const VKByte vk_stage12_{label}_start[13] = '+'{'+','.join(map(str,starts))+'};']
    strings += ['const VKByte *vk_stage12_collision_tiles(VKByte room)', '{',
                '    return room < VK_STAGE12_ROOM_COUNT ? vk_stage12_tiles[room] : 0;', '}', '']
    dest=ROOT/'sgdk/src/generated'
    (dest/'stage12_gameplay_data.h').write_text(header)
    (dest/'stage12_gameplay_data.c').write_text('\n'.join(strings))


def main(rom_path):
    rom=load_rom(Path(rom_path))
    compared, precedents=check_precedents(rom)
    entries=historical_positions()
    maps=[bytes(decode_room(rom,12,room)[2]) for room in range(ROOMS)]
    # Existing stage-12 tiles must be exact images of 4F83's normal-path expansion.
    existing=former_stage_map(ROOT/'sgdk/src/generated/stage12_foundations.c')
    allbytes=b''.join(maps)
    if existing[:len(allbytes)] != allbytes or len(existing) != len(allbytes):
        raise AssertionError('Stage-12 collision tiles disagree with raw ROM expansion')
    write_c_arrays(entries)
    report={
        'rom_sha256':hashlib.sha256(rom).hexdigest(),
        'scope':'collision map load data and historical room object positions, not playable stage',
        'method_collision':'4F83 normal path 4FAF translated via tools/extract_rooms.py; independently compared with generated Z80 maps stages 1-11',
        'precedent_comparison_bytes':compared,
        'precedent_rooms':sum(e['rooms'] for e in precedents),
        'precedents':precedents,
        'stage12_collision_rooms':ROOMS,
        'stage12_collision_bytes':len(allbytes),
        'stage12_collision_sha256':hashlib.sha256(allbytes).hexdigest(),
        'method_objects':'positions transcribed and mechanically parsed from historical 5A49+5B1B CPU+mapper execution results in research/castle-candles.md; no current Z80 execution',
        'objects_by_room':{str(r):{'candles':[list(v) for v in entries[r]['candles']], 'other_objects':[list(v) for v in entries[r]['other_objects']]} for r in range(ROOMS)},
        'total_candle_positions':sum(len(entries[r]['candles']) for r in range(ROOMS)),
        'total_other_object_positions':sum(len(entries[r]['other_objects']) for r in range(ROOMS)),
        'not_verified':['types and contents of stage12 candles/objects', '5A49/5B1B raw object records in this session','6154 fixed enemies', 'collision runtime semantics/solid tile classifications', 'secret-passage state machine', 'key/door conditions', 'SGDK in-emulator gameplay']}
    (ROOT/'research/stage12-gameplay-data-audit.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    print(f'Stage 12: {compared} precedent bytes identical; {len(allbytes)} collision bytes; {report["total_candle_positions"]} candles; {report["total_other_object_positions"]} other objects (positions only)')

if __name__=='__main__':
    if len(sys.argv)!=2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
