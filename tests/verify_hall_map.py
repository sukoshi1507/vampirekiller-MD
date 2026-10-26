#!/usr/bin/env python3
"""Validate the checked-in hall collision map against room 1/0 artwork tiles."""
import re
from pathlib import Path
root = Path(__file__).resolve().parents[1] / 'sgdk/src/generated'
player = (root / 'player_assets.c').read_text()
rooms = (root / 'rooms.c').read_text()
match = re.search(r'vk_player_room\[768\]\s*=\s*\{([^}]*)\}', player, re.S)
assert match
hall = [int(v) for v in re.findall(r'\d+', match.group(1))]
match = re.search(r'vk_maps\[\]\[704\]\s*=\s*\{\s*\{\s*/\* stage 1 room 0 \*/(.*?)\}', rooms, re.S)
assert match
visible = [int(v) for v in re.findall(r'\d+', match.group(1))]
assert len(hall) == 768 and len(visible) == 704
assert hall[64:] == visible, 'Hall collision tiles differ from rendered stage 1 room 0'
print('Hall collision map: 704/704 visible tiles match stage 1 room 0; top 64 preserved')
