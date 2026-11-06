#!/usr/bin/env python3
"""Verify isolated stage 13/14 collision maps and saved Z80 object snapshots.
Requires a local reference MSX2 ROM; it is never copied into the delivery.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from prepare_stage12_gameplay_data import check_precedents
from prepare_stages13_14_gameplay import main as check_generated
from probe import load_rom
from extract_rooms import decode_room

ROOT=Path(__file__).resolve().parents[1]

def verify(rom):
    check_generated(True)
    n,precedents=check_precedents(rom)
    assert n==60672 and sum(p['rooms'] for p in precedents)==79
    out={'precedent_bytes':n,'precedent_rooms':79,'stages':{}}
    for stage,count in ((13,12),(14,8)):
        tiles=b''.join(bytes(decode_room(rom,stage,room)[2]) for room in range(count))
        source=(ROOT/f'sgdk/src/generated/stage{stage}_foundations.c').read_text()
        tiles_c=source.split(f'const VKByte vk_stage{stage}_tiles',1)[1].split('=',1)[1].split('};',1)[0]
        generated=bytes(int(v) for v in re.findall(r'\b\d+\b',tiles_c))
        assert tiles==generated,(stage,'tile mismatch')
        snapshot=json.loads((ROOT/f'research/stage{stage}-gameplay-audit.json').read_text())
        assert snapshot['stage']==stage and snapshot['room_count']==count
        out['stages'][stage]={'collision_bytes':len(tiles),'sha256':hashlib.sha256(tiles).hexdigest(),'candle_count':snapshot['candle_count'],'fixed_count':snapshot['fixed_count'],'raw_slots':snapshot['raw_slots'],'uninterpreted':len(snapshot['unknown_slots'])}
    return out

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('rom');args=p.parse_args()
    result=verify(load_rom(Path(args.rom)))
    print(json.dumps(result,indent=2))
