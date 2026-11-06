#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_stage12_secret.XXXXXX)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -Isgdk/src tests/test_stage12_secret.c sgdk/src/stage12_secret.c sgdk/src/generated/stage12_foundations.c -o "$BIN"
"$BIN"
