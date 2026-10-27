#!/usr/bin/env python3
"""Regression test for the SGDK facing convention in ROM-derived attack sprites."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'sgdk/src/main.c').read_text()
# main.c derives the body facing from crouch_facing (whip.facing during an attack,
# player->facing otherwise), then reverses the source half order when facing is left.
assert 'const u16 crouch_facing = attack_frame ? whip.facing : player->facing;' in s
assert 'const u16 facing = (u16)crouch_facing;' in s
assert 'const u16 source_half = facing ? (u16)(1 - i) : i;' in s
assert 'TILE_ATTR_FULL(PAL2, TRUE, FALSE, facing,\n                               base + source_half * tiles_per_half' in s
assert 'const u16 source_half = whip.facing ? i :' not in s
print('Attack orientation source regression: PASS (right=normal, left=mirrored)')
