"""HUD and text of the 16-bit graphics mode (tools/build_hud16.py, research/gfx16.md): the generated tables are up to date, use
only entries that exist in vk_item16_palette, and leave the HUD entries (1, 2, 8, 14, 15) as they were."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'
sys.path.insert(0, str(ROOT / 'tools'))

import build_hud16 as H  # noqa: E402


def palette():
    t = (GEN / 'items16_md.c').read_text()
    m = re.search(r'vk_item16_palette\[16\] = \{(.*?)\};', t, re.S)
    return [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]


def test_generated_files_are_up_to_date():
    h, c, _ = H.render()
    assert (GEN / 'hud16_md.h').read_text() == h
    assert (GEN / 'hud16_md.c').read_text() == c


def test_entries_exist_in_pal1_and_steel_is_steel():
    pal = palette()
    for e in H.entries_used():
        assert pal[e] != 0, e
    assert pal[2] == 0                              # the text background stays black
    assert pal[10] == 0x0ECC and pal[3] == 0x0864   # bluish greys of the mock-up (B')


def test_font_rows_and_chevrons():
    steel = H.steel_words()
    assert [i for i, w in enumerate(steel) if w] == [1, 2, 6, 7]
    chev = H.chevrons()
    assert len(chev) == 16 and chev[:8] != chev[8:]
    for w in chev:                                   # the same 14 as the font, recoloured by the rows
        assert {(w >> (4 * k)) & 15 for k in range(8)} <= {0, 14}


def test_frame_tiles_use_bevel_and_box_colours():
    w = H.frame_tiles()
    assert len(w) == 5 * 8
    assert {(x >> (4 * k)) & 15 for x in w for k in range(8)} == {0, H.RED, H.WHITE, H.BEVEL}
