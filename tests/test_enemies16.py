"""16-bit common enemies of block 1 (tools/build_enemies16.py, research/gfx16.md): in 16-bit mode PAL2 of the rooms of
stages 1 to 3 is vk_enemy16_palette. It may only change the entries that no other PAL2 content of those rooms uses
(objects, candles, flames, weapons, key, door, HUD boxes, text, frame of the item boxes): those keep their MSX colour.
The tiles keep the layout of the MSX arrays (the second layer of every cell pair is empty)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'
sys.path.insert(0, str(ROOT / 'tools'))

import build_enemies16 as B  # noqa: E402


def arrays(path):
    out = {}
    for m in re.finditer(r'const\s+(?:u32|unsigned long)\s+(\w+)\[[^\]]*\]\s*=\s*\{(.*?)\};', path.read_text(), re.S):
        out[m.group(1)] = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{1,8})', m.group(2))]
    return out


def used(words):
    return {(w >> (4 * k)) & 15 for w in words for k in range(8)} - {0}


# Tile data that stages 1 to 3 show with PAL2 / VK_PAL_OBJ in 16-bit mode, other than the enemies of block 1.
OTHER = ['door_key_md.c', 'candle_flames_md.c', 'death_flame_md.c', 'object_tiles_md.c', 'loot_md.c', 'weapons_md.c',
         'whip_level1_md.c', 'whip_level2_md.c', 'flame_fx_md.h']


def test_free_entries_unused_by_other_pal2_content():
    seen = set()
    for f in OTHER:
        for name, words in arrays(GEN / f).items():
            seen |= used(words)
    assert seen & set(B.FREE) == set(), seen & set(B.FREE)
    assert seen <= set(B.FIXED), seen - set(B.FIXED)
    # Frame of the item boxes of hall room 0 (scene 6, rows 1..3, columns 15..31), drawn with PAL2.
    text = (GEN / 'intro_assets.c').read_text()

    def a(name):
        m = re.search(r'const u(?:32|16) %s\[[^\]]*\] = \{(.*?)\};' % name, text, re.S)
        return [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', m.group(1))]
    tiles, cells = a('scene_tiles_6'), a('scene_map_6')
    frame = {cells[r * 32 + c] for r in (1, 2, 3) for c in range(15, 32)} - {0}
    assert used([w for t in frame for w in tiles[t * 8:t * 8 + 8]]) <= set(B.FIXED)


def test_palette_keeps_msx_colours_of_every_room():
    pal = arrays_u16(GEN / 'enemies16_md.c', 'vk_enemy16_palette')
    assert pal[0] == 0
    for room in B.room_palettes():
        assert [pal[i] for i in B.FIXED] == [room[i] for i in B.FIXED]


def arrays_u16(path, name):
    m = re.search(r'%s\[\d+\] = \{(.*?)\};' % name, path.read_text(), re.S)
    return [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]


def test_layout_of_the_msx_arrays():
    msx = arrays(GEN / 'enemy_sprites_md.c')
    new = arrays(GEN / 'enemies16_md.c')
    for key, (msx_name, cells, c_name) in B.ARRAYS.items():
        assert len(new[c_name]) == len(msx[msx_name]) == cells * 32, key
        # layer N + 4 (the second cell of each pair) stays empty
        for c in range(1, cells, 2):
            assert not any(new[c_name][c * 32:(c + 1) * 32]), (key, c)
        assert used(new[c_name]) <= set(range(1, 16))
        assert sum(1 for c in range(0, cells, 2) if any(new[c_name][c * 32:(c + 1) * 32])) >= 3, key
