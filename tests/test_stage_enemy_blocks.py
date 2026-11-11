"""Stages 7, 8 and 9 show the same ROM objects 0B (skeleton), 0C (flying creature) and the bone (D700 type 4), drawn by one
sprite code (enemies.c: HOPPER 64 + 8k / 68 + 8k, LEAPER 84 + 4k, BONE 98 + 4k) from the type 5 block of the stage.
Each stage block must hold those cells at those patterns (tester report 2026-11-11: the stage 7 block kept its own
layout, 0B was drawn with the cells of 0C and of the bone). The flea 0D takes A4..B0 in stages 7 and 9 (fleaBase)."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / 'sgdk/src/generated'


def block(stage):
    words = [int(x, 16) for x in re.findall(r'0x([0-9A-F]{8})', (GEN / f'stage{stage}_enemies_md.c').read_text())]
    assert len(words) == 1024
    return words


def cell(words, pattern):
    i = (pattern - 0x40) * 8
    return words[i:i + 32]


def test_shared_objects_same_cells():
    s7, s8, s9 = block(7), block(8), block(9)
    for pattern in range(0x64, 0xA4, 4):
        assert any(cell(s8, pattern)), hex(pattern)
        assert cell(s7, pattern) == cell(s8, pattern), ('stage 7', hex(pattern))
        assert cell(s9, pattern) == cell(s8, pattern), ('stage 9', hex(pattern))


def test_flea_cells():
    s7, s9 = block(7), block(9)
    for pattern in range(0xA4, 0xB4, 4):
        assert any(cell(s7, pattern)) and cell(s7, pattern) == cell(s9, pattern), hex(pattern)


def test_sprite_code_bases():
    src = (ROOT / 'sgdk/src/enemies.c').read_text()
    for needle in ('0x64 + 8 * (pose - 0x47)', '0x68 + 8 * (pose - 0x47)', '0x84 + 4 * (pose - 0x87)',
                   '0x98 + 4 * (pose - 0x4B)', 'fleaBase = v->stage == 7 || v->stage == 9 ? 0xA4 : 0x80;'):
        assert needle in src, needle
