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
        assert 'VK_CROUCH_FRAME_COUNT 5' in header
        assert 'VK_CROUCH_TILES_PER_FRAME 16' in header
        assert len((ROOT/'sgdk/src/generated/simon_crouch_md.c').read_text().split('0x'))==641
    def test_source_is_rom_derived(self):
        from PIL import Image
        im=Image.open(ROOT/'sgdk/src/generated/simon_crouch_preview.png').convert('RGBA')
        assert im.size == (32 * 5,32)
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
        self.assertIn('(attack_frame && whip.crouched)',src)
        self.assertIn('crouch_x_right[5] = { -24, -24, -24, -24, -24 }',src)
        self.assertIn('crouch_x_left[5]  = {  -8,  -8,  -8,  -8,  -8 }',src)
        self.assertIn('crouch_y_offset[5] = { -25, -25, -25, -25, -25 }',src)
        for index, yoff in enumerate((-25,-25,-25,-25,-25)):
            im=Image.open(ROOT/f'sgdk/src/generated/crouch-candidate-{index}.png').convert('RGBA')
            bbox=im.getbbox()
            assert bbox is not None
            # All crouch frames must keep their last occupied row at y-1.
            assert yoff + bbox[3]-1 == -1, (index,yoff,bbox)
            # For the right-facing sprite, body-side pixels should occupy x-8..x+7.
            if index in (0,1,4):
                assert -24+bbox[0] == -8
                assert -24+bbox[2]-1 == 7
            # Reversing a 32-wide source sprite retains its body anchor.
            if index in (0,1,4):
                mirrored_left = -8 + (32 - bbox[2])
                mirrored_right = -8 + (31 - bbox[0])
                assert (mirrored_left,mirrored_right) == (-8,7)
    def test_wiring(self):
        m=(ROOT/'sgdk/src/main.c').read_text()
        assert 'vk_whip_crouch_visual_step' in m
        assert 'VK_CROUCH_TILES_PER_FRAME' in m
        assert 'crouch_mode' in m
if __name__=='__main__':unittest.main()
