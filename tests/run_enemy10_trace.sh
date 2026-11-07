#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_enemy10.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -std=c99 -O2 -Wall -Wextra -Werror -Isgdk/src tests/enemy10_bridge.c sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/stage*_maps.c -o "$BIN"
${PYTHON:-python3} tools/verify_enemy10_trace.py "$BIN"
