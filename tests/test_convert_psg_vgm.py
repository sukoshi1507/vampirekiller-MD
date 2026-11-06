import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import convert_psg_vgm as conv  # noqa: E402


class PsgConversionTests(unittest.TestCase):
    def test_volume_mapping_endpoints(self):
        self.assertEqual(conv.ay_volume_to_sn_attenuation(0), 15)
        self.assertEqual(conv.ay_volume_to_sn_attenuation(15), 0)
        values = [conv.ay_volume_to_sn_attenuation(v) for v in range(1, 16)]
        self.assertEqual(values, sorted(values, reverse=True))

    def test_tone_latch_bytes(self):
        # Canal 0, N = 0x123 : premier octet 1 00 0 0011, second 0 010010
        self.assertEqual(conv.sn_latch_tone(0, 0x123), bytes([0x83, 0x12]))
        # Canal 2, N = 0x3FF
        self.assertEqual(conv.sn_latch_tone(2, 0x3FF), bytes([0xCF, 0x3F]))

    def test_noise_rate_monotonic(self):
        rates = [conv.nearest_noise_rate(np) for np in (31, 16, 8, 4, 2, 1)]
        self.assertEqual(rates, sorted(rates))

    def test_convert_emits_tone_and_volume(self):
        writes = [(0, 0, 0x80), (0, 1, 0x00), (0, 8, 0x0F), (0, 7, 0x3E)]
        events, stats = conv.convert(writes, 735)
        flat = b''.join(chunk for _, chunk in events)
        # Etat initial (4 atténuations à 15) puis latch ton canal 0 N=0x080 puis volume 0
        self.assertIn(bytes([0x80 | 0x00, 0x08]), flat)
        self.assertEqual(stats['tone_clipped'], 0)
        self.assertIn(0x90, flat)  # volume canal 0 = 0 atténuation

    def test_tone_clipping_is_counted(self):
        writes = [(0, 0, 0xFF), (0, 1, 0x0F), (0, 8, 0x0F), (0, 7, 0x3E)]
        _, stats = conv.convert(writes, 735)
        self.assertGreater(stats['tone_clipped'], 0)

    def test_parse_and_build_roundtrip(self):
        ay = bytearray(0x40)
        ay[0:4] = b'Vgm '
        body = bytes([0xA0, 0x08, 0x0F, 0x61, 0x13, 0x02, 0xA0, 0x07, 0x3E, 0x66])
        ay += body
        writes, total, unsupported = conv.parse_ay_vgm(bytes(ay))
        self.assertEqual(writes, [(0, 8, 15), (0x0213, 7, 0x3E)])
        self.assertEqual(total, 0x0213)
        self.assertEqual(unsupported, {})
        events, _ = conv.convert(writes, total)
        vgm = conv.build_vgm(events, total)
        self.assertEqual(vgm[:4], b'Vgm ')
        self.assertEqual(struct.unpack_from('<I', vgm, 0x0C)[0], conv.SN_CLOCK)
        self.assertEqual(struct.unpack_from('<I', vgm, 0x18)[0], total)
        self.assertEqual(vgm[-1], 0x66)


class FrameConversionTests(unittest.TestCase):
    def test_volume_is_three_db_per_ay_step(self):
        self.assertEqual(conv.ay_volume_to_sn_attenuation(13), 3)
        self.assertEqual(conv.ay_volume_to_sn_attenuation(5), 15)

    def test_low_tone_is_raised_by_octaves(self):
        self.assertEqual(conv.fold_tone_period(1000), (1000, 0))
        self.assertEqual(conv.fold_tone_period(2000), (1000, 1))
        self.assertEqual(conv.fold_tone_period(4095), (512, 3))
        self.assertEqual(conv.fold_tone_period(0), (1, 0))

    def test_envelope_shapes(self):
        self.assertEqual(conv.envelope_level(0, 0), 15)     # decay
        self.assertEqual(conv.envelope_level(0, 20), 0)
        self.assertEqual(conv.envelope_level(13, 40), 15)   # attack then hold high
        self.assertEqual(conv.envelope_level(10, 16), 0)    # triangle: decay then attack
        self.assertEqual(conv.envelope_level(10, 31), 15)

    def test_effect_on_channel_c_only(self):
        fc = conv.FrameConverter(channels=(2,))
        out = fc.frame([(7, 0xB8), (4, 0x80), (5, 0), (10, 15), (8, 15), (0, 0x10)])
        self.assertEqual(out, bytes([0xC0, 0x08, 0xD0, 0xFF]))

    def test_noise_clocked_by_tone_two_when_free(self):
        fc = conv.FrameConverter(channels=(2,))
        # Mixer 9C: tone C off, noise C on; noise period 26.
        out = fc.frame([(7, 0x9C), (6, 26), (10, 14)])
        self.assertIn(bytes(conv.sn_latch_tone(2, 26)), out)
        self.assertIn(0xE7, out)
        self.assertIn(0xF0 | conv.ay_volume_to_sn_attenuation(14), out)

    def test_unchanged_state_writes_nothing_and_loop_restates(self):
        frames = [[(7, 0xB8), (0, 100), (8, 15)], [(7, 0xB8)], [(0, 100)]]
        out, _ = conv.convert_frames(frames, (0,), loop=2)
        self.assertEqual(out[1], b'')
        self.assertEqual(out[2], bytes([0x84, 0x06, 0x90, 0xFF]))   # noise silence too


if __name__ == '__main__':
    unittest.main()
