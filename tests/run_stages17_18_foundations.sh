#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stages17_18_foundations.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_stages17_18_foundations.c \
    sgdk/src/generated/stage17_foundations.c \
    sgdk/src/generated/stage18_foundations.c -o "$BIN"
"$BIN"
