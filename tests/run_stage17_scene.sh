#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage17_objects.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Isgdk/src/generated \
 tests/test_stage17_scene.c sgdk/src/stage17_scene.c \
 sgdk/src/generated/stage17_objects.c sgdk/src/generated/stage17_foundations.c -o "$BIN"
"$BIN"
