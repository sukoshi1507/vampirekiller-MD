#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage16_foundations.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage16_foundations.c sgdk/src/generated/stage16_foundations.c -o "$BIN"
"$BIN"
echo 'Only stage 16 static ROM tables are validated; not gameplay.'
