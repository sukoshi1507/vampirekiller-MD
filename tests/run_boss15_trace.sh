#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_boss15.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -std=c99 -O2 -Wall -Wextra -Werror -Isgdk/src tests/boss15_bridge.c sgdk/src/enemies.c sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/stage*_maps.c -o "$BIN"
${PYTHON:-python3} tools/build_boss15_md.py --check
${PYTHON:-python3} tools/verify_boss15_trace.py "$BIN"
