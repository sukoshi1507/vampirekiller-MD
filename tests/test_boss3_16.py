"""16-bit bat boss of the stage 3 (tools/build_boss3_16.py, research/gfx16.md): same layout as the MSX arrays, and a PAL2
for the boss room that keeps the MSX colours other content uses and already holds the colours the orb writes."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'
sys.path.insert(0, str(ROOT / 'tools'))

import build_enemies16 as B  # noqa: E402


def u32(path, name):
    m = re.search(r'%s\[\d+\] = \{(.*?)\};' % name, path.read_text(), re.S)
    return [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{1,8})', m.group(1))]


def u16(path, name):
    m = re.search(r'%s\[\d+\] = \{(.*?)\};' % name, path.read_text(), re.S)
    return [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]


def used(words):
    return {(w >> (4 * k)) & 15 for w in words for k in range(8)} - {0}


def test_layout_of_the_msx_arrays():
    boss, orb = u32(GEN / 'boss_md.c', 'vk_boss_tiles'), u32(GEN / 'boss_md.c', 'vk_orb_tiles')
    b16, o16 = u32(GEN / 'boss3_16_md.c', 'vk_boss16_tiles'), u32(GEN / 'boss3_16_md.c', 'vk_orb16_tiles')
    assert len(b16) == len(boss) == 256 and len(o16) == len(orb) == 384
    for c in range(8):                                   # 7 cells of the two poses and the fireball: all drawn
        assert any(b16[c * 32:(c + 1) * 32]), c
    for p in range(3):
        assert any(o16[p * 128:(p + 1) * 128]), p


def test_palette_of_the_boss_room():
    pal = u16(GEN / 'boss3_16_md.c', 'vk_boss16_palette')
    room = B.room_palettes()[19]                         # stage 3, room 5
    assert pal[0] == 0
    assert [pal[i] for i in B.FIXED] == [room[i] for i in B.FIXED]   # colours other PAL2 content uses
    assert (pal[4], pal[6]) == (0x006, 0x00A)            # what the orb writes (actors.c): the rewrite changes nothing
    act = (ROOT / 'sgdk/src/actors.c').read_text()
    assert 'red[2] = { 0x006, 0x00A }' in act
    words = u32(GEN / 'boss3_16_md.c', 'vk_boss16_tiles') + u32(GEN / 'boss3_16_md.c', 'vk_orb16_tiles')
    assert used(words) <= set(range(1, 16))
