#!/usr/bin/env python3
"""Run the baseline build and required source/MD/reference checks."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from probe import ROOT, load_rom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--core', required=True, type=Path)
    args = parser.parse_args()
    load_rom(args.rom)
    if not args.core.is_file():
        parser.error('Provide a Genesis Plus GX libretro core')
    def python(tool, *arguments):
        subprocess.run([sys.executable, str(ROOT / 'tools' / tool), *map(str, arguments)], check=True)
    python('build.py', '--verify-reproducible')
    python('verify_msx_startup.py', args.rom)
    python('verify_z80_maps.py', args.rom)
    library = ROOT / 'build/cache/player.so'
    subprocess.run(['gcc', '-shared', '-fPIC', '-std=c99', '-pedantic-errors', '-Wall',
                    '-Wextra', '-Werror', '-O2', str(ROOT / 'sgdk/src/player.c'),
                    str(ROOT / 'sgdk/src/generated/player_assets.c'), '-o', str(library)], check=True)
    python('verify_player_primitives.py', args.rom, library)
    md = ROOT / 'build/vampire-killer-md-viewer.bin'
    symbols = ROOT / 'sgdk/out/symbol.txt'
    python('verify_intro.py', args.core, args.rom, md, symbols)
    python('verify_play_mode.py', args.core, args.rom, md, symbols)
    python('verify_viewer.py', args.core, args.rom, md)
    report = {'all_passed': True, 'checks': ['two clean GCC builds', 'full-ROM MSX startup reference',
              '153 Z80 maps', '53872 C/Z80 primitive comparisons', 'MD startup flow and pixels',
              'MD movement', '152 MD backgrounds'],
              'startup_status': 'Graphics validated; documented timing adaptations; silent port'}
    (ROOT / 'validation/baseline-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
