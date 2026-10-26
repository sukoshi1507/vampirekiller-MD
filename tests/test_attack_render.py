"""Static regression checks: animation body and lash must render together."""
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class RenderTest(unittest.TestCase):
    def test_lash_not_suppressed_by_body_frame(self):
        src=(ROOT/'sgdk/src/main.c').read_text()
        self.assertIn('if (segment < count)',src)
        self.assertNotIn('if (!attack_frame && segment < count)',src)
    def test_facing_unchanged(self):
        src=(ROOT/'sgdk/src/main.c').read_text()
        self.assertIn('const u16 source_half = whip.facing ? (u16)(1 - i) : i;',src)
if __name__=='__main__': unittest.main()
