#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BIN="$(mktemp /tmp/vk_dracula.XXXXXX.so)"
trap 'rm -f "$BIN"' EXIT INT TERM
${CC:-cc} -shared -fPIC -std=c99 -O2 -Wall -Wextra -Werror -Isgdk/src tests/dracula_bridge.c -o "$BIN"
${PYTHON:-python3} tools/build_dracula_data.py --check
${PYTHON:-python3} tools/build_dracula_md.py --check
${PYTHON:-python3} tools/build_dracula_background.py --check
${PYTHON:-python3} tools/build_dracula_credits.py --check
${PYTHON:-python3} tools/verify_dracula_trace.py "$BIN"
