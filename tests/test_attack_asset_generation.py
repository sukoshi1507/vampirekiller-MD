"""Offline checks for generated MSX2->Mega Drive attack resources."""
import json,re,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_simon_attack_md import PAIRS

class AssetTest(unittest.TestCase):
    def test_preselected_pairs_are_positioned(self):
        # The crosspair audit does not cover upper indices 7 and 8, so check the
        # generator's own placement: every layer of every traced pair must be placed.
        from decode_extended_simon import components,render
        from extract_rooms import decode_room
        from probe import load_rom,reference_rom
        rom=load_rom(reference_rom())
        palette=decode_room(rom,1,0)[0]['palette_rgb333']
        colors=[tuple(round(v*255/7) for v in c) for c in palette]
        for low,up in PAIRS:
            _,orphans,_=render([components(rom,low,'lower'),components(rom,up,'upper')],colors)
            self.assertFalse(orphans,f'unplaced layers for {low}/{up}')
    def test_generated_tiles_count_and_variation(self):
        source=(ROOT/'sgdk/src/generated/simon_attack_md.c').read_text()
        vals=[int(x,16) for x in re.findall(r'0x([A-Fa-f0-9]{8})UL',source)]
        self.assertEqual(len(vals),len(PAIRS)*256)
        # Traced frames 6, 7, 8 are distinct; the rest pose is not one of them.
        self.assertNotEqual(vals[:256],vals[256:512])
        self.assertNotEqual(vals[256:512],vals[512:768])
        self.assertNotEqual(vals[:256],vals[512:768])
        self.assertTrue(all((v & 0x88888888)==0 for v in vals))
    def test_manifest_unverified_timing(self):
        p=ROOT/'sgdk/src/generated/attack_candidates_manifest.json'
        m=json.loads(p.read_text())
        self.assertIs(m['phase_order_verified'],False)
        self.assertEqual(len(m['frame_assignments']),len(PAIRS))

if __name__=='__main__': unittest.main()
