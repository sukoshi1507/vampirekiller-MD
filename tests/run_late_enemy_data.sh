#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_late_enemy.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -pedantic -Isgdk/src tests/test_late_enemy_data.c sgdk/src/late_enemy_data.c -o "$BIN"
"$BIN"
