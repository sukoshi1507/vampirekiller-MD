#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_gameplay.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_gameplay.c sgdk/src/gameplay.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_physics.c sgdk/src/player.c \
    sgdk/src/generated/player_assets.c sgdk/src/generated/stage1_collision.c -o "$BIN"
"$BIN"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_whip.c sgdk/src/whip.c -o "$BIN"
"$BIN"
echo 'Native gameplay, physics, and whip tests: PASS'
