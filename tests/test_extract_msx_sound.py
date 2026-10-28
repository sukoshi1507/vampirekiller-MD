"""Extraction of the MSX2 sounds by running the ROM driver, and their conversion.

Needs the reference ROM (VK_ROM_PATH, see tools/probe.py) and the z80 module;
skipped otherwise. The expected values come from the openMSX GIWRTE trace
research/audio/long1-bios-giwrte.csv.gz (see research/audio-driver.md).
"""
import csv
import gzip
import sys
import unittest
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))

try:
    import extract_msx_sound as ext
    from probe import load_rom, reference_rom
    ROM = load_rom(reference_rom())
except Exception as error:  # noqa: BLE001 - missing ROM or z80 module
    ROM = None
    SKIP = str(error)

TRACE = ROOT / 'research/audio/long1-bios-giwrte.csv.gz'


def trace_frames():
    frames = defaultdict(list)
    with gzip.open(TRACE, 'rt') as f:
        for row in csv.DictReader(f):
            if int(row['reg']) < 14:           # 14-15 are the joystick ports
                frames[int(row['frame'])].append((int(row['reg']), int(row['value'])))
    return frames


@unittest.skipIf(ROM is None, 'reference ROM or z80 module missing')
class ExtractionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace = trace_frames()

    def test_start_jingle_matches_openmsx(self):
        # 8A requested in state 3 (4222); trace frames 598.. hold the driver output.
        sound = ext.run_sound(ROM, 0x8A)
        self.assertIsNone(sound['loop'])
        self.assertEqual(len(sound['frames']), 385)
        for i, frame in enumerate(sound['frames']):
            self.assertEqual([tuple(w) for w in frame], self.trace[598 + i], f'frame {i}')

    def test_stage_music_loop_matches_openmsx(self):
        # 80 from frame 1096 (state 5), channels A and B (effects use C): 7649
        # frames, so the loop point is crossed three times.
        sound = ext.run_sound(ROM, 0x80)
        frames, loop = sound['frames'], sound['loop']
        self.assertEqual((len(frames), loop), (2388, 596))
        keep = {0, 1, 2, 3, 8, 9}
        for i in range(7649):
            ours = frames[i] if i < len(frames) else frames[loop + (i - len(frames)) % (len(frames) - loop)]
            self.assertEqual([tuple(w) for w in ours if w[0] in keep],
                             [w for w in self.trace[1096 + i] if w[0] in keep], f'frame {i}')

    def test_whip_effect_matches_openmsx(self):
        # Whip 05 (7225) at trace frame 1197: channel C registers and noise.
        sound = ext.run_sound(ROM, 0x05)
        self.assertEqual(len(sound['frames']), 13)
        keep = {4, 5, 6, 10}
        for i, frame in enumerate(sound['frames']):
            self.assertEqual([tuple(w) for w in frame if w[0] in keep],
                             [w for w in self.trace[1197 + i] if w[0] in keep], f'frame {i}')

    def test_every_sound_ends_or_loops(self):
        for sid in ext.EFFECTS:
            sound = ext.run_sound(ROM, sid)
            self.assertIsNone(sound['loop'])
            self.assertLess(len(sound['frames']), 100)
        for sid in ext.MUSIC:
            sound = ext.run_sound(ROM, sid)
            self.assertTrue(sound['loop'] is not None or len(sound['frames']) < 400, hex(sid))

    def test_death_jingle_locks_effects(self):
        # 89 sets C0A8 (command E6) from its first frame to its end.
        sound = ext.run_sound(ROM, 0x89)
        self.assertEqual(sound['lock'], list(range(len(sound['frames']))))


if __name__ == '__main__':
    unittest.main()
