#!/usr/bin/env python3
"""Capture an unmodified full-ROM boot under openMSX/C-BIOS, without CPU hooks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zipfile
from probe import ROOT, load_rom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--openmsx', required=True, type=Path)
    parser.add_argument('--system-data', required=True, type=Path)
    args = parser.parse_args()
    rom = load_rom(args.rom)
    binary = args.openmsx.resolve()
    system = args.system_data.resolve()
    work = ROOT / 'build/cache/msx-reference'
    work.mkdir(parents=True, exist_ok=True)
    for old in work.glob('sprites-*.bin'):
        old.unlink()
    for old in work.glob('*.mem'):
        old.unlink()
    (work / 'complete.txt').unlink(missing_ok=True)
    (work / 'game.rom').write_bytes(rom)
    if any(c in str(work) for c in '{}\n\r'):
        parser.error('Capture path cannot contain Tcl braces or newlines')
    script = 'set vk_output {' + str(work) + '}\n'
    script += (ROOT / 'tools/capture_msx_startup.tcl').read_text()
    (work / 'capture.tcl').write_text(script)
    env = dict(os.environ, OPENMSX_SYSTEM_DATA=str(system),
               OPENMSX_USER_DATA=str(work / 'user'),
               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    with (work / 'openmsx.log').open('w') as log:
        subprocess.run([str(binary), '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(work / 'game.rom'), '-script', str(work / 'capture.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=log, env=env,
                       check=True, timeout=60)
    complete = (work / 'complete.txt').read_text().splitlines()
    if complete != ['openMSX 19.1', '319']:
        raise RuntimeError('Unexpected reference version or intro update count: ' + repr(complete))
    # A second unmodified run checks the unattended title and automatic demo.
    (work / 'attract-complete.txt').unlink(missing_ok=True)
    (work / 'capture-attract.tcl').write_text('set vk_attract 1\n' + script)
    with (work / 'openmsx-attract.log').open('w') as log:
        subprocess.run([str(binary), '-control', 'stdio', '-machine', 'C-BIOS_MSX2',
                        '-cart', str(work / 'game.rom'), '-script', str(work / 'capture-attract.tcl')],
                       stdin=subprocess.DEVNULL, stdout=log, stderr=log, env=env,
                       check=True, timeout=60)
    if (work / 'attract-complete.txt').read_text().strip() != 'openMSX 19.1':
        raise RuntimeError('Unattended startup capture did not complete')
    # Only observable graphics/state data is archived; never the game or BIOS ROMs.
    files = [p for p in work.iterdir() if p.suffix in ('.vram', '.pal', '.regs', '.ram', '.csv')
             or p.name.startswith('sprites-')]
    metadata = {'emulator': complete[0], 'machine': 'C-BIOS_MSX2',
                'emulator_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                'source_rom_sha256': hashlib.sha256(rom).hexdigest(),
                'bios_sha256': {name: hashlib.sha256((system / 'machines' / name).read_bytes()).hexdigest()
                                for name in ('cbios_main_msx2.rom', 'cbios_logo_msx2.rom', 'cbios_sub.rom')},
                'input': 'SPACE down/up 0.2/0.4 emulated seconds after first title draw',
                'intro_updates': 319,
                'scenarios': ['SPACE starts the game from the title', 'No input reaches the automatic demo'],
                'ram_range': 'C000-FFFF only; mapped cartridge and BIOS ROM excluded',
                'scope': 'Full ROM, CPU, C-BIOS, V9938 and interrupts; read-only breakpoints, no patches.',
                'images': 'VRAM snapshots, not native emulator framebuffer screenshots'}
    output = ROOT / 'validation/msx'
    output.mkdir(exist_ok=True)
    with zipfile.ZipFile(output / 'startup-reference.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
        for f in sorted(files):
            entry = zipfile.ZipInfo(f.name, date_time=(2026, 10, 26, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, f.read_bytes())
        entry = zipfile.ZipInfo('metadata.json', date_time=(2026, 10, 26, 0, 0, 0))
        entry.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(entry, json.dumps(metadata, indent=2) + '\n')
    (output / 'capture.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
