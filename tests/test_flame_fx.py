"""Check generated Mega Drive visual effects for tile sizes and regressions."""
from pathlib import Path
import importlib.util
import re
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
if importlib.util.find_spec('z80') is None:
    raise SystemExit('test_flame_fx needs the z80 module: run with an interpreter where '
                     '`pip install -r requirements.txt` was done (e.g. a venv), not the bare system python.')
header=root/'sgdk/src/generated/flame_fx_md.h'
original=header.read_bytes()
subprocess.run([sys.executable,str(root/'tools/build_flame_fx.py')],check=True)
assert original==header.read_bytes(), 'Generator output is not deterministic'
text=header.read_text()
words=[int(v,16) for v in re.findall(r'0x([0-9A-F]{8})UL',text)]
assert len(words)==224
assert any(v for v in words)
frames=[tuple(words[i:i+32]) for i in range(0,224,32)]
# ROM states A (VRAM x 48) and B (VRAM x 32) only: 4 flame frames A,A,B,B and 3 burst frames A,B,A.
assert len(set(frames))==2, 'Expected exactly two ROM flame states'
a,b=frames[0],frames[2]
assert a!=b and frames==[a,a,b,b,a,b,a]
assert 'NOT ROM' not in text and 'from the reference ROM' in text
assert '"status": "decoded"' in (root/'sgdk/src/generated/flame_fx_manifest.json').read_text()
main=(root/'sgdk/src/main.c').read_text()
# flame_burst was removed by 2db4cbb: the courtyard flame cycles over the ROM states only.
assert '(flame_clock >> 3) & (VK_FLAME_FRAMES - 1)' in main
assert 'FLAME_TILE_INDEX + cycle * VK_FLAME_TILES_PER_FRAME' in main
assert 'flame_burst' not in main
assert 'VDP_loadTileData(vk_flame_fx_tiles' in main
assert 'vk_whip_tick(&whip' in main
print('V16: deterministic ROM-extracted flame graphics (2 states, 4 flame frames, 3 burst frames), render wiring OK')
