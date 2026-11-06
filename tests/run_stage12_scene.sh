#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage12_scene.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src -Isgdk/src/generated \
 tests/test_stage12_scene.c sgdk/src/stage12_scene.c sgdk/src/stage12_object_view.c \
 sgdk/src/generated/stage12_candles.c sgdk/src/generated/stage12_fixed_objects.c \
 sgdk/src/generated/stage12_object_records.c sgdk/src/generated/stage12_gameplay_data.c \
 sgdk/src/generated/stage12_foundations.c -o "$BIN"
"$BIN"
