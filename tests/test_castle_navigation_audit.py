import sys
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from probe import load_rom, reference_rom
import audit_castle_navigation as audit

class NavigationAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = audit.build(load_rom(reference_rom()))

    def test_all_stages_present(self):
        self.assertEqual(self.data['stage_count'], 18)
        self.assertEqual(self.data['room_records'], 153)
        self.assertEqual(sum(self.data['stage_room_counts'].values()), 153)

    def test_no_invented_links(self):
        self.assertEqual(len(self.data['stage1_edge_geometry']), 16)
        self.assertTrue(all(r['links_status'] == 'not_verified' for r in self.data['rooms']))
        self.assertTrue(all('geometry_only' in e['status'] for e in self.data['stage1_edge_geometry']))

    def test_edge_probe_does_not_claim_solid_wall_passable(self):
        # Stage-1 room 4 left boundary is fully blocked at sampled heights.
        edge = next(e for e in self.data['stage1_edge_geometry'] if e['room'] == 4 and e['side'] == 'left')
        self.assertEqual(edge['candidate_count'], 0)

if __name__ == '__main__':
    unittest.main()
