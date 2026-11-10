#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_trade.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -std=c99 -O2 -Wall -Wextra -Werror -Itests/trade_include -Isgdk/src tests/trade_bridge.c tests/trade_view_bridge.c sgdk/src/generated/object_tiles_md.c sgdk/src/candles.c sgdk/src/whip.c sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage*_maps.c -o "$BIN"
${PYTHON:-python3} tools/build_trade_data.py --check
${PYTHON:-python3} tools/verify_trade_trace.py "$BIN"
