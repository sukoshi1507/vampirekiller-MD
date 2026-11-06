#!/usr/bin/env python3
"""Extract stage-13 final door raw entry from the ROM 3-byte table.

This is a table extraction only: no Z80 execution, no key or state machine.
Reference: tools/extract_stage_exits.py, stage door offset 0x1BB61+3*stage.
"""
import json
import sys
from pathlib import Path
from probe import load_rom

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x1BB61
STAGE = 13

def extract(data):
    # Cross-check the same table against stage 1-11 Z80-established entries
    # already recorded in the project; detect a wrong table/bank mapping.
    old = json.loads((ROOT / 'research/stages-1-10/MSX2_STAGE_EXITS.json').read_text())
    for record in old:
        s = record['stage']
        raw = data[BASE + s*3:BASE + (s+1)*3]
        assert raw.hex() == record['door']['raw'], (s, raw.hex(), record['door']['raw'])
    off = BASE + STAGE*3
    flags, y, x = data[off:off+3]
    assert (flags & 0x80) != 0
    assert (flags & 0x0F) < 12
    result = {'stage':STAGE,'file_offset':f'0x{off:05X}',
              'raw_hex':data[off:off+3].hex(),
              'flags':flags,'room':flags&0x0F,'y':y,'x':x,
              'source':'ROM table described in tools/extract_stage_exits.py',
              'crosscheck_stages':len(old),
              'scope':'raw door coordinates only; Z80 BB37, key and opening NOT validated'}
    return result

if __name__ == '__main__':
    if len(sys.argv)!=2: raise SystemExit('usage: extract_stage13_door.py reference.rom')
    data=load_rom(Path(sys.argv[1]))
    result=extract(data)
    (ROOT/'research/stage13-door-rom.json').write_text(json.dumps(result,indent=2)+'\n')
    (ROOT/'sgdk/src/generated/stage13_door_rom.h').write_text(
        '/* Stage 13 ROM RAW door table; no door gameplay integration. */\n'
        '#ifndef VK_STAGE13_DOOR_ROM_H\n#define VK_STAGE13_DOOR_ROM_H\n'
        '#define VK_STAGE13_DOOR_RAW_FLAGS '+str(result['flags'])+'\n'
        '#define VK_STAGE13_DOOR_ROOM '+str(result['room'])+'\n'
        '#define VK_STAGE13_DOOR_X '+str(result['x'])+'\n'
        '#define VK_STAGE13_DOOR_Y '+str(result['y'])+'\n'
        '#endif\n')
    print(json.dumps(result,indent=2))
