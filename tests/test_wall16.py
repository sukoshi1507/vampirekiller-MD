"""16-bit old man of the walls and white key over the room (tools/build_wall16.py, item_view.c compose_key,
research/gfx16.md): same layout as the MSX pictures, only PAL1 entries (wall) and the three colours of the 16-bit white
key (key), so that msx[] of the room can take them to PAL0 entries without a conversion in the game."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'
sys.path.insert(0, str(ROOT / 'tools'))


def arrays(path):
    out = {}
    for m in re.finditer(r'const\s+(?:u32|u16|u8)\s+(\w+)\[[^\]]*\]\s*=\s*\{(.*?)\};', path.read_text(), re.S):
        out[m.group(1)] = [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', m.group(2))]
    return out


def nibbles(w):
    return [(w >> (4 * k)) & 15 for k in range(8)]


def test_wall16_has_the_msx_silhouette():
    msx = arrays(GEN / 'object_tiles_md.c')['vk_wall_frame_tiles']
    new = arrays(GEN / 'wall16_md.c')['vk_wall16_tiles']
    assert len(msx) == len(new) == 480
    for a, b in zip(msx, new):
        assert [bool(v) for v in nibbles(a)] == [bool(v) for v in nibbles(b)]


def test_wall16_is_up_to_date():
    import build_wall16 as B
    expected = [v for f in range(5) for v in B.words(B.draw(f))]
    assert arrays(GEN / 'wall16_md.c')['vk_wall16_tiles'] == expected


def test_white_key_uses_three_colours():
    # the key sprite is drawn with PAL1: the picture must only use black, white and light blue (2, 14, 10).
    items = arrays(GEN / 'items16_md.c')
    cell = items['vk_item16_object_cell'][0x18 - 1]
    assert cell != 0xFF
    words = items['vk_item16_object_tiles'][cell * 32:(cell + 1) * 32]
    assert {v for w in words for v in nibbles(w)} - {0} == {2, 10, 14}
