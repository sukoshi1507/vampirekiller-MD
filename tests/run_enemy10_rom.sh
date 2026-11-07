#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tools/verify_enemy10_rom.py
