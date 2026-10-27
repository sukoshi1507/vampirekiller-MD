#!/bin/sh
# Non-regression gameplay scenarios on the built MD ROM (Genesis Plus GX, headless).
# Usage: tests/run_md_scenarios.sh [OUT_DIR]   (build first: cd sgdk && make GDK=...)
# Outputs (PNG, dumps.json/csv, result.json) go to OUT_DIR/<scenario>/ (default scratchpad/md).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/scratchpad/md"}
status=0
for scenario in "$ROOT"/tests/md_scenarios/*.txt; do
    name=$(basename "$scenario" .txt)
    echo "== $name"
    python3 "$ROOT/tools/md_harness.py" "$scenario" --out "$OUT/$name" --quiet || status=1
done
[ "$status" -eq 0 ] && echo "ALL MD SCENARIOS PASSED" || echo "MD SCENARIOS FAILED"
exit "$status"
