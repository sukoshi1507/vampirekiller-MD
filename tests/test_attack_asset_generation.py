"""Offline checks for generated MSX2->Mega Drive attack resources."""
import json,re,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from build_simon_attack_md import PAIRS

class AssetTest(unittest.TestCase):
    def test_preselected_pairs_are_positioned(self):
        audit=json.loads((ROOT/'research/simon-assets/crosspairs/crosspair-manifest.json').read_text())
        table={(e['lower'],e['upper']):e for e in audit['entries']}
        for pair in PAIRS:
            self.assertEqual(table[pair]['status'],'fully_positioned')
    def test_generated_tiles_count_and_variation(self):
        source=(ROOT/'sgdk/src/generated/simon_attack_md.c').read_text()
        vals=[int(x,16) for x in re.findall(r'0x([A-Fa-f0-9]{8})UL',source)]
        self.assertEqual(len(vals),len(PAIRS)*256)
        self.assertEqual(vals[:256],vals[-256:])  # return to rest
        self.assertNotEqual(vals[256:512],vals[512:768])
        self.assertNotEqual(vals[:256],vals[256:512])
        self.assertNotEqual(vals[256:512],vals[512:768])
        self.assertTrue(all((v & 0x88888888)==0 for v in vals))
    def test_manifest_unverified_timing(self):
        p=ROOT/'sgdk/src/generated/attack_candidates_manifest.json'
        m=json.loads(p.read_text())
        self.assertIs(m['phase_order_verified'],False)
        self.assertEqual(len(m['frame_assignments']),len(PAIRS))

if __name__=='__main__': unittest.main()
