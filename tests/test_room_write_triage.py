import sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from classify_room_transitions_z80 import classify
from probe import load_rom, reference_rom
class TestRoomWriteTriage(unittest.TestCase):
    def test_preserves_uncertainty(self):
        r=classify(load_rom(reference_rom()))
        self.assertEqual(len(r['writes']),9)
        self.assertEqual(r['verified_edges'],[])
        self.assertTrue(all(not w['transition_edge_confirmed'] for w in r['writes']))
    def test_false_positives_flagged(self):
        r=classify(load_rom(reference_rom()))
        false={w['file_offset'] for w in r['writes'] if w['triage']=='table_false_positive_likely'}
        self.assertEqual(false,{'1CE2B','1D0E6'})
if __name__=='__main__':unittest.main()
