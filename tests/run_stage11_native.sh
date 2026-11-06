#!/bin/sh
# Stage 11 only; native tests of extracted ROM data and Z80-emulated vectors.
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage11.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
MAPS="sgdk/src/generated/stage2_maps.c sgdk/src/generated/stage3_maps.c sgdk/src/generated/stage4_maps.c sgdk/src/generated/stage5_maps.c sgdk/src/generated/stage6_maps.c sgdk/src/generated/stage7_maps.c sgdk/src/generated/stage8_maps.c sgdk/src/generated/stage9_maps.c sgdk/src/generated/stage10_maps.c sgdk/src/generated/stage11_maps.c"
COMMON="sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS"
CFLAGS="-std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include"
echo '[stage11] extracted room links, candles, fixed objects, enemies and door'
${CC:-cc} $CFLAGS tests/test_stage11_rooms.c sgdk/src/room_manager.c sgdk/src/platforms.c sgdk/src/generated/candles_md.c $MAPS -o "$BIN"
"$BIN"
echo '[stage11] eagle and carried flea: ROM Z80 vectors and channel'
${CC:-cc} $CFLAGS -Itests/native_include -Itests tests/test_stage11.c sgdk/src/generated/intro_assets.c $COMMON -o "$BIN"
"$BIN"
echo '[stage11] dragon: ROM Z80 vectors and aim'
${CC:-cc} $CFLAGS -Itests/native_include -Itests tests/test_dragon.c sgdk/src/generated/intro_assets.c $COMMON -o "$BIN"
"$BIN"
echo '[stage11] candle objects, key, door and stage exit (shared stages)'
${CC:-cc} $CFLAGS tests/test_candles.c sgdk/src/candles.c sgdk/src/whip.c sgdk/src/generated/candles_md.c $COMMON -o "$BIN"
"$BIN"
${CC:-cc} $CFLAGS -Itests/native_include tests/stage_exits.c sgdk/src/items.c sgdk/src/candles.c sgdk/src/whip.c sgdk/src/generated/candles_md.c sgdk/src/generated/intro_assets.c $COMMON -o "$BIN"
"$BIN"
echo 'Stage 11 targeted native suite: PASS (not an emulator/render/audio certification)'
