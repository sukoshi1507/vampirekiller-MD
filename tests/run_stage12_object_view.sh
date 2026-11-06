#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_s12_view.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
  tests/test_stage12_object_view.c sgdk/src/stage12_object_view.c \
  sgdk/src/generated/stage12_object_records.c -o "$BIN"
"$BIN"
