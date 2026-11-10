#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_items.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -std=c99 -O2 -Wall -Wextra -Werror -Itests/native_include -Isgdk/src tests/pools_bridge.c sgdk/src/generated/intro_assets.c sgdk/src/boss.c sgdk/src/weapons.c sgdk/src/item_effects.c sgdk/src/stage_map.c sgdk/src/candles.c sgdk/src/wall.c sgdk/src/whip.c sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage_map_md.c sgdk/src/generated/stage*_maps.c sgdk/src/generated/stage1[2-8]_foundations.c -o "$BIN"
${PYTHON:-python3} tools/verify_touch09.py "$BIN" "$@"
