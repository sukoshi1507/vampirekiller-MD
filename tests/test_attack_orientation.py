#!/usr/bin/env python3
"""Regression test for the SGDK facing convention in ROM-derived attack sprites."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'sgdk/src/main.c').read_text()
assert 'const u16 source_half = whip.facing ? (u16)(1 - i) : i;' in s
assert 'TILE_ATTR_FULL(PAL2, TRUE, FALSE, whip.facing,\n                               base + source_half' in s
assert 'const u16 source_half = whip.facing ? i :' not in s
print('Attack orientation source regression: PASS (right=normal, left=mirrored)')
