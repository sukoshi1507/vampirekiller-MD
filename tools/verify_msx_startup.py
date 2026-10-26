#!/usr/bin/env python3
"""Compare extraction with recorded full-ROM openMSX VRAM and sprite data."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import zipfile
from PIL import Image, ImageChops
from probe import ROOT, load_rom
from intro_harness import IntroHarness


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    args = parser.parse_args()
    rom = load_rom(args.rom)
    reference = ROOT / 'validation/msx/startup-reference.zip'
    baseline = json.loads((ROOT / 'config/baseline.json').read_text())
    if hashlib.sha256(reference.read_bytes()).hexdigest() != baseline['reference']['sha256']:
        raise AssertionError('Reference archive changed; review before updating its pinned hash')
    with zipfile.ZipFile(reference) as archive:
        metadata = json.loads(archive.read('metadata.json'))
        if metadata['source_rom_sha256'] != hashlib.sha256(rom).hexdigest():
            raise AssertionError('Wrong reference ROM')
        h = IntroHarness(rom)
        h.call(0x4B59); h.call(0x6202)
        for _ in range(98):
            h.call(0x624C)
        backgrounds = []

        def scene(name):
            vram = archive.read(name + '.vram')
            palette = archive.read(name + '.pal')
            assert h.vram[:27136] == vram[:27136], name + ': background bytes'
            used = {i for b in vram[:27136] for i in (b >> 4, b & 15)}
            expected = [((palette[i * 2] >> 4) & 7, palette[i * 2 + 1] & 7,
                         palette[i * 2] & 7) for i in range(16)]
            assert all(h.palette[i] == expected[i] for i in used), name + ': palette'
            # A normalized VRAM preview, explicitly not a native screenshot.
            pixels = [tuple(round(c * 255 / 7) for c in expected[i])
                      for b in vram[:27136] for i in (b >> 4, b & 15)]
            image = Image.new('RGB', (256, 212))
            image.putdata(pixels)
            image.save(ROOT / 'validation/msx' / (name + '-vram.png'))
            asset = {'logo': 'konami', 'title': 'title', 'castle': 'castle',
                     'courtyard': 'courtyard-0'}[name]
            generated = Image.open(ROOT / 'assets/intro' / (asset + '.png')).convert('RGB')
            if name == 'courtyard':
                # The static flame overlay was captured before running the
                # intro ticks. Reproduce that documented asset phase; the
                # hardware comparison above remains on the raw background.
                seed = IntroHarness(rom)
                seed.call(0x4B59); seed.call(0x4D47); seed.call(0x44C6)
                seed.cpu.memory[0xC41A] = 1; seed.call(0x63D3)
                seed.cpu.memory[0xC41A] = 0; seed.call(0x62D0); seed.call(0x422C)
                extracted = seed.composite()
            else:
                extracted = h.image()
            assert ImageChops.difference(generated.crop((0, 0, 256, 212)), extracted).getbbox() is None, \
                name + ': generated scene asset is stale'
            backgrounds.append({'scene': name, 'background_bytes_equal': 27136,
                                'used_palette_entries_equal': sorted(used)})

        scene('logo')
        h.call(0x4D47); scene('title')
        h.call(0x44C6); h.cpu.memory[0xC41A] = 1; h.call(0x63D3); scene('castle')
        samples = list(csv.DictReader(io.StringIO(archive.read('updates.csv').decode())))
        assert len(samples) == 319
        original = IntroHarness(rom)
        for frame, sample in enumerate(samples):
            h.cpu.memory[0xC003] = (frame + 1) & 255
            h.call(0x4E93)
            original.vram[0xF400:0x10000] = archive.read(f'sprites-{frame + 1:03d}.bin')
            # Compare positions, source masks, line colours and CC composition.
            # The hardware data was recorded independently of this decoder.
            assert h.sprite_groups() == original.sprite_groups(), f'Intro update {frame + 1}'
            assert h.vram[0xF400:0xF600] == original.vram[0xF400:0xF600], 'Sprite line colours'
            assert h.vram[0xF800:0x10000] == original.vram[0xF800:0x10000], 'Sprite pattern data'
        h.cpu.memory[0xC41A] = 0; h.call(0x62D0); h.call(0x422C); scene('courtyard')
        memory = archive.read('courtyard.ram')
        assert bytes(h.cpu.memory[0xD100:0xD400]) == memory[0x1100:0x1400]
        trace = list(csv.DictReader(io.StringIO(archive.read('trace.csv').decode())))
        attract_trace = list(csv.DictReader(io.StringIO(archive.read('attract-trace.csv').decode())))
        assert [(s['state'], s['substate']) for s in attract_trace] == [
            ('0', '0'), ('0', '1'), ('0', '2'), ('1', '0'), ('2', '0'), ('2', '1')]
    report = {'all_passed': True, 'reference': metadata['emulator'] + ' / ' + metadata['machine'],
              'reference_zip_sha256': hashlib.sha256(reference.read_bytes()).hexdigest(),
              'source_rom_sha256': metadata['source_rom_sha256'],
              'backgrounds': backgrounds, 'intro_updates_equal': len(samples),
              'intro_groups_per_update': 8, 'courtyard_collision_bytes_equal': 768,
              'state_trace': trace,
              'unattended_demo_trace': attract_trace,
              'first_intro_update_time': float(samples[0]['time']),
              'last_intro_update_time': float(samples[-1]['time']),
              'scope': 'Full-ROM emulation data vs extraction, then separate MD pixel checks.',
              'limits': ['C-BIOS, not a manufacturer BIOS or physical MSX2',
                         'VRAM previews, not native emulator screenshots',
                         'Mega Drive logo timing, title wait and intro skip are adaptations',
                         'Mega Drive sound is not implemented']}
    (ROOT / 'validation/msx-startup-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'all_passed': True, 'backgrounds_equal': len(backgrounds),
                      'intro_updates_equal': len(samples), 'courtyard_map_equal': True}, indent=2))


if __name__ == '__main__':
    main()
