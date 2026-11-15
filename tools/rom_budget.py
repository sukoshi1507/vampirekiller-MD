#!/usr/bin/env python3
"""ROM budget of Vampire Killer MD inside the 4 MiB Vampire Collection (research/rom-budget.md).

Reads sgdk/out/rom.out (m68k-elf-nm / objdump), sorts the ROM image into categories and prints
what is left in the collection: Vampire Killer starts at VK_BASE (vampirecollection-md/Makefile,
default 0x214000), after Castlevania-MD, and must end before 0x400000 (no mapper;
collection/merge_rom.py stops otherwise).

Usage: python3 tools/rom_budget.py [--elf rom.out] [--top N] [--check] [--margin BYTES]
--check exits 1 when the image is over the ceiling (or leaves less than --margin bytes).
"""
import argparse
import collections
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM_MAX = 4 * 1024 * 1024
DEFAULT_VK_BASE = 0x214000
COLLECTION = Path(os.environ.get('VK_COLLECTION', ROOT.parent / 'vampirecollection-md'))
GDK = Path(os.environ.get('GDK', '/home/sukoshi/Documents/githubsuko/SGDK'))

CATEGORIES = ('cartes', 'tuiles MSX', 'tuiles 16 bits', 'sprites', 'son PSG', 'son FM', 'PCM',
              'code', 'SGDK', 'autres')


