#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
${PYTHON:-python3} tools/trace_late_pools.py
