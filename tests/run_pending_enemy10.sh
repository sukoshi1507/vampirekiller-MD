#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tools/audit_pending_lots.py enemy10 --check
