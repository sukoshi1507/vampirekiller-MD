#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
SCENARIO_OUT="${VK_SCENARIO_MD_OUT:-scratchpad/lot12-scenarios}"
${PYTHON:-python3} tools/build_late_scenarios.py --check
for stage in 12 13 14 15 16 17 18; do
    ${PYTHON:-python3} tools/md_harness.py "tests/md_scenarios/zzp_stage${stage}.txt" --elf sgdk/out/rom.out --rom sgdk/out/rom.bin --core "$VK_MD_CORE" --out "$SCENARIO_OUT/stage${stage}" --quiet
done
