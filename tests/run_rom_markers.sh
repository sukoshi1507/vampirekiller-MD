#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_markers.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -std=c99 -O2 -Wall -Wextra -Werror -Itests/native_include -Isgdk/src tests/rom_markers_bridge.c sgdk/src/generated/intro_assets.c sgdk/src/health.c sgdk/src/items.c sgdk/src/room_freeze.c sgdk/src/candles.c sgdk/src/whip.c sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage*_maps.c -o "$BIN"
${PYTHON:-python3} tools/build_room_freeze.py --check
${PYTHON:-python3} tools/verify_rom_markers.py "$BIN"
