#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_late_spawn.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_late_enemy_spawn.c \
 sgdk/src/enemies.c sgdk/src/player.c sgdk/src/generated/player_assets.c \
 sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
