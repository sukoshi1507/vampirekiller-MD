#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage16_objects.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Isgdk/src/generated \
 tests/test_stage16_scene.c sgdk/src/stage16_scene.c \
 sgdk/src/generated/stage16_objects.c sgdk/src/generated/stage16_foundations.c -o "$BIN"
"$BIN"
