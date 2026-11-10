#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE to Genesis Plus GX libretro core}"
${PYTHON:-python3} tools/verify_item_md.py --core "$VK_MD_CORE" --out "${VK_ITEM_MD_OUT:-scratchpad/item-md}"
