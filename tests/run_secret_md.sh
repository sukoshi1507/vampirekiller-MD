#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE to Genesis Plus GX libretro core}"
${PYTHON:-python3} tools/verify_secret_md.py --core "$VK_MD_CORE" --out "${VK_SECRET_MD_OUT:-scratchpad/secret-md}"
