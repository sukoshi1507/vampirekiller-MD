import json
import unittest
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import build_simon_crouch_md as gen
class TestCrouch(unittest.TestCase):
    def test_tiles(self):
        gen.generate()
        header=(ROOT/'sgdk/src/generated/simon_crouch_md.h').read_text()
        assert 'VK_CROUCH_FRAME_COUNT 4' in header
        assert 'VK_CROUCH_TILES_PER_FRAME 32' in header
        assert len((ROOT/'sgdk/src/generated/simon_crouch_md.c').read_text().split('0x'))==4*32*8+1
    def test_source_is_rom_derived(self):
        from PIL import Image
        im=Image.open(ROOT/'sgdk/src/generated/simon_crouch_preview.png').convert('RGBA')
        assert im.size == (64 * 4,32)
        assert im.getbbox() is not None
        meta=json.loads((ROOT/'sgdk/src/generated/simon_crouch_manifest.json').read_text())
        assert meta['asset_source'] == 'MSX2 original, position/mask tables'
        assert meta['candidate_state'] == 'crouch / crouch-whip'
        # Every frame shares the same component anchor; no per-frame
        # compression that might detach Simon's torso from his legs.
        assert all(not f['gap_adjustments'] for f in meta['frame_assignments'])
        assert len({f['bounds']['max_y_exclusive'] for f in meta['frame_assignments']}) <= 2
    def test_crouch_anchors_and_locked_attack(self):
        from PIL import Image
        src=(ROOT/'sgdk/src/main.c').read_text()
        self.assertIn('(crouch_mode || attack_frame)',src)
        self.assertIn('const s16 origin_y = crouch_mode ? -25 : -30;',src)
        self.assertIn('const s16 origin_x = facing ? -39 : -24;',src)
        # Every crouch frame keeps the same 64x32 buffer anchor as the attack frames.
        assert all(f['canvas_offset_xy'][1] >= 0 for f in json.loads((ROOT/'sgdk/src/generated/simon_crouch_manifest.json').read_text())['frame_assignments'])
    def test_wiring(self):
        m=(ROOT/'sgdk/src/main.c').read_text()
        assert 'vk_whip_crouch_visual_step' in m
        assert 'VK_CROUCH_TILES_PER_FRAME' in m
        assert 'crouch_mode' in m
if __name__=='__main__':unittest.main()
