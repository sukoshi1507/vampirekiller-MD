#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} tools/verify_late_runtime_md.py --core "$VK_MD_CORE" --out "${VK_LATE_MD_OUT:-scratchpad/lot11-late-md}"
