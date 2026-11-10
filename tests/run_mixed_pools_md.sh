#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} tools/verify_mixed_pools_md.py --core "$VK_MD_CORE" --out "${VK_MIXED_MD_OUT:-scratchpad/lot14-mixed-md}"
