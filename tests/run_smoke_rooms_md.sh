#!/bin/sh
# Crash/hang smoke test: every room of stages 1-18 entered and played a few seconds on the MD build.
set -eu
cd "$(dirname "$0")/../tools"
: "${VK_MD_CORE:?Set VK_MD_CORE}"
${PYTHON:-python3} smoke_rooms_md.py --core "$VK_MD_CORE" --out "${VK_SMOKE_OUT:-../scratchpad/smoke-rooms}"
