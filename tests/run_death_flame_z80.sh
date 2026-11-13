#!/bin/sh
# Death flame offset table 9B33 and creation point, ROM routine 9A3E run on the Z80 emulator (all types).
set -eu
cd "$(dirname "$0")/.."
${PYTHON:-python3} tools/verify_death_flame_z80.py
