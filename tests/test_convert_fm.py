"""FM conversion of the music (tools/convert_fm.py, tools/build_msx_sound_md.py).

Needs the reference ROM (VK_ROM_PATH, see tools/probe.py) and the z80 module; skipped
otherwise. The comparison with the ear-validated prototype (fm.py, m80.json) runs only
where that prototype exists (VK_FM_PROTO, default scratchpad/fm_proto/src of the main
checkout); every other test is self-contained.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))

try:
    import build_msx_sound_md as build
    import convert_fm as fm
    import extract_msx_sound as ext
    from probe import load_rom, reference_rom
    ROM = load_rom(reference_rom())
except Exception as error:  # noqa: BLE001 - missing ROM or z80 module
    ROM = None

PROTO = Path(os.environ.get('VK_FM_PROTO', ROOT / 'scratchpad/fm_proto/src')).resolve()
MUSICS = range(0x80, 0x8F)
_cache = {}


def sound(sid):
    if sid not in _cache:
        _cache[sid] = ext.run_sound(ROM, sid)
    return _cache[sid]


def frame_offsets(stream):
    """Byte offsets of the frame headers of a stream (count in the low 6 bits)."""
    offsets, i = [], 0
    while stream[i] != 0xFF:
        offsets.append(i)
        i += 1 + (stream[i] & 0x3F) * (2 if stream is not None else 1)
    return offsets


def fm_frames(stream):
    out, i = [], 0
    while stream[i] != 0xFF:
        n = stream[i]
        out.append([(stream[i + 1 + 2 * k], stream[i + 2 + 2 * k]) for k in range(n)])
        i += 1 + 2 * n
    return out, i


def psg_frame_count(stream):
    n, i = 0, 0
    while stream[i] != 0xFF:
        i += 1 + (stream[i] & 0x3F)
        n += 1
    return n


def psg_offsets(stream):
    offs, i = [], 0
    while stream[i] != 0xFF:
        offs.append(i)
        i += 1 + (stream[i] & 0x3F)
    return offs


@unittest.skipIf(ROM is None, 'reference ROM or z80 module missing')
class FmConversionTests(unittest.TestCase):
    def test_same_frames_and_loop_as_the_psg_stream(self):
        for sid in MUSICS:
            s = sound(sid)
            psg, psg_loop, _ = build.encode(s)
            fmb, fm_loop, _ = build.encode_fm(s)
            noise, noise_loop = build.encode_noise(s)
            frames, _ = fm_frames(fmb)
            self.assertEqual(len(frames), len(s['frames']), hex(sid))
            self.assertEqual(psg_frame_count(psg), len(frames), hex(sid))
            self.assertEqual(psg_frame_count(noise), len(frames), hex(sid))
            if s['loop'] is None:
                self.assertEqual((fm_loop, noise_loop, psg_loop), (0xFFFF,) * 3)
                continue
            # byte offset of the loop frame in each stream = the loop frame index
            fm_offs, i = [], 0
            while fmb[i] != 0xFF:
                fm_offs.append(i)
                i += 1 + 2 * fmb[i]
            self.assertEqual(fm_offs.index(fm_loop), s['loop'], hex(sid))
            self.assertEqual(psg_offsets(noise).index(noise_loop), s['loop'], hex(sid))
            self.assertEqual(psg_offsets(psg).index(psg_loop), s['loop'], hex(sid))

    def test_noise_stream_has_no_tone_but_the_noise_clock(self):
        for sid in MUSICS:
            noise, _ = build.encode_noise(sound(sid))
            i = 0
            while noise[i] != 0xFF:
                n = noise[i] & 0x3F
                for b in noise[i + 1:i + 1 + n]:
                    if b & 0x80:
                        channel = (b >> 5) & 3
                        # volume of the tone channels (0x9F, 0xBF, 0xDF) only silences them
                        if b & 0x10 and channel < 3:
                            self.assertEqual(b & 15, 15, hex(sid))
                        if not b & 0x10:
                            self.assertIn(channel, (2, 3), hex(sid))   # tone 2 = noise clock
                i += 1 + n

    def test_held_notes_keep_the_ay_pitch(self):
        for sid in MUSICS:
            s = sound(sid)
            ay = build.ay_registers(s)
            frames = fm.convert(ay, s['loop'])
            freq = [None] * 3
            checked = 0
            for index, writes in enumerate(frames):
                regs = ay[index]['regs']
                for reg, value in writes:
                    if 0xA0 <= reg <= 0xA2:
                        hi = [v for r, v in writes if r == reg + 4][-1]
                        freq[reg - 0xA0] = (hi >> 3, (hi & 7) << 8 | value)
                for ch in range(3):
                    per, vol, active = fm.ay_voice(regs, ch)
                    if not active:
                        continue
                    self.assertIsNotNone(freq[ch], (hex(sid), index, ch))
                    block, fnum = freq[ch]
                    hz = fnum * fm.FS * 2 ** block / 2 ** 21
                    want = fm.AYCLK / 16 / per
                    # fnum is 11 bits: the rounding error is at most 0.5 / fnum, below 1 %
                    # once fnum >= 50 (high notes on block 0 can only be that coarse)
                    self.assertLess(abs(hz / want - 1), 0.01, (hex(sid), index, ch, hz, want))
                    checked += 1
            self.assertGreater(checked, 100)

    def test_music_80_matches_the_prototype(self):
        sys.path.insert(0, str(PROTO))
        try:
            import fm as proto
        except Exception:  # noqa: BLE001
            self.skipTest('prototype not available')
        finally:
            sys.path.remove(str(PROTO))
        data = json.load(open(PROTO / 'm80.json'))
        s = sound(0x80)
        ay = build.ay_registers(s)
        self.assertEqual(len(ay), len(data['frames']))
        self.assertEqual(s['loop'], data['loop'])
        for i, (mine, ref) in enumerate(zip(ay, data['frames'])):
            self.assertEqual(mine['regs'], ref['regs'], i)
            self.assertEqual(mine['notes'], sorted(set(c for c in ref['notes'] if c < 3)), i)
        events, _, n = proto.convert(data['frames'], proto.SETS['A_cuivres'])
        init = [(r, v) for t, p, r, v in events if t == 0]
        self.assertEqual([w for w in init if w[0] != 0x2B], fm.init_writes())
        by_frame = [[] for _ in range(n)]
        for t, p, r, v in events:
            if t > 0:
                self.assertEqual(p, 0)
                by_frame[int((t - 40 + 100) // proto.SPF)].append((r, v))
        mine = fm.convert(ay, s['loop'])
        loop = s['loop']
        for i in range(n):
            if i == loop:
                # the loop frame restates the state: the prototype writes are a subsequence
                it = iter(mine[i])
                self.assertTrue(all(w in it for w in by_frame[i]), i)
            else:
                self.assertEqual(mine[i], by_frame[i], i)


    def test_core_replays_the_converter_frame_by_frame(self):
        """fm_core.c on psg_core.c: the YM2612 pairs of every frame of music 80 (2 loops) are the
        converter's (= the prototype's, test above), in the same order; frame 0 is preceded by
        the instrument registers and the three key offs of a music start. Same for the loop."""
        cc = shutil.which('cc') or shutil.which('gcc')
        if not cc:
            self.skipTest('no C compiler')
        s = sound(0x80)
        ay = build.ay_registers(s)
        mine = fm.convert(ay, s['loop'])
        n = len(mine)
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'fm_core_dump'
            subprocess.run([cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'sgdk/src'),
                            str(ROOT / 'tests/fm_core_dump.c'), str(ROOT / 'sgdk/src/psg_core.c'), str(ROOT / 'sgdk/src/psg_core_fm.c'),
                            str(ROOT / 'sgdk/src/fm_core.c'),
                            str(ROOT / 'sgdk/src/generated/msx_sound_md.c'),
                            str(ROOT / 'sgdk/src/generated/msx_fm_md.c'), '-o', str(exe)], check=True)
            lines = subprocess.run([str(exe), '80', str(n + 200)], check=True, capture_output=True,
                                   text=True).stdout.splitlines()
        self.assertEqual(len(lines), n + 200)
        got = []
        for line in lines:
            head = line.split('|')[0].split()
            count = int(head[2])
            v = [int(x, 16) for x in head[3:]]
            self.assertEqual(len(v), 2 * count)
            got.append(list(zip(v[0::2], v[1::2])))
        prologue = fm.init_writes() + [(0x28, 0), (0x28, 1), (0x28, 2)]
        self.assertEqual(got[0], prologue + mine[0])
        for i in range(1, n):
            self.assertEqual(got[i], mine[i], i)
        # after the end of the stream the music goes on at the loop frame (the stream restates
        # the loop frame, so the replay is the same as the first pass)
        for k in range(200):
            self.assertEqual(got[n + k], mine[s['loop'] + k], (n, k))


if __name__ == '__main__':
    unittest.main()
