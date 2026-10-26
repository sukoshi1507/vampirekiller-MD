#!/usr/bin/env python3
"""Differential tests: portable C collision/motion primitives vs actual Z80 ROM.
Compile first: cc -shared -fPIC -O2 sgdk/src/player.c \
 sgdk/src/generated/player_assets.c -o /tmp/player.so
"""
import argparse
import ctypes as C
import json
from pathlib import Path
import random
from z80 import Z80Machine
from probe import load_rom, ROOT
from extract_rooms import decode_room


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('native_library', type=Path)
    args = parser.parse_args()
    data = load_rom(args.rom)
    native = C.CDLL(str(args.native_library.resolve()))
    Byte = C.c_ubyte
    Ptr = C.POINTER(Byte)
    native.vk_sample_tile.argtypes = [Ptr, Byte, Byte]
    native.vk_sample_tile.restype = Byte
    native.vk_tile_solid.argtypes = [Byte, Byte, Byte]
    native.vk_grounded.argtypes = [Ptr, Byte, Byte, Byte, Byte]
    native.vk_wall.argtypes = [Ptr, Byte, Byte, C.c_int, Byte, Byte, Byte]
    native.vk_jump_step.argtypes = [Ptr, Ptr]
    machine = Z80Machine()
    for bank in range(4):
        machine.set_memory_block(0x4000 + bank * 8192, data[bank * 8192:(bank + 1) * 8192])
    machine.set_breakpoint(0x0100)
    checks = {'tile_classification': 0, 'coordinate_lookup': 0, 'ground_probe': 0,
              'side_probe': 0, 'jump_step': 0}

    def invoke(address, a=0, d=0, e=0, hl=0, bc=0):
        machine.a, machine.d, machine.e, machine.hl, machine.bc = a, d, e, hl, bc
        machine.f = 0
        machine.sp = 0xF100
        machine.memory[0xF100:0xF102] = bytes((0, 1))
        machine.pc = address
        machine.ticks_to_stop = 20000
        for _ in range(10):
            machine.run()
            if machine.pc == 0x0100:
                return machine.a, machine.f & 1
            if not machine.ticks_to_stop:
                break
        raise RuntimeError(f'Routine did not return: {address:04X}')

    for stage in range(19):
        machine.memory[0xD000] = stage
        for special in (0, 1):
            machine.memory[0xCE00] = 6 if special else 0
            for tile in range(256):
                _, carry = invoke(0x7C5E, a=tile)
                assert carry == native.vk_tile_solid(tile, stage, special), (stage, special, tile)
                checks['tile_classification'] += 1
    rng = random.Random(0x564B)
    manifest = json.loads((ROOT / 'assets/manifest.json').read_text())
    cases = [(room['stage'], room['room'], bytes(decode_room(data, room['stage'], room['room'])[2]))
             for room in manifest['rooms']]
    intro = json.loads((ROOT / 'assets/intro/manifest.json').read_text())
    cases.extend((0, room, bytes(tilemap)) for room, tilemap in enumerate(intro['courtyard_collision_maps']))
    for stage, room_number, tilemap in cases:
        array = (Byte * 768).from_buffer_copy(tilemap)
        machine.set_memory_block(0xD100, tilemap)
        machine.memory[0xD000] = stage
        for _ in range(24):
            x, y = rng.randrange(256), rng.randrange(256)
            actual, _ = invoke(0x7D2F, d=x, e=y)
            assert actual == native.vk_sample_tile(array, x, y), (stage, room_number, x, y)
            checks['coordinate_lookup'] += 1
            machine.memory[0xC427] = x
            machine.memory[0xC425] = y
            for special in (0, 1):
                machine.memory[0xCE00] = 6 if special else 0
                _, carry = invoke(0x7B88)
                assert carry == native.vk_grounded(array, x, y, stage, special)
                checks['ground_probe'] += 1
                for crouched in (0, 1):
                    machine.memory[0xC420] = 2 if crouched else 0
                    for direction, address in ((1, 0x7BA9), (-1, 0x7C05)):
                        _, carry = invoke(address)
                        assert carry == native.vk_wall(array, x, y, direction, crouched, stage, special)
                        checks['side_probe'] += 1
    # Original jump helper, valid active phases only; no artificial 8-bit phase wrap.
    for phase in range(80):
        for y in range(0, 256, 7):
            machine.memory[0xC428] = phase
            machine.memory[0xC425] = y
            invoke(0x6D4D, d=19, hl=0xC428, bc=0x6D95)
            cy, cp = Byte(y), Byte(phase)
            native.vk_jump_step(C.byref(cy), C.byref(cp))
            assert cy.value == machine.memory[0xC425]
            assert cp.value == machine.memory[0xC428]
            checks['jump_step'] += 1
    report = {'all_equal': True, 'cases': checks, 'total': sum(checks.values()),
              'seed': '564B', 'reference': 'Actual original ROM instructions executed by kosarev/z80 1.2.0',
              'scope': 'Collision probes and jump helper, not the full player state dispatcher.'}
    (ROOT / 'validation/player-primitives.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
