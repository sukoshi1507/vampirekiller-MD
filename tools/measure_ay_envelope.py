#!/usr/bin/env python3
"""Measure the pitch of the AY-3-8910 envelope in openMSX and compare it with the SN76489 buzz.

For each (shape, EP) the script starts openMSX headless (C-BIOS_MSX2), writes the PSG registers of
a channel C with its tone at period 1 (112 kHz, inaudible) and its volume driven by the envelope
(R10 = 10h, R11-R13), records the audio to a WAV file and takes the strongest FFT bin. That is
the envelope frequency heard on the MSX. It is compared with the pitch of the SN tone the
converter writes (tools/convert_psg_vgm.py `envelope_buzz_period`, f = 3 579 545 / (32 N)).
research/audio-driver.md section 5; tests/test_convert_psg_vgm.py runs it when openmsx exists.

Usage: measure_ay_envelope.py [SHAPE:EP ...]   (default: 12:1 10:1 12:5 12:39)
"""
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import convert_psg_vgm as conv  # noqa: E402

TCL = """
after time 1 {{
    foreach {{reg val}} {{7 0xB8 4 0x01 5 0x00 10 0x10 11 {lo} 12 {hi} 13 {shape}}} {{
        debug write "PSG regs" $reg $val
    }}
    record start -audioonly {wav}
}}
after time 4 {{ record stop; exit }}
"""


def measure(shape, ep, workdir):
    wav = Path(workdir) / f'env_{shape}_{ep}.wav'
    script = Path(workdir) / f'env_{shape}_{ep}.tcl'
    script.write_text(TCL.format(lo=ep & 0xFF, hi=ep >> 8, shape=shape, wav=wav))
    subprocess.run(['openmsx', '-machine', 'C-BIOS_MSX2', '-script', str(script)],
                   check=True, capture_output=True, timeout=120)
    with wave.open(str(wav)) as w:
        rate, channels = w.getframerate(), w.getnchannels()
        x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(float)
    if channels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    x = x[rate // 2:]
    x -= x.mean()
    spectrum = np.abs(np.fft.rfft(x * np.hanning(len(x))))
    return np.fft.rfftfreq(len(x), 1 / rate)[spectrum.argmax()]


def predicted(shape, ep):
    """(AY envelope frequency from the datasheet, SN buzz frequency from the converter)."""
    steps = 32 if shape in (10, 14) else 16
    ay_hz = conv.AY_CLOCK / (16 * steps * ep)
    regs = [0] * 16
    regs[11], regs[12], regs[13] = ep & 0xFF, ep >> 8, shape
    n = conv.envelope_buzz_period(regs)
    return ay_hz, (conv.SN_CLOCK / (32 * n) if n else None), n


def main(argv):
    cases = [tuple(int(v) for v in a.split(':')) for a in argv] or [(12, 1), (10, 1), (12, 5), (12, 39)]
    with tempfile.TemporaryDirectory() as workdir:
        for shape, ep in cases:
            seen = measure(shape, ep, workdir)
            ay_hz, sn_hz, n = predicted(shape, ep)
            print(f'shape {shape:2d} EP {ep:3d}: openMSX {seen:9.1f} Hz, datasheet {ay_hz:9.1f} Hz, '
                  f'SN N={n} {sn_hz:9.1f} Hz, SN/openMSX {sn_hz / seen:.5f}')


if __name__ == '__main__':
    main(sys.argv[1:])
