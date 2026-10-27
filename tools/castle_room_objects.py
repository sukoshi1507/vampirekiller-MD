#!/usr/bin/env python3
"""Decode the object list of each room by running the original ROM routines.

Usage: castle_room_objects.py ROM OUT.json

For each room of assets/manifest.json (stages 1-18) and the three courtyard
rooms (stage 0), sets D000 (stage), D001 (room) and D002 (ROM table 5E6A[stage]),
runs 5A49 (decodes the stage stream from bank 14 into E000) and then 5B1B
(copies the room objects into C420-C5FF). The 8 slots C470-C4F0 receive the
candle records: byte +0 = type (01 at load, 02 once updated by 868C), +1 = Y,
+2 = X, +5 = unknown kind byte. C500-C5F0 receive other objects (type 83 seen).

Harness only: CPU + bank mapper, no VDP, no interrupts. Read-only on the ROM.
Verified in game for courtyard rooms 0 and 1 and hall rooms 0 and 1 (see
research/castle-candles.md).
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))

from z80 import Z80Machine  # noqa: E402  (kosarev/z80, meme dependance que verify_z80_maps.py)
from probe import load_rom  # noqa: E402

STAGE_TABLE_5E6A = 0x1E6A  # D002 = table[stage] ; CPU 5E6A (bank 0, fenetre fixe 4000) = offset fichier 1E6A
COURTYARD_ROOMS = (0, 1, 2)


def make_machine(data):
    machine = Z80Machine()
    for bank in range(4):
        machine.set_memory_block(0x4000 + bank * 8192, data[bank * 8192:(bank + 1) * 8192])

    def mapper_write(address, value):
        window = (address - 0x4000) // 8192
        if window not in (1, 2, 3) or value > 15:
            raise ValueError(f'Ecriture mapper inattendue {address:04X}={value}')
        machine.set_memory_block(0x4000 + window * 8192, data[value * 8192:(value + 1) * 8192])

    machine.set_write_callback(mapper_write)
    machine.mark_addrs(0x6000, 0x6000, machine.WRITE_MARK)
    return machine


def call(machine, addr, limit=3000000):
    machine.sp = 0xF100
    machine.memory[0xF100:0xF102] = bytes((0, 1))
    machine.pc = addr
    machine.set_breakpoint(0x0100)
    machine.ticks_to_stop = limit
    for _ in range(100):
        machine.run()
        if machine.pc == 0x0100:
            return
        if not machine.ticks_to_stop:
            raise RuntimeError(f'Limite de cycles atteinte pour {addr:04X}')
    raise RuntimeError(f'La routine {addr:04X} ne retourne pas')


def decode_room(data, stage, room):
    machine = make_machine(data)
    machine.memory[0xD000] = stage
    machine.memory[0xD001] = room
    machine.memory[0xD002] = data[STAGE_TABLE_5E6A + stage]
    machine.memory[0xC41A] = 0
    call(machine, 0x5A49)
    call(machine, 0x5B1B)
    objects = bytes(machine.memory[0xC420:0xC600])
    candles, items = [], []
    for slot in range(0, len(objects), 16):
        address = 0xC420 + slot
        record = objects[slot:slot + 16]
        if not any(record):
            continue
        entry = {'slot': f'{address:04X}', 'type': record[0], 'y': record[1],
                 'x': record[2], 'kind': record[5], 'raw': record.hex()}
        if 0xC470 <= address <= 0xC4F0 and record[0] == 1:
            candles.append(entry)
        else:
            items.append(entry)
    return {'stage': stage, 'room': room, 'd002': data[STAGE_TABLE_5E6A + stage],
            'candles': candles, 'other_objects': items}


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    data = load_rom(Path(sys.argv[1]))
    manifest = json.loads((ROOT / 'assets/manifest.json').read_text())
    pairs = [(0, room) for room in COURTYARD_ROOMS]
    pairs += [(room['stage'], room['room']) for room in manifest['rooms']]
    result = [decode_room(data, stage, room) for stage, room in pairs]
    Path(sys.argv[2]).write_text(json.dumps(result, indent=1) + '\n')
    print(f'{len(result)} salles ; bougies : {sum(len(r["candles"]) for r in result)}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
