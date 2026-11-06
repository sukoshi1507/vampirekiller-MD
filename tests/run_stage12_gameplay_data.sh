#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage12_data.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_stage12_gameplay_data.c \
    sgdk/src/generated/stage12_foundations.c \
    sgdk/src/generated/stage12_gameplay_data.c -o "$BIN"
"$BIN"
echo 'NOTE: raw collision/object data checks only; SGDK runtime not enabled.'
