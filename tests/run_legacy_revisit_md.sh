#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} tools/verify_legacy_revisit_md.py --core "$VK_MD_CORE" --out "${VK_LEGACY_MD_OUT:-scratchpad/lot14d-legacy-md}"
${PYTHON:-python3} tools/md_harness.py tests/md_scenarios/zzm_stage10.txt --elf sgdk/out/rom.out --rom sgdk/out/rom.bin --core "$VK_MD_CORE" --out "${VK_LEGACY_STAGE10_OUT:-scratchpad/lot14d-stage10}" --quiet
