#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage14_foundations.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage14_foundations.c sgdk/src/generated/stage14_foundations.c -o "$BIN"
"$BIN"
echo 'Only the raw ROM stage14 tables were validated; not a playable stage.'
