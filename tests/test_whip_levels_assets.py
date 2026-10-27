"""Offline checks for the generated whip level 1 and level 2 SGDK resources."""
import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
GEN = ROOT / 'sgdk/src/generated'


def _words(name):
    text = (GEN / f'{name}.c').read_text()
    return [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{8})UL', text)]


def _palette(name):
    text = (GEN / f'{name}.c').read_text()
    block = re.search(r'palette\[16\] = \{(.*?)\};', text, re.S).group(1)
    return [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{4})', block)]


class WhipLevelAssetTest(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads((GEN / 'whip_levels_manifest.json').read_text())

    def test_counts_and_header(self):
        for level in (1, 2):
            words = _words(f'whip_level{level}_md')
            self.assertEqual(len(words), 6 * 32 * 8)
            header = (GEN / f'whip_level{level}_md.h').read_text()
            self.assertIn(f'VK_WHIP_LEVEL{level}_FRAME_COUNT 6', header)
            self.assertIn(f'VK_WHIP_LEVEL{level}_TILES_PER_FRAME 32', header)
            self.assertIn(f'vk_whip_level{level}_tiles[{6 * 32 * 8}]', header)

    def test_palette_is_md_format(self):
        for level in (1, 2):
            pal = _palette(f'whip_level{level}_md')
            self.assertEqual(len(pal), 16)
            # MD colour word: 0000BBB0GGG0RRR0 (no bit outside the 9 used bits)
            self.assertTrue(all((v & ~0x0EEE) == 0 for v in pal))

    def test_nibbles_are_colour_indices_up_to_three(self):
        # 4 bpp words, only colours 0..3 are produced by the layer planes.
        for level in (1, 2):
            for v in _words(f'whip_level{level}_md'):
                for shift in range(0, 32, 4):
                    self.assertLessEqual((v >> shift) & 0xF, 3)

    def test_manifest_frames_and_status(self):
        self.assertEqual(self.manifest['canvas']['origin_x'], {'right': -24, 'left': -39})
        for level, data in self.manifest['levels'].items():
            self.assertEqual(len(data['frames']), 6)
            self.assertEqual([f['facing'] for f in data['frames']], ['right'] * 3 + ['left'] * 3)
            for f in data['frames']:
                self.assertEqual(f['status'], 'candidate')
                self.assertEqual(f['tiles'], 32)
                # Only the 12 and 27 upper compositions have more mask layers than positions.
                self.assertEqual(bool(f['unplaced_layers']), f['upper_index'] in (12, 27))
                self.assertTrue(all(u['part'] == 'upper' for u in f['unplaced_layers']))
                for part in ('lower', 'upper'):
                    c = f['components'][part]
                    self.assertEqual(c['pattern_file_offset'], f"{int(c['pattern_file_offset'], 16):05X}")
                    self.assertIn(c['index'], range(32))

    def test_palettes_are_the_stage1_room0_palette_used_by_the_wired_body(self):
        # main.c draws the whip body on PAL2, loaded with vk_palettes[0] (stage 1 room 0).
        rooms = (GEN / 'rooms.c').read_text()
        block = re.search(r'vk_palettes\[\]\[16\] = \{\s*\{(.*?)\}', rooms, re.S).group(1)
        room0 = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{4})', block)]
        self.assertEqual(len(room0), 16)
        for level in (1, 2):
            self.assertEqual(_palette(f'whip_level{level}_md'), room0)

    def test_palette_source_and_index0(self):
        self.assertIn('transparent', self.manifest['index0'])
        self.assertEqual(len(self.manifest['palette_rgb333']), 16)

    def test_previews_are_indexed_with_transparency(self):
        from PIL import Image
        for level, data in self.manifest['levels'].items():
            for f in data['frames']:
                img = Image.open(ROOT / f['png'])
                self.assertEqual(img.mode, 'P')
                self.assertEqual(img.size, (64, 32))
                self.assertEqual(img.info.get('transparency'), 0)
                self.assertLessEqual(len(set(img.tobytes())), 4)

    def test_level0_reference_checked(self):
        # The generator refuses to write if the same code path does not reproduce simon_attack_md.c.
        self.assertEqual(self.manifest['level0_reference_words_checked'], 3 * 32 * 8)


if __name__ == '__main__':
    unittest.main()
