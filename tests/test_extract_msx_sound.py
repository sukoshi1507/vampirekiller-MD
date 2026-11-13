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
    import build_msx_sound_md as build
    import convert_psg_vgm as conv
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


CALL_OPS = (0xCD, 0xC3, 0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC, 0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA)


def rom_callers(rom):
    """Every static request of a sound id in banks 0-3 (CPU 4000-BFFF, default mapping):
    {id: {(kind, address)}}. `LD A,id` right before CALL/JP 509F (kind call, address of the
    CALL), before CALL A4E8 (wrap), before RET in the 8D3E pickup handlers (ret, address of
    the LD), and the three special cases 796F (table), 77F7 (JR between 87 and 86)."""
    found = {}
    for i in range(0x8000 - 5):
        if rom[i] != 0x3E:
            continue
        pc, sid, op, target = 0x4000 + i, rom[i + 1], rom[i + 2], (rom[i + 3], rom[i + 4])
        if op in CALL_OPS and target == (0x9F, 0x50):
            found.setdefault(sid, set()).add(('call', pc + 2))
        elif op == 0xCD and target == (0xE8, 0xA4):
            found.setdefault(sid, set()).add(('wrap', pc + 2))
        elif op == 0xC9 and 0x8D3E <= pc < 0x8EA0:
            found.setdefault(sid, set()).add(('ret', pc))
    return found


@unittest.skipIf(ROM is None, 'reference ROM or z80 module missing')
class SoundEventTests(unittest.TestCase):
    """research/audio-driver.md section 3: who requests each sound, proven on the ROM bytes."""

    def test_every_request_in_the_rom_is_declared(self):
        declared = {sid: set(map(tuple, callers)) for sid, (_, _, callers) in build.SOUNDS.items()}
        for sid, callers in build.CONTROLS.items():
            declared.setdefault(sid, set()).update(map(tuple, callers))
        found = rom_callers(ROM)
        for sid in set(declared) | set(found):
            have = {c for c in declared.get(sid, set()) if c[0] in ('call', 'wrap', 'ret')}
            self.assertEqual(found.get(sid, set()), have, f'sound {sid:02X}')
        # 87 and 86 share the JR of 77F7-77FD; the music table 7972 (D000 = 0..18) feeds 80-85.
        self.assertEqual(bytes(ROM[0x77F7 - 0x4000:0x77FE - 0x4000]), bytes.fromhex('3e8720023e86c3'))
        self.assertEqual(bytes(ROM[0x796F - 0x4000:0x7972 - 0x4000]), bytes.fromhex('c39f50'))
        table = ROM[0x7972 - 0x4000:0x7972 - 0x4000 + 19]
        for sid in range(0x80, 0x86):
            self.assertIn(sid, table)
        self.assertEqual(set(table), set(range(0x80, 0x86)))

    def test_every_extracted_sound_has_a_proven_name_and_a_caller(self):
        extracted = set(ext.EFFECTS) | set(ext.MUSIC) | set(ext.SPECIALS)
        music_unused = {0x8F}
        self.assertEqual(set(build.SOUNDS), extracted - music_unused)
        names = [entry[0] for entry in build.SOUNDS.values()]
        self.assertEqual(len(names), len(set(names)), 'duplicate label')
        for sid, (name, event, callers) in build.SOUNDS.items():
            self.assertTrue(callers, f'{sid:02X} has no caller')
            self.assertTrue(event, f'{sid:02X} has no event')
            self.assertLessEqual(len(name), build.NAME_MAX, name)
            # a label is an event, not an address or a bare id ("OBJET 94FC", "MUSIQUE 88")
            self.assertFalse(any(len(word) == 4 and all(c in '0123456789ABCDEF' for c in word)
                                 and any(c.isdigit() for c in word) for word in name.split()), name)
            self.assertNotIn('%02X' % sid, name.split(), name)

    def test_fast_envelope_buzz_is_only_in_sound_0b(self):
        # research/audio-driver.md section 5: the only frames where the SN tone takes the
        # envelope frequency are frames 25 and 26 of effect 0B (EP = 1, saw, tone period 1).
        buzz = {}
        for sid in ext.EFFECTS + ext.MUSIC + ext.SPECIALS:
            sound = ext.run_sound(ROM, sid)
            channels = build.channels_of(sound)
            converter = conv.FrameConverter(channels)
            for index, frame in enumerate(sound['frames']):
                before = converter.stats['envelope_buzz']
                converter.frame(frame)
                if converter.stats['envelope_buzz'] != before:
                    buzz.setdefault(sid, []).append(index)
        self.assertEqual(buzz, {0x0B: [25, 26]})

    def test_hearts_spent_sound_is_extracted(self):
        sound = ext.run_sound(ROM, 0x1D)
        self.assertEqual(len(sound['frames']), 46)
        self.assertIsNone(sound['loop'])


if __name__ == '__main__':
    unittest.main()
