#!/bin/sh
# No original ROM required at runtime: validates previously extracted raw ROM tables.
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage12_foundations.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_stage12_foundations.c sgdk/src/generated/stage12_foundations.c -o "$BIN"
"$BIN"
echo 'NOTE: This is NOT a playable stage-12 or a Z80 collision/boss verification.'
