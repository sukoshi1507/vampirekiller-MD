#!/usr/bin/env python3
"""Build with a pinned SGDK/GCC setup; optionally compare two clean builds."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sgdk', type=Path, default=os.environ.get('GDK') or None)
    parser.add_argument('--prefix', default=os.environ.get('PREFIX', 'm68k-elf-'))
    parser.add_argument('--verify-reproducible', action='store_true')
    args = parser.parse_args()
    if args.sgdk is None:
        parser.error('Provide --sgdk /path/to/SGDK or set GDK')
    sdk = args.sgdk.resolve()
    if not re.search(r'#define\s+SGDK_VERSION\s+2\.11\b', (sdk / 'inc/genesis.h').read_text()):
        parser.error('This baseline requires SGDK 2.11')
    compiler = shutil.which(args.prefix + 'gcc')
    if compiler is None:
        parser.error('GCC not found; set --prefix to the full m68k-elf- prefix')
    prefix = compiler[:-3]
    version = subprocess.check_output([compiler, '-dumpfullversion'], text=True).strip()
    if version != '13.1.0':
        parser.error('This reproducible baseline requires GCC 13.1.0')
    env = os.environ.copy()
    java = shutil.which('java')
    if java:
        java_lib = Path(java).resolve().parents[1] / 'lib'
        env['LD_LIBRARY_PATH'] = str(java_lib) + os.pathsep + env.get('LD_LIBRARY_PATH', '')
    cache = ROOT / 'build/cache'
    cache.mkdir(parents=True, exist_ok=True)
    temporary = cache / 'tmp'
    temporary.mkdir(exist_ok=True)
    # GCC may otherwise fall back to the source directory when /tmp is blocked.
    # SGDK discovers *.s there and can accidentally compile a leftover LTO file.
    env['TMPDIR'] = str(temporary)
    native = cache / 'libmd-native.a'
    # SGDK's archive contains native fat objects; avoid foreign GCC LTO sections.
    # Never change the user's SDK installation.
    subprocess.run([prefix + 'objcopy', '--remove-section=.gnu.lto_*',
                    str(sdk / 'lib/libmd.a'), str(native)], check=True)
    command = ['make', '-C', str(ROOT / 'sgdk'), 'GDK=' + str(sdk),
               'PREFIX=' + prefix, 'LIBMD=' + str(native), 'EXTRA_FLAGS=-std=c99 -Werror']
    hashes = []
    with (ROOT / 'validation/build.log').open('w') as log:
        for _ in range(2 if args.verify_reproducible else 1):
            subprocess.run(command + ['clean'], env=env, stdout=log, stderr=log, check=True)
            subprocess.run(command, env=env, stdout=log, stderr=log, check=True)
            rom = (ROOT / 'sgdk/out/rom.bin').read_bytes()
            hashes.append(hashlib.sha256(rom).hexdigest())
    log_path = ROOT / 'validation/build.log'
    log_path.write_text('\n'.join(line.rstrip() for line in log_path.read_text().splitlines()) + '\n')
    if len(set(hashes)) != 1:
        raise RuntimeError('Clean builds produced different ROMs')
    if len(rom) != 524288 or rom[0x100:0x104] != b'SEGA':
        raise RuntimeError('Unexpected Mega Drive ROM header or size')
    if int.from_bytes(rom[0x1A4:0x1A8], 'big') != len(rom) - 1:
        raise RuntimeError('ROM header end address does not match binary size')
    checksum = 0
    for i in range(0, len(rom), 2):
        checksum ^= int.from_bytes(rom[i:i + 2], 'big')
    if checksum:
        raise RuntimeError('Invalid SGDK XOR checksum')
    shutil.copyfile(ROOT / 'sgdk/out/rom.bin', ROOT / 'build/vampire-killer-md-viewer.bin')
    report = {'passed': True, 'sgdk': '2.11', 'gcc': version,
              'extra_flags': '-std=c99 -Werror', 'clean_builds': len(hashes),
              'rom_sha256': hashes[0], 'rom_size': len(rom),
              'reproducible': len(hashes) == 2, 'sgdk_xor16': checksum,
              'library_mode': 'native fat objects; LTO sections removed from a local copy'}
    (ROOT / 'validation/build-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
