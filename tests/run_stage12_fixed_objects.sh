#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python tools/prepare_stage12_fixed_objects.py --check
BIN="$(mktemp /tmp/vk_s12_fixed.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
 tests/test_stage12_fixed_objects.c sgdk/src/generated/stage12_fixed_objects.c \
 sgdk/src/generated/stage12_object_records.c -o "$BIN"
"$BIN"
