#!/usr/bin/env python3
"""Verify saved stage 17/18 object slots and static maps against independent ROM sources.

This checks extracted data, not live enemy/Dracula behavior or playable SGDK integration.
"""
import argparse
import hashlib
import json
from pathlib import Path
from collections import Counter
from prepare_stages17_18_gameplay import main as check_generated
from prepare_stage12_candles import verified_reference, definition
from prepare_stage12_fixed_objects import validated, decode
from probe import load_rom
from extract_rooms import decode_room

ROOT = Path(__file__).resolve().parents[1]


def verify(rom_path):
    rows = json.loads((ROOT / 'references/castle_objects_z80.json').read_text())
    assert verified_reference(rows) == 275
    assert sum(len(v) for _, v in validated()) == 5  # also checks 71 earlier fixed items
    check_generated(True)
    rom = load_rom(rom_path)
    summary = {}
    for stage, count in ((17, 12), (18, 10)):
        records = sorted((r for r in rows if r['stage'] == stage), key=lambda r: r['room'])
        assert [r['room'] for r in records] == list(range(count))
        audit = json.loads((ROOT / f'research/stage{stage}-gameplay-audit.json').read_text())
        foundation = json.loads((ROOT / f'research/stage{stage}-foundations.json').read_text())
        assert foundation['rom_sha256'] == hashlib.sha256(rom).hexdigest()
        assert foundation['room_count'] == count
        raw = []
        candles = fixed = keys = unknown = 0
        for r in records:
            for category in ('candles', 'other_objects'):
                for item in r[category]:
                    b = bytes.fromhex(item['raw'])
                    assert len(b) == 16
                    assert (b[0], b[1], b[2], b[5]) == (item['type'], item['y'], item['x'], item['kind'])
                    raw.append(b)
                    if category == 'candles' and b[0] == 1:
                        assert definition(item)[3] == b[5]
                        candles += 1
                    elif b[0] == 0x83:
                        d = decode(item)
                        if d[3] == 0x18:
                            keys += 1
                        else:
                            fixed += 1
                    else:
                        assert b[0] == 0 and b[5] == 1 and category == 'other_objects'
                        unknown += 1
        assert (candles, fixed, keys, unknown, len(raw)) == tuple(audit[x] for x in ('candle_count', 'fixed_count', 'white_keys')) + (len(audit['unknown_slots']), audit['raw_slots'])
        assert hashlib.sha256(b''.join(raw)).hexdigest() == audit['raw_sha256']
        for room in range(count):
            actual = bytes(decode_room(rom, stage, room)[2]); assert len(actual) == 768
            assert hashlib.sha256(actual).hexdigest() == foundation['rooms'][room]['map_sha256']
        summary[str(stage)] = {'rooms': count, 'collision_bytes': count * 768, 'candles': candles, 'fixed_objects': fixed, 'white_keys': keys, 'uninterpreted_raw_slots': unknown, 'raw_records': len(raw), 'raw_sha256': audit['raw_sha256']}
    return summary

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('rom', type=Path)
    a = parser.parse_args()
    print(json.dumps(verify(a.rom), indent=2, ensure_ascii=False))
