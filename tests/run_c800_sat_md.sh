#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} tools/verify_c800_sat_md.py --core "$VK_MD_CORE" --out "${VK_C800_MD_OUT:-scratchpad/lot14e-c800-md}"
