#!/usr/bin/env python3
"""Reproducible ROM and source-data *static* audit for unfinished late-game lots.

Not a Z80 emulator, not an AI port or frame-by-frame behavioral test.
"""
import argparse
import hashlib
import json
from pathlib import Path
from probe import load_rom, physical_offset, reference_rom

ROOT = Path(__file__).resolve().parents[1]
LABELS = {
    'enemy09': '09h / AF5F', 'enemy10': '10h / ADF8 + 9F61',
    'enemy12': '12h / BE70', 'channels': '40h/80h / 5EB8, 5ED8',
    'bosses': 'boss stages 12, 15, 18', 'merchants': 'marchands et murs stages 12-18',
    'secrets': 'passages secrets stages 13-18', 'items': 'inventaire objets',
}


def evidence(rom):
    # Anchored ROM addresses in bank-2 dispatch, bank-3 enemy code.
    start = physical_offset(2, 0x9942, 0x8000)
    dispatch = {f'{t:02X}': int.from_bytes(rom[start + (t-1)*2:start + t*2], 'little')
                for t in (9, 0x10, 0x12)}
    assert dispatch == {'09': 0xAF5F, '10': 0xADF8, '12': 0xBE70}
    window = json.loads((ROOT / 'research/late-enemy-handler-windows.json').read_text())
    for rec in window['routines']:
        addr = int(rec['cpu_entry'], 16)
        end = int(rec['cpu_range_end_exclusive'], 16)
        assert addr == dispatch[rec['type']]
        off = physical_offset(3, addr, 0xA000)
        body = rom[off:off + end - addr]
        assert len(body) == rec['bytes_in_audit_window']
        assert hashlib.sha256(body).hexdigest() == rec['sha256']
        assert body[:16].hex(' ') == rec['entry_bytes']
    fixed = json.loads((ROOT / 'research/late-enemy-unported-worklist.json').read_text())
    flags = json.loads((ROOT / 'research/late-enemy-flags-audit.json').read_text())
    return dispatch, fixed, flags


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('lot', choices=(*LABELS, 'all'))
    p.add_argument('--check', action='store_true', help='Read-only validation (the default)')
    a = p.parse_args()
    rom = load_rom(reference_rom())
    dispatch, fixed, flags = evidence(rom)
    assert len(rom) == 0x20000
    # Verify that all 7 late-stage flag arrays refer to the exact original bytes.
    assert [s['stage'] for s in flags['stages']] == list(range(12, 19))
    for s in flags['stages']:
        assert len(s['flags']) == s['count']
        assert list(rom[s['offset']:s['offset']+s['count']]) == s['flags']
        assert sum(bool(x) for x in s['flags']) == s['nonzero']
    # Unported worklist must preserve the three dispatch targets.
    text = (ROOT / 'research/late-enemy-unported-worklist.md').read_text()
    assert fixed['unsupported'] == len(fixed['records']) == 47
    totals = {t: sum(r['type'] == t for r in fixed['records']) for t in ('09', '10', '12')}
    assert totals == {'09': 13, '10': 28, '12': 6}
    assert all(r['handler_cpu'] == f"{dispatch[r['type']]:04X}" for r in fixed['records'])
    assert '47' in text and 'AF5F' in text and 'ADF8' in text and 'BE70' in text
    # Shared source-file anchor checks for items / trade and current implementation boundaries.
    candles = (ROOT / 'sgdk/src/candles.c').read_text()
    wall = (ROOT / 'sgdk/src/wall.c').read_text()
    enemies = (ROOT / 'sgdk/src/enemies.c').read_text()
    gameplay = (ROOT / 'sgdk/src/gameplay.c').read_text()
    for token in ('kind == 0x12', 'kind == 0x0A', 'kind == 0x04',
                  'kind == 0x10 || kind == 0x11', 'kind == 0x1E || kind == 0x0F'):
        assert token in candles
    assert 'VK_SHOP_PRICE_COUNT' in wall
    assert '#define VK_SHOP_PRICE_COUNT 9' in (ROOT/'sgdk/src/generated/trade_data.h').read_text()
    assert 'vk_candles_give(c, w->sale)' in wall
    assert 'VK_ITEM_HIGH_JUMP' in gameplay
    assert 'vk_late_enemy_flags' in enemies
    # Explicitly non-behavioral: snapshots of evidence sources for all eight lots.
    results = {
        'dispatch': {k: f'{v:04X}' for k, v in dispatch.items()},
        'flagged_rooms': {str(s['stage']): s['nonzero'] for s in flags['stages']},
        'worklist_type_totals': totals,
    }
    lots = LABELS if a.lot == 'all' else {a.lot: LABELS[a.lot]}
    for key, label in lots.items():
        print(f'{key}: {label}: static ROM/source consistency PASS (no Z80 execution)')
    print('Evidence:', json.dumps(results, sort_keys=True))

if __name__ == '__main__':
    main()
