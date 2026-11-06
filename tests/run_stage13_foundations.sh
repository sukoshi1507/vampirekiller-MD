#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage13_foundations.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src \
    tests/test_stage13_foundations.c sgdk/src/generated/stage13_foundations.c -o "$BIN"
"$BIN"
echo 'NOTE: static ROM foundation tables only, not a playable stage.'
