import json
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class TestCrouch(unittest.TestCase):
    def test_tiles(self):
        header=(ROOT/'sgdk/src/generated/simon_crouch_md.h').read_text()
        assert 'VK_CROUCH_FRAME_COUNT 4' in header
        assert 'VK_CROUCH_TILES_PER_FRAME 32' in header
        assert len((ROOT/'sgdk/src/generated/simon_crouch_md.c').read_text().split('0x'))==4*32*8+1
    def test_source_is_rom_derived(self):
        from PIL import Image
        im=Image.open(ROOT/'sgdk/src/generated/simon_crouch_preview.png').convert('RGBA')
        assert im.size[1] == 32
        assert im.getbbox() is not None
        meta=json.loads((ROOT/'sgdk/src/generated/simon_crouch_manifest.json').read_text())
        assert meta['asset_source'] == 'MSX2 original, position/mask tables'
        assert meta['candidate_state'] == 'crouch / crouch-whip'
        # Every frame shares the same component anchor; no per-frame
        # compression that might detach Simon's torso from his legs.
        assert all(not f['gap_adjustments'] for f in meta['frame_assignments'])
        assert len({f['bounds']['max_y_exclusive'] for f in meta['frame_assignments']}) <= 2
    def test_frames_share_anchor(self):
        # Every crouch frame keeps the same 64x32 buffer anchor as the attack frames.
        assert all(f['canvas_offset_xy'][1] >= 0 for f in json.loads((ROOT/'sgdk/src/generated/simon_crouch_manifest.json').read_text())['frame_assignments'])
if __name__=='__main__':unittest.main()
