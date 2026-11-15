"""16-bit candles, objects, thrown weapons, flames and loot (tools/build_items16.py, research/gfx16.md): in 16-bit mode they
are drawn with PAL1, which then holds vk_item16_palette instead of the HUD palette (room_view.c hud_palette). Everything else
drawn with PAL1 (HUD bar, text, frames) must read the same colours: the entries it uses keep their value."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'
sys.path.insert(0, str(ROOT / 'tools'))

import build_items16 as B  # noqa: E402


def arrays(path):
    out = {}
    for m in re.finditer(r'const\s+(?:u32|u16|u8|unsigned long)\s+(\w+)\[[^\]]*\]\s*=\s*\{(.*?)\};', path.read_text(), re.S):
        out[m.group(1)] = [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', m.group(2))]
    return out


def used(words):
    return {(w >> (4 * k)) & 15 for w in words for k in range(8)} - {0}


def hud_palette():
    t = (ROOT / 'sgdk/src/room_view.c').read_text()
    m = re.search(r'hud_palette\[16\] = \{(.*?)\};', t, re.S)
    return [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', m.group(1))]


def test_hud_entries_keep_their_colours():
    pal, hud = arrays(GEN / 'items16_md.c')['vk_item16_palette'], hud_palette()
    assert len(pal) == len(hud) == 16 and pal[0] == 0
    for i in B.HUD_ENTRIES:
        assert pal[i] == hud[i], i


def test_pal1_users_only_read_hud_entries():
    # HUD bar (tiles of scene 6), MSX font and digits (text, hearts), frames of the boxes, SGDK font (colour 15 only).
    objs = arrays(GEN / 'object_tiles_md.c')
    scene = arrays(GEN / 'intro_assets.c')['scene_tiles_6']
    bar = [27, 28, 29, 41, 42, 43, 51, 52, 53]          # bar_source of hud.c
    seen = used(objs['vk_msx_font_tiles']) | used(objs['vk_hud_digit_tiles'])
    seen |= used([w for s in bar for w in scene[s * 8:s * 8 + 8]])
    seen |= {8, 14}                                       # frame_tiles of hud.c
    assert seen <= set(B.HUD_ENTRIES), seen


def test_layout_of_the_msx_arrays():
    new = arrays(GEN / 'items16_md.c')
    msx = B.msx_arrays()
    assert len(new['vk_item16_weapon_tiles']) == len(msx['wpn'])
    assert len(new['vk_item16_loot_tiles']) == len(msx['loot'])
    assert len(new['vk_item16_death_flame_tiles']) == len(msx['death'])
    cells = new['vk_item16_object_cell']
    assert len(cells) == len(msx['obj']) // 32
    stored = len(new['vk_item16_object_tiles']) // 32
    assert all(c == 0xFF or c < stored for c in cells)
    for name in ('vk_item16_object_tiles', 'vk_item16_weapon_tiles', 'vk_item16_loot_tiles', 'vk_item16_death_flame_tiles'):
        assert used(new[name]) <= set(range(1, 16)), name
    # every MSX object that has pixels and can be shown as a sprite has a picture (0x15: solid block, never drawn; 0x1F, 0x20: empty)
    for i in range(1, 0x23):
        cell = msx['obj'][(i - 1) * 32:i * 32]
        if any(cell) and i != 0x15:
            assert cells[i - 1] != 0xFF, hex(i)


def test_flames_are_the_loot_balls():
    # candle flame colours 0E (cell 0), 01, 08 of candle_flames_md.c are the white, skin and red balls: loot cells 3, 2, 1
    loot = arrays(GEN / 'items16_md.c')['vk_item16_loot_tiles']
    assert all(any(loot[c * 32:(c + 1) * 32]) for c in range(5))
    assert loot[0:32] == loot[4 * 32:5 * 32]
