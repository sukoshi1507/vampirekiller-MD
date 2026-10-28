#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_gameplay.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_gameplay.c sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_physics.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_whip.c sgdk/src/whip.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_targets.c sgdk/src/targets.c sgdk/src/whip.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_room_manager.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_enemies.c sgdk/src/enemies.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_health.c sgdk/src/health.c sgdk/src/enemies.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_items.c sgdk/src/items.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_candles.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c -o "$BIN"
"$BIN"
echo 'Native gameplay, physics, whip, room manager, enemy, health, item and candle tests: PASS'
