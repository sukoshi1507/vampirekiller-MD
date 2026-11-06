#!/usr/bin/env python3
"""Static checks for castle-stage progression; does not simulate gameplay."""
from pathlib import Path
import re, json, sys
root=Path(__file__).resolve().parents[1]
main=(root/'sgdk/src/main.c').read_text()
items=(root/'sgdk/src/items.c').read_text()
rooms=(root/'sgdk/src/room_manager.c').read_text()
counts={}
for n in range(1,11):
    if n==1:
        m=re.search(r'vk_stage1_links\[VK_STAGE1_ROOM_COUNT\]\s*=\s*\{(.*?)\};',rooms,re.S)
        count=len(re.findall(r'\{[^{}]*\}',m.group(1))) if m else None
    else:
        h=(root/f'sgdk/src/generated/stage{n}_maps.h').read_text()
        m=re.search(rf'VK_STAGE{n}_ROOM_COUNT\s+(\d+)',h)
        count=int(m.group(1)) if m else None
    counts[n]=count
checks={
 'no_stage3_intercept':'show_stage_end' not in main and 'stage_end_timer' not in main,
 'normal_increment':'vk_game_enter_stage(&vk_game, next_stage)' in main and 'vk_game.stage + 1' in main,
 'stage10_guard':'item == VK_ITEM_STAGE_EXIT && vk_game.stage >= 10' in main,
 'all_stage_links_declared':all(f'vk_stage{n}_links' in rooms for n in range(1,11)),
 'all_stage_counts_declared':all(v is not None and v>0 for v in counts.values()),
}
key_stages=len(json.loads((root/'research/stages-1-10/MSX2_STAGE_EXITS.json').read_text()))
report={'checks':checks,'stage_room_counts':counts,'stages_with_defined_key_and_exit':list(range(1,key_stages+1)), 'stages_without_defined_key_and_exit':list(range(key_stages+1,11)), 'note':'Static source audit only; no SGDK compilation or emulator validation.'}
print(json.dumps(report,indent=2))
sys.exit(0 if all(checks.values()) else 1)
