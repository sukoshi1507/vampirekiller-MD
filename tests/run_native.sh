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
# Stage 11 extracted room/gameplay coverage (six rooms, key, fixed enemies).
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage11_rooms.c \
    sgdk/src/room_manager.c sgdk/src/platforms.c sgdk/src/generated/candles_md.c $MAPS -o "$BIN"
"$BIN"
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
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_boss3_ball.c sgdk/src/enemies.c sgdk/src/player.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_wall.c sgdk/src/wall.c sgdk/src/enemies.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c \
    sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage_map.c sgdk/src/stage_map.c sgdk/src/item_effects.c sgdk/src/wall.c sgdk/src/enemies.c sgdk/src/candles.c sgdk/src/whip.c \
    sgdk/src/gameplay.c sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/player_assets.c \
    sgdk/src/generated/stage1_collision.c sgdk/src/generated/candles_md.c sgdk/src/generated/stage_map_md.c sgdk/src/generated/stage1[2-8]_foundations.c $MAPS -o "$BIN"
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
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_stage11.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_dragon.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Itests/native_include -Itests tests/test_death_flame.c sgdk/src/gameplay.c \
    sgdk/src/player.c sgdk/src/room_manager.c sgdk/src/generated/intro_assets.c sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c $MAPS -o "$BIN"
"$BIN"
echo 'Native gameplay, physics, whip, room manager, enemy, health, item, candle, weapon, PSG, room freeze, wall, stage map, stage 8 object, stage 9 object, stage 10 fishman, stage 11 objects, death flame and boss and stage 1-11 tests: PASS'
# Archived MSX2 Z80 raw object slots for the unfinished stage 12.
sh ./tests/run_stage12_object_records.sh
sh ./tests/run_stage12_secret.sh
sh ./tests/run_secret_trace.sh
sh ./tests/run_loot_trace.sh
sh ./tests/run_candle_loot_trace.sh
sh ./tests/run_item_trace.sh
sh ./tests/run_late_runtime.sh
# Safe per-room access to original stage-12 Z80 object slots; no object effects inferred.
sh ./tests/run_stage12_object_view.sh
# Candle field mapping independently checked against 275 Z80 precedents (stages 0..11).
sh ./tests/run_stage12_candles.sh
# Five original type-83 fixed objects, decoded after validating 71 older entries.
sh ./tests/run_stage12_fixed_objects.sh
# Isolated state for all twelve stage-12 rooms (does not enable progression).
sh ./tests/run_stage12_scene.sh
# Independent stage 13/14 data/state adapters. No stage transition hook.
sh ./tests/run_stage13_scene.sh
sh ./tests/run_stage14_scene.sh

# Phase-2 isolated stage 15-16 room/object facades (not gameplay activation).
bash tests/run_stage15_scene.sh
bash tests/run_stage16_scene.sh
# Phase-2 isolated stage 17-18 room/object facades (no progression activation).
sh tests/run_stage17_scene.sh
sh tests/run_stage18_scene.sh

# Raw late-stage enemy channel flags, no spawn emulation.
sh ./tests/run_late_enemy_data.sh
sh ./tests/run_late_enemy_spawn.sh
# Explicit distinction between late-stage types supported by existing constructors
# and types whose Z80 behavior is not yet ported.
sh ./tests/run_late_enemy_coverage.sh

# Static ROM evidence only for future type-10 implementation (not a behavior test).
sh ./tests/run_enemy10_rom.sh
# Additional byte-accurate conditional fire-path check; no executed Z80 trace.
sh ./tests/run_enemy10_fire_path.sh
# Enemy 10h: static semantics of AF40 and projectile-slot preparation, no frame trace.
sh ./tests/run_enemy10_helpers.sh

# Eight late-game lot static audits: ROM/source consistency, NOT frame traces.
[ ! -x ./tests/run_pending_enemy09.sh ] || ./tests/run_pending_enemy09.sh
[ ! -x ./tests/run_pending_enemy10.sh ] || ./tests/run_pending_enemy10.sh
[ ! -x ./tests/run_pending_enemy12.sh ] || ./tests/run_pending_enemy12.sh
[ ! -x ./tests/run_pending_channels.sh ] || ./tests/run_pending_channels.sh
[ ! -x ./tests/run_pending_bosses.sh ] || ./tests/run_pending_bosses.sh
[ ! -x ./tests/run_pending_merchants.sh ] || ./tests/run_pending_merchants.sh
[ ! -x ./tests/run_pending_secrets.sh ] || ./tests/run_pending_secrets.sh
[ ! -x ./tests/run_pending_items.sh ] || ./tests/run_pending_items.sh

# Executed Z80 vs C type-10 gameplay and returning projectile.
sh ./tests/run_enemy10_trace.sh

sh ./tests/run_enemy09_trace.sh

sh ./tests/run_enemy12_trace.sh

sh ./tests/run_late_channels_trace.sh

sh ./tests/run_late_pools_trace.sh
sh ./tests/run_mixed_pools.sh
sh ./tests/run_sat_alias.sh
sh ./tests/run_d700_alias.sh
sh ./tests/run_c800_sat.sh
sh ./tests/run_legacy_revisit.sh
sh ./tests/run_touch09.sh
sh ./tests/run_boss15_pools.sh

# Stage12 boss, actual Z80 fight, boxes and end-of-stage script.
sh ./tests/run_boss12_trace.sh

# Stage15 body, projectiles, collision and ending versus actual Z80.
sh ./tests/run_boss15_trace.sh

# Stage18 Dracula and credits: standalone logic versus actual Z80.
sh ./tests/run_dracula_trace.sh

# Late walls/merchants versus actual Z80, all74 rooms.
sh ./tests/run_trade_trace.sh

# Optional compiled-ROM regression: explicit core path, SGDK ROM must exist.
if [ -n "${VK_MD_CORE:-}" ]; then
    sh ./tests/run_dracula_md.sh
    sh ./tests/run_trade_md.sh
    sh ./tests/run_secret_md.sh
    sh ./tests/run_item_md.sh
    sh ./tests/run_late_runtime_md.sh
    sh ./tests/run_loot_md.sh
    sh ./tests/run_candle_loot_md.sh
    sh ./tests/run_mixed_pools_md.sh
    sh ./tests/run_touch09_md.sh
    sh ./tests/run_c800_sat_md.sh
    sh ./tests/run_legacy_revisit_md.sh
    sh ./tests/run_late_scenarios.sh
fi
