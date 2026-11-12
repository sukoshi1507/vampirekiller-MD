#!/bin/sh
# Dracula (stage 18 room 9, both forms) beaten with the controller only on the built MD ROM, up to the ending
# (front mode leaves the interior). Build first: cd sgdk && make GDK=...
# Env: PYTHON (needs numpy/Pillow only with --shots), OUT (default scratchpad/dracula-fight).
set -eu
cd "$(dirname "$0")/.."
${PYTHON:-python3} tools/dracula_fight_md.py --out "${OUT:-scratchpad/dracula-fight}" --weapon 2 "$@"
