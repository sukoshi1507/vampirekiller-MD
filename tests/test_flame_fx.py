"""Check generated Mega Drive visual effects for tile sizes and regressions."""
from pathlib import Path
import re
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
header=root/'sgdk/src/generated/flame_fx_md.h'
original=header.read_bytes()
subprocess.run([sys.executable,str(root/'tools/build_flame_fx.py')],check=True)
assert original==header.read_bytes(), 'Generator output is not deterministic'
text=header.read_text()
words=[int(v,16) for v in re.findall(r'0x([0-9A-F]{8})UL',text)]
assert len(words)==224
assert any(v for v in words)
assert len(set(tuple(words[i:i+32]) for i in range(0,224,32)))==7
main=(root/'sgdk/src/main.c').read_text()
assert 'VK_FLAME_FRAMES + ((12 - flame_burst) / 4)' in main
assert 'VDP_loadTileData(vk_flame_fx_tiles' in main
assert 'vk_whip_tick(&whip' in main
print('V16: deterministic graphics, 4 flame frames, 3 impact frames, render wiring OK')
