#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage10.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
MAPS="sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c"
echo "[stage10] tests/test_stage10_rooms.c"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage10_rooms.c sgdk/src/room_manager.c sgdk/src/platforms.c sgdk/src/generated/candles_md.c $MAPS -o "$BIN"
"$BIN"
echo "[stage10] tests/test_platforms.c"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_platforms.c sgdk/src/platforms.c -o "$BIN"
"$BIN"
echo "[stage10] tests/test_enemies.c"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_enemies.c sgdk/src/enemies.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c -o "$BIN"
"$BIN"
echo "[stage10] tests/test_candles.c"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_candles.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c -o "$BIN"
"$BIN"
echo "[stage10] tests/test_fish3.c"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_fish3.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
echo "[stage10] tests/stage_exits.c"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include tests/stage_exits.c sgdk/src/items.c sgdk/src/candles.c sgdk/src/whip.c sgdk/src/gameplay.c sgdk/src/generated/intro_assets.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
echo "Stage 10: all targeted native checks passed (ROM-trace-derived checks; not an MD emulator validation)"
