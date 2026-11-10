#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${VK_MD_CORE:?Set VK_MD_CORE to a Genesis Plus GX libretro shared library}"
${PYTHON:-python3} tools/verify_dracula_md.py --core "$VK_MD_CORE" --out "${VK_MD_OUT:-validation/dracula-md}"