def run(*cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def vk_base():
    """VK_BASE of the collection Makefile when it is checked out next to this repository."""
    env = os.environ.get('VK_BASE')
    if env:
        return int(env, 0)
    try:
        m = re.search(r'^VK_BASE\s*:?=\s*(0x[0-9A-Fa-f]+)', (COLLECTION / 'Makefile').read_text(), re.M)
        if m:
            return int(m.group(1), 16)
    except OSError:
        pass
    return DEFAULT_VK_BASE


def image_size(elf):
    """Bytes of the cartridge image (objcopy -O binary): loaded sections from the first LMA to the last."""
    lo, hi = None, 0
    lines = run('m68k-elf-objdump', '-h', str(elf)).splitlines()
    for a, b in zip(lines, lines[1:]):
        f = a.split()
        if len(f) >= 7 and f[0].isdigit() and 'LOAD' in b and 'CONTENTS' in b:
            size, lma = int(f[2], 16), int(f[4], 16)
            if size:
                lo = lma if lo is None else min(lo, lma)
                hi = max(hi, lma + size)
    return hi - (lo or 0), hi


def text_size(elf):
    for line in run('m68k-elf-objdump', '-h', str(elf)).splitlines():
        f = line.split()
        if len(f) >= 3 and f[1] == '.text':
            return int(f[2], 16)
    return 0


def sgdk_symbols():
    names = set()
    for lib in ('libmd.a', 'libgcc.a'):
        path = GDK / 'lib' / lib
        if path.exists():
            for line in run('m68k-elf-nm', '--defined-only', '-g', str(path)).splitlines():
                f = line.split()
                if len(f) == 3 and f[1] in 'TtDdRrWw':
                    names.add(f[2])
    return names


def base_name(name):
    return re.sub(r'(\.(constprop|isra|part|lto_priv|cold)\.\d+)+$', '', name)


SGDK_PREFIX = re.compile(r'^(VDP_|SPR_|SYS_|XGM|DMA_|JOY_|MEM_|PAL_|TILE_|BMP_|MAP_|SND_|Z80_|YM2612_|PSG_|'
                         r'FIX|RNG|STR_|KLOG|VRAM|TSK_|SRAM_|FF_|DMAQ|MDDBG|__|_)|^(memcpy|memset|memcmp|strlen|'
                         r'strcpy|strcmp|vsprintf|sprintf|siprintf|vsnprintf|sgdk|drv_|font_|rom_head|ldiv|ltoa|'
                         r'intToStr|uintToStr|u2s|JOY|Z80|printf|free|malloc)')
# Names of the game's own tables; the pattern order is the priority.
RULES = (
    ('PCM', re.compile(r'^wilhelm_pcm$|^drv_pcm$')),
    ('son FM', re.compile(r'^msx_fm_|^vk_fm_')),
    ('son PSG', re.compile(r'^msx_sound_|^vk_psg_(init|table|songs)')),
    ('cartes', re.compile(r'^(vk_maps|late_maps|scene_map_\d+|vk_courtyard_maps|vk_dracula_background_map|vk_intro16_map|'
                          r'vk_end16_map|g16_\w+_map|vk_stage\d+_collision|vk_stages_?\w*collision)$')),
    ('tuiles 16 bits', re.compile(r'^(vk_gfx16_\w+|g16_[\w]+|vk_(intro|end|wall|hud)16_\w*)$')),
    ('tuiles MSX', re.compile(r'^(vk_tiles|vk_stage\d+_tiles|scene_tiles_\d+|vk_intro_tiles|vk_msx_font_tiles|'
                              r'vk_wall_frame_tiles|vk_dracula_background_tiles|late_graphics_(words|cells|slots)|'
                              r'vk_intro_frames)$')),
    ('sprites', re.compile(r'(^|_)(simon|player|enemy|enemies|boss|whip|orb|item|object|weapon|candle|dracula|ghost|'
                           r'sprite|bust|flame|key)\w*_(tiles|sets|sprites|frames|poses)\w*$|^vk_\w*16_\w+$|'
                           r'^vk_(player|simon|stage\d+_enemy|stage\d+_\w+|enemy\d*_\w+)_tiles$|_sprites$')),
    ('autres', re.compile(r'(palette|font|_defs$|_lut$|_table|credit|_text|names|_order$)')),
)


def classify(name, kind, sgdk):
    b = base_name(name)
    if b in sgdk or SGDK_PREFIX.match(b):
        return 'SGDK'
    for cat, pat in RULES:
        if pat.search(b):
            return cat
    return 'code'


def symbols(elf):
    out = []
    for line in run('m68k-elf-nm', '-S', '-t', 'd', '--size-sort', str(elf)).splitlines():
        f = line.split(None, 3)
        if len(f) == 4 and f[2] in ('t', 'T', 'r', 'R'):
            out.append((f[3], int(f[1]), f[2]))
    return out


def inventory(elf):
    sgdk = sgdk_symbols()
    cats = collections.defaultdict(int)
    syms = collections.defaultdict(list)
    for name, size, kind in symbols(elf):
        cat = classify(name, kind, sgdk)
        cats[cat] += size
        syms[cat].append((size, name))
    named = sum(cats.values())
    gap = text_size(elf) - named
    cats['autres'] += max(gap, 0)
    return cats, syms, gap


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--elf', default=str(ROOT / 'sgdk/out/rom.out'))
    ap.add_argument('--top', type=int, default=0, help='list the N biggest symbols of each category')
    ap.add_argument('--check', action='store_true', help='exit 1 when over the collection ceiling')
    ap.add_argument('--margin', type=int, default=0, help='with --check: free bytes required below the ceiling')
    args = ap.parse_args()
    elf = Path(args.elf)
    if not elf.exists():
        sys.exit(f'{elf}: not built (cd sgdk && make GDK=...)')

    img, _ = image_size(elf)
    base = vk_base()
    ceiling = ROM_MAX - base
    free = ceiling - img
    cats, syms, gap = inventory(elf)

    print(f'{elf}: image {img} B ({img / 1024:.1f} KiB), collection ceiling {ceiling} B '
          f'(VK_BASE {base:#x} .. {ROM_MAX:#x})')
    for cat in sorted(cats, key=lambda c: -cats[c]):
        print(f'  {cat:<15}{cats[cat]:>9} B {cats[cat] / 1024:>8.1f} KiB {100 * cats[cat] / img:>5.1f} %')
        for size, name in sorted(syms[cat], reverse=True)[:args.top]:
            print(f'      {size:>9}  {name}')
    print(f'  (autres includes {max(gap, 0)} B without symbol: padding, strings, literal pools)')
    print(f'free in the collection: {free} B ({free / 1024:.1f} KiB)')
    if args.check and free < args.margin:
        print(f'FAIL: ROM over the collection ceiling by {args.margin - free} B', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
