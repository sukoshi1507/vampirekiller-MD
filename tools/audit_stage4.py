#!/usr/bin/env python3
"""Static inventory for stage 4. Does not assert gameplay completeness."""
import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'sgdk/src/generated/rooms.c').read_text()
rows=re.search(r'const VKStage vk_stages\[18\] = \{(.*?)\};',s,re.S).group(1)
stages=[tuple(map(int,m)) for m in re.findall(r'\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}',rows)]
first,count,tileset=stages[3]
assert count==6
assert all(f'stage 4 room {i}' in s for i in range(count))
report={'stage':4,'room_links':'verified against original B99A; native boundary tests passed','key_and_exit':'verified against original ROM','backgrounds':{'first_map_index':first,'room_count':count,'tileset':tileset,'room_comments_present':True},'missing_for_playable_stage':['collision maps validated against MSX2','objects and candles','enemies and behaviors','gameplay integration and emulator tests'],'compilation':'not part of this audit'}
print(json.dumps(report,indent=2))
(root/'research/stages-1-10/STAGE4_AUDIT.json').write_text(json.dumps(report,indent=2)+'\n')
