#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage14_objects.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Isgdk/src/generated \
 tests/test_stage14_scene.c sgdk/src/stage14_scene.c \
 sgdk/src/generated/stage14_objects.c sgdk/src/generated/stage14_foundations.c -o "$BIN"
"$BIN"
