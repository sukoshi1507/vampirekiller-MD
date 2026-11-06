"""Stages 10-18: C maps against the original Z80 routine 4F83, C links against the ROM table B9D3."""
import importlib.util
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from probe import reference_rom  # noqa: E402


@unittest.skipUnless(importlib.util.find_spec('z80'), 'z80==1.2.0 is not installed')
@unittest.skipUnless(reference_rom().exists(), 'reference ROM is missing')
class StageMapsRomTests(unittest.TestCase):
    def test_maps_and_links_match_rom(self):
        from probe import load_rom
        import verify_stage_maps_rom as verify
        report = verify.check(load_rom(reference_rom()))
        self.assertEqual({s: n for s, (n, _) in report.items()}, {10: 9, 11: 6, 12: 12, 13: 12, 14: 8, 15: 10, 16: 10, 17: 12, 18: 10})
        for stage, (_, problems) in report.items():
            self.assertEqual(problems, [], f'stage {stage}')


if __name__ == '__main__':
    unittest.main()
