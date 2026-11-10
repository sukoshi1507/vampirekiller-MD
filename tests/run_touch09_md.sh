#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} tools/verify_touch09_md.py --core "$VK_MD_CORE" --out "${VK_TOUCH09_MD_OUT:-scratchpad/lot14f-touch09-md}"
${PYTHON:-python3} tools/reproduce_boss15_pool_gap.py --free-third --core "$VK_MD_CORE" --out scratchpad/lot14f-boss-coexist

${PYTHON:-python3} tools/reproduce_boss15_pool_gap.py --core "$VK_MD_CORE" --out scratchpad/lot14g-boss-full
