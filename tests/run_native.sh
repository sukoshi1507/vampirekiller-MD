#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_gameplay.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_gameplay.c sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_physics.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_platforms.c sgdk/src/platforms.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_crushers.c sgdk/src/crushers.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_whip.c sgdk/src/whip.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_targets.c sgdk/src/targets.c sgdk/src/whip.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_room_manager.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_enemies.c sgdk/src/enemies.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_health.c sgdk/src/health.c sgdk/src/enemies.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_items.c sgdk/src/items.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_candles.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_weapons.c sgdk/src/weapons.c sgdk/src/enemies.c \
    sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c \
    sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_psg_core.c sgdk/src/psg_core.c \
    sgdk/src/generated/msx_sound_md.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_room_freeze.c sgdk/src/room_freeze.c -o "$BIN"
"$BIN"
MAPS="sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c"
# Stages 1-10: exits, room links and enemy tables against the generated maps.
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include tests/enemy_integration.c sgdk/src/enemies.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests -Itests/native_include tests/test_stage7.c sgdk/src/enemies.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include tests/room_transitions.c sgdk/src/items.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include tests/stage_exits.c sgdk/src/items.c sgdk/src/candles.c sgdk/src/whip.c sgdk/src/gameplay.c sgdk/src/generated/intro_assets.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_boss.c sgdk/src/boss.c sgdk/src/enemies.c sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_wall.c sgdk/src/wall.c sgdk/src/enemies.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c \
    sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage_map.c sgdk/src/stage_map.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c \
    sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage_map_md.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_stage8.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests tests/test_stage9.c sgdk/src/enemies.c sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_stage9_enemies.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_ghost.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_fish3.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_death_flame.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
echo 'Native gameplay, physics, whip, room manager, enemy, health, item, candle, weapon, PSG, room freeze, wall, stage map, stage 8 object, stage 9 object, stage 10 fishman, death flame and boss and stage 1-10 tests: PASS'
