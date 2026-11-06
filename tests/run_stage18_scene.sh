#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage18_objects.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Isgdk/src/generated \
 tests/test_stage18_scene.c sgdk/src/stage18_scene.c \
 sgdk/src/generated/stage18_objects.c sgdk/src/generated/stage18_foundations.c -o "$BIN"
"$BIN"
