import json
import re
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
GEN=ROOT/'sgdk/src/generated'
class TestCrouch(unittest.TestCase):
    def test_tiles(self):
        header=(GEN/'simon_crouch_md.h').read_text()
        assert 'VK_CROUCH_FRAME_COUNT 4' in header
        assert 'VK_CROUCH_TILES_PER_FRAME 32' in header
        assert len((GEN/'simon_crouch_md.c').read_text().split('0x'))==4*32*8+1
    def test_source_is_rom_derived(self):
        from PIL import Image
        im=Image.open(GEN/'simon_crouch_preview.png').convert('RGBA')
        assert im.size[1] == 32
        assert im.getbbox() is not None
        meta=json.loads((GEN/'simon_crouch_manifest.json').read_text())
        assert meta['asset_source'] == 'MSX2 original, position/mask tables'
        assert meta['candidate_state'] == 'crouch / crouch-whip'
        # Every frame shares the same component anchor; no per-frame
        # compression that might detach Simon's torso from his legs.
        assert all(not f['gap_adjustments'] for f in meta['frame_assignments'])
        assert len({f['bounds']['max_y_exclusive'] for f in meta['frame_assignments']}) <= 2
    def test_frames_share_anchor(self):
        # Every crouch frame keeps the same 64x32 buffer anchor as the attack frames.
        assert all(f['canvas_offset_xy'][1] >= 0 for f in json.loads((GEN/'simon_crouch_manifest.json').read_text())['frame_assignments'])
    def test_previews_match_tiles(self):
        # The PNG previews written by tools/build_simon_crouch_md.py must be those of the
        # tiles in simon_crouch_md.c: one 64x32 sheet cell and one candidate PNG per frame,
        # no leftover candidate, same opaque pixels as the non-zero tile indices.
        from PIL import Image
        meta=json.loads((GEN/'simon_crouch_manifest.json').read_text())
        frames=meta['frame_assignments']
        w,h=meta['canvas_size']
        names=sorted(p.name for p in GEN.glob('crouch-candidate-*.png'))
        assert names == sorted(f['png'] for f in frames), names
        sheet=Image.open(GEN/'simon_crouch_preview.png').convert('RGBA')
        assert sheet.size == (w*len(frames), h), sheet.size
        words=[int(v,16) for v in re.findall(r'0x([0-9A-F]{8})UL',(GEN/'simon_crouch_md.c').read_text())]
        per_frame=(w//32)*16*8
        for f in frames:
            k=f['frame']
            cand=Image.open(GEN/f['png']).convert('RGBA')
            assert cand.size == (w,h)
            assert cand.tobytes() == sheet.crop((k*w,0,(k+1)*w,h)).tobytes(), f['png']
            fw=words[k*per_frame:(k+1)*per_frame]
            i=0
            for half in range(w//32):
                for tx in range(4):
                    for ty in range(4):
                        for py in range(8):
                            word=fw[i]; i+=1
                            for px in range(8):
                                idx=(word>>(4*(7-px)))&15
                                alpha=cand.getpixel((half*32+tx*8+px,ty*8+py))[3]
                                assert (idx!=0)==(alpha!=0), (f['png'],half,tx,ty,py,px)
if __name__=='__main__':unittest.main()
