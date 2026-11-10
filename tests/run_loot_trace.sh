#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_loot.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -O2 -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/loot_bridge.c sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
${PYTHON:-python3} tools/build_loot_data.py --check
${PYTHON:-python3} tools/verify_loot_trace.py "$BIN"
