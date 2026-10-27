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


if __name__ == '__main__':
    unittest.main()
