#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} tools/verify_candle_loot_md.py --core "$VK_MD_CORE" --out "${VK_CANDLE_LOOT_MD_OUT:-scratchpad/lot13-candle-md}"
