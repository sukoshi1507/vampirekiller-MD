#!/usr/bin/env python3
"""Execute la routine originale 4F83/4FAF et compare les 153 cartes decodees.
Harness CPU + mapper uniquement: ce n'est pas une emulation MSX2 complete.
"""
import argparse
import json
from pathlib import Path
from z80 import Z80Machine
from probe import load_rom, ROOT
from extract_rooms import decode_room


def execute_map(data, stage, room):
    machine = Z80Machine()
    for bank in range(4):
        machine.set_memory_block(0x4000 + bank * 8192, data[bank * 8192:(bank + 1) * 8192])
    writes = []

    def mapper_write(address, value):
        window = (address - 0x4000) // 8192
        if window not in (1, 2, 3) or value > 15:
            raise ValueError('Ecriture mapper inattendue')
        machine.set_memory_block(0x4000 + window * 8192, data[value * 8192:(value + 1) * 8192])
        writes.append((address, value))

    machine.set_write_callback(mapper_write)
    machine.mark_addrs(0x6000, 0x6000, machine.WRITE_MARK)
    machine.memory[0xD000] = stage
    machine.memory[0xD001] = room
    machine.memory[0xC41A] = 0  # Chemin normal, distinct du chemin alternatif.
    machine.sp = 0xF100
    machine.memory[0xF100:0xF102] = bytes((0, 1))
    machine.pc = 0x4F83
    machine.set_breakpoint(0x0100)
    machine.ticks_to_stop = 1000000
    for _ in range(100):
        machine.run()
        if machine.pc == 0x0100:
            break
        if not machine.ticks_to_stop:
            raise RuntimeError('Limite de cycles atteinte')
    else:
        raise RuntimeError('La routine ne retourne pas')
    if writes != [(0x6000, 11), (0x8000, 12), (0xA000, 13),
                  (0x6000, 1), (0x8000, 2), (0xA000, 3)]:
        raise AssertionError('Sequence de banques inattendue')
    return bytes(machine.memory[0xD100:0xD400]), 1000000 - machine.ticks_to_stop


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    data = load_rom(args.rom)
    manifest = json.loads((ROOT / 'assets/manifest.json').read_text())
    results = []
    for room in manifest['rooms']:
        actual, cycles = execute_map(data, room['stage'], room['room'])
        expected = bytes(decode_room(data, room['stage'], room['room'])[2])
        if actual != expected:
            raise AssertionError(f"Carte differente: {room['stage']}/{room['room']}")
        results.append({'stage': room['stage'], 'room': room['room'],
                        'all_768_bytes_equal': True, 'cycles_until_return': cycles})
    report = {'emulator': 'kosarev/z80 Python package 1.2.0',
              'entry': '4F83', 'return_breakpoint': '0100', 'alternate_path_C41A': 0,
              'maps_checked': len(results), 'bytes_per_map': 768,
              'all_maps_equal': True, 'mapper_restore_verified': True,
              'scope': 'Original ROM routine in CPU+mapper harness, no BIOS/VDP/interrupt timing.',
              'rooms': results}
    out = ROOT / 'validation'
    out.mkdir(exist_ok=True)
    (out / 'z80-map-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'rooms'}, indent=2))


if __name__ == '__main__':
    main()
