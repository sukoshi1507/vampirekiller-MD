#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE to Genesis Plus GX libretro core}"
${PYTHON:-python3} tools/verify_loot_md.py --core "$VK_MD_CORE" --out "${VK_LOOT_MD_OUT:-scratchpad/lot12-loot-md}"
