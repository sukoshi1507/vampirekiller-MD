#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_late_runtime.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -O2 -std=c99 -Wall -Wextra -Werror -Itests/native_include -Isgdk/src tests/late_runtime_bridge.c sgdk/src/generated/intro_assets.c sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/candles.c sgdk/src/items.c sgdk/src/item_effects.c sgdk/src/wall.c sgdk/src/enemies.c sgdk/src/whip.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage*_maps.c -o "$BIN"
${PYTHON:-python3} tools/build_late_runtime.py --check
${PYTHON:-python3} tools/build_late_ghost.py --check
${PYTHON:-python3} tools/build_candles_md.py --check
${PYTHON:-python3} tools/extract_stage_exits.py --check > /dev/null
${PYTHON:-python3} tools/build_late_graphics.py --check
${PYTHON:-python3} tools/verify_late_runtime.py "$BIN"
