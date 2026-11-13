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
    def test_level_frames(self):
        # Crouched attack of whip levels 1 (upper 9-11) and 2 (12-14), lower 6: 3 frames each,
        # right facing, in simon_crouch_levels_md.c (research/whip-variants.md, "Accroupi").
        header=(GEN/'simon_crouch_levels_md.h').read_text()
        assert 'VK_CROUCH_LEVEL_FRAME_COUNT 6' in header
        assert 'VK_CROUCH_LEVEL_TILES_PER_FRAME 32' in header
        assert len((GEN/'simon_crouch_levels_md.c').read_text().split('0x'))==6*32*8+1
        meta=json.loads((GEN/'simon_crouch_manifest.json').read_text())
        lv=meta['level_frame_assignments']
        assert [(f['level'],f['lower_index'],f['upper_index']) for f in lv]==\
            [(1,6,9),(1,6,10),(1,6,11),(2,6,12),(2,6,13),(2,6,14)]
        from PIL import Image
        def right(name):
            im=Image.open(GEN/name).convert('RGBA')
            return max(x for x in range(im.width) for y in range(im.height) if im.getpixel((x,y))[3])-24
        # Level 0 lash tip x +32, level 1 x +39, thrown pose arm up to x +12.
        assert right('crouch-candidate-3.png')==32
        assert right('crouch-level1-2.png')==39
        assert right('crouch-level2-2.png')==12
    def test_left_is_exact_mirror_of_right(self):
        # The engine draws the crouched left attack (C42E 16, upper +15) by flipping the right
        # frames: checked against the ROM compositions on the shared 64x32 canvas (origin x-24
        # right, x-39 left), levels 0, 1 and 2.
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from PIL import Image, ImageOps
        from probe import load_rom, reference_rom
        from extract_rooms import decode_room
        if not reference_rom().exists():
            self.skipTest('reference ROM not available')
        import build_simon_crouch_md as b
        rom=load_rom(reference_rom())
        palette=decode_room(rom,1,0)[0]['palette_rgb333']
        colors=[tuple(round(v*255/7) for v in rgb) for rgb in palette]
        def left(low,up):
            from decode_extended_simon import components, render
            lo=components(rom,low,'lower'); u=components(rom,up,'upper')
            lo['offsets_dy_dx']=[(dy-6,dx) for dy,dx in lo['offsets_dy_dx']]
            pic,_,bd=render([lo,u],colors)
            bb=pic.getbbox(); c=Image.new('RGBA',(64,32))
            c.alpha_composite(pic.crop(bb),(bd['min_x']+bb[0]+39,bd['min_y']+bb[1]+30))
            return c
        for r,l in [((6,6),(16,21)),((6,7),(16,22)),((6,8),(16,23)),((6,9),(16,24)),((6,10),(16,25)),
                    ((6,11),(16,26)),((6,12),(16,27)),((6,13),(16,28)),((6,14),(16,29))]:
            canvas,_=b.build_frame(rom,*r,colors,allow_unplaced=True)
            assert ImageOps.mirror(canvas).tobytes()==left(*l).tobytes(), (r,l)
if __name__=='__main__':unittest.main()
