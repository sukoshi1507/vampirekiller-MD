"""Check generated Mega Drive visual effects for tile sizes and regressions."""
from pathlib import Path
import importlib.util
import re
import subprocess
import sys
import unittest

root=Path(__file__).resolve().parents[1]
if importlib.util.find_spec('z80') is None:
    raise unittest.SkipTest('test_flame_fx needs the z80 module: run with an interpreter where '
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
print('V16: deterministic ROM-extracted flame graphics (2 states, 4 flame frames, 3 burst frames)')
