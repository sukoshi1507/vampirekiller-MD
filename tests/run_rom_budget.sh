#!/bin/sh
# ROM budget: Vampire Killer must fit in the 4 MiB collection after Castlevania-MD
# (tools/rom_budget.py, research/rom-budget.md). Reads the existing ELF, runs in well under a second.
set -eu
cd "$(dirname "$0")/.."
ELF=sgdk/out/rom.out
if [ ! -f "$ELF" ]; then
    (cd sgdk && make GDK="${GDK:-$HOME/Documents/githubsuko/SGDK}" >/dev/null)
fi
${PYTHON:-python3} tools/rom_budget.py --elf "$ELF" --check
