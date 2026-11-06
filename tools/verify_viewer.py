#!/usr/bin/env python3
"""Compare les 152 fonds affiches par Genesis Plus GX a l'extraction ROM.
Ne valide pas l'extracteur contre une execution du jeu MSX2 original.
"""
import argparse
import hashlib
import json
from functools import reduce
from operator import xor
from pathlib import Path
from PIL import ImageChops
from probe import load_rom, ROOT
from extract_rooms import decode_room, render
from run_headless import run


def gx_rgb565(rgb):
    r, g, b = rgb
    # Genesis Plus GX vdp_render.c MAKE_PIXEL(r<<1,g<<1,b<<1), RGB565 output.
    return ((r * 4 + (r >> 2)) * 255 // 31,
            (g * 8 + (g >> 1)) * 255 // 63,
            (b * 4 + (b >> 2)) * 255 // 31)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core', type=Path)
    parser.add_argument('source_rom', type=Path)
    parser.add_argument('md_rom', type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / 'validation')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    data = load_rom(args.source_rom)
    manifest = json.loads((ROOT / 'assets/manifest.json').read_text())
    rooms = [r for r in manifest['rooms'] if r['status'] == 'static_background_decoded']
    buttons = []
    for before, after in zip(rooms, rooms[1:]):
        buttons.append(7 if after['stage'] == before['stage'] else 4)
    results = []

    def observe(image, step):
        room = rooms[step]
        record, graphics, tilemap = decode_room(data, room['stage'], room['room'])
        expected = render(graphics, tilemap, record['palette_rgb333'],
                          [gx_rgb565(c) for c in record['palette_rgb333']])
        crop = image.crop((0, 32, 256, 208))
        diff = ImageChops.difference(crop, expected)
        passed = diff.getbbox() is None
        results.append({'stage': room['stage'], 'room': room['room'], 'pixels_equal': passed})
        if step in (0, 7, 8, len(rooms) - 1):
            image.save(args.out / f"md-stage-{room['stage']:02d}-room-{room['room']:02d}.png")
        if not passed:
            diff.save(args.out / f"difference-{step:03d}.png")
            raise AssertionError(f"Image differente: stage={room['stage']} room={room['room']}")

    startup = [([1], 5), ([], 5)]  # A from the title opens the developer explorer.
    run(args.core, args.md_rom, args.out / 'last-room.png', buttons=buttons, observe=observe, startup=startup)
    # Atlas toggle, stage wrap and reset are separate navigation paths.
    run(args.core, args.md_rom, args.out / 'md-atlas.png', buttons=[1], startup=startup)
    run(args.core, args.md_rom, args.out / 'md-stage-wrap.png', buttons=[5], startup=startup)
    run(args.core, args.md_rom, args.out / 'md-reset.png', buttons=[7, 4, 1, 3], startup=startup)
    from PIL import Image
    assert Image.open(args.out / 'md-reset.png').tobytes() == Image.open(args.out / 'md-stage-01-room-00.png').tobytes()
    assert Image.open(args.out / 'md-atlas.png').tobytes() != Image.open(args.out / 'md-stage-01-room-00.png').tobytes()
    raw = args.md_rom.read_bytes()
    checksum = sum(int.from_bytes(raw[i:i + 2], 'big') for i in range(0x200, len(raw), 2)) & 0xFFFF
    assert raw[0x100:0x104] == b'SEGA'
    assert len(raw) % 131072 == 0
    # SGDK sizebnd uses XOR of 16-bit words over the entire ROM, including header.
    sgdk_xor = reduce(xor, (int.from_bytes(raw[i:i + 2], 'big') for i in range(0, len(raw), 2)))
    assert sgdk_xor == 0
    report = {'core': 'Genesis Plus GX (libretro), RGB565',
              'source_rom_sha256': hashlib.sha256(data).hexdigest(),
              'md_rom_sha256': hashlib.sha256(raw).hexdigest(), 'md_rom_size': len(raw),
              'header_checksum': raw[0x18E:0x190].hex().upper(), 'checksum_format': 'SGDK XOR16',
              'sgdk_xor_over_rom': sgdk_xor, 'sega_sum16_for_reference': f'{checksum:04X}',
              'all_backgrounds_equal': all(r['pixels_equal'] for r in results),
              'backgrounds_checked': len(results), 'pixels_per_background': 256 * 176,
              'navigation': {'next_room': True, 'next_stage': True, 'reset': True, 'atlas_toggle': True},
              'scope': 'MD viewer vs static extraction; original MSX execution not checked.',
              'rooms': results}
    (args.out / 'viewer-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'rooms'}, indent=2))


if __name__ == '__main__':
    main()
