#!/usr/bin/env python3
"""Run an instrumented openMSX animation trace using the project's verified ROM."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
from analyze_simon_trace import analyze
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'

def load_rom(path):
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as z:
            roms = [f for f in z.namelist() if f.lower().endswith('.rom')]
            if len(roms) != 1:
                raise ValueError('Expected precisely one .rom in ZIP')
            data = z.read(roms[0])
    else:
        data = path.read_bytes()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED:
        raise ValueError(f'ROM SHA-256 mismatch: {actual}')
    return data

def run(rom_path, openmsx, system_data, output, seconds, probe=None):
    data = load_rom(rom_path)
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='vk-openmsx-') as tmp:
        tmp = Path(tmp)
        (tmp / 'game.rom').write_bytes(data)
        snippet = (ROOT / 'tools/trace_simon_live.tcl').read_text()
        # Replace only the time limit; paths are held outside the Tcl source.
        snippet = snippet.replace('after time 38 {vk_finish}', f'after time {seconds} {{vk_finish}}')
        prefix = 'set vk_output {' + str(output.resolve()) + '}\n'
        if probe:
            row, column, delay, hold = probe
            if not (0 <= row <= 10 and 0 <= column <= 7 and delay >= 0 and 0.02 <= hold <= 1):
                raise ValueError('Invalid experimental key matrix pulse')
            prefix += f'set vk_probe_row {row}\nset vk_probe_col {column}\nset vk_probe_delay {delay}\nset vk_probe_hold {hold}\n'
        script = prefix + snippet
        (tmp / 'trace.tcl').write_text(script)
        env = dict(os.environ, OPENMSX_SYSTEM_DATA=str(system_data.resolve()),
                   OPENMSX_USER_DATA=str(tmp / 'user'), SDL_VIDEODRIVER='dummy',
                   SDL_AUDIODRIVER='dummy')
        with (output / 'openmsx-trace.log').open('w') as log:
            subprocess.run([str(openmsx.resolve()), '-control', 'stdio',
                            '-machine', 'C-BIOS_MSX2', '-cart', str(tmp / 'game.rom'),
                            '-script', str(tmp / 'trace.tcl')], stdin=subprocess.DEVNULL,
                           stdout=log, stderr=subprocess.STDOUT, env=env,
                           timeout=seconds * 8 + 60, check=True)
    complete = output / 'simon-live-complete.txt'
    if not complete.is_file():
        raise RuntimeError('openMSX exited without completion marker; inspect openmsx-trace.log')
    summary = analyze(output / 'simon-live.csv')
    (output / 'simon-sequences.json').write_text(json.dumps(summary, indent=2) + '\n')
    return summary

def summarize(csv_path):
    rows = list(csv.DictReader(csv_path.open()))
    if not rows:
        raise RuntimeError('No interrupt samples captured; check Z80 breakpoint/BIOS settings')
    segments = []
    for r in rows:
        identity = (r['marker'],r['c000'],r['c001'],r['lower_c42e'],r['upper_c42f'])
        if segments and segments[-1]['identity'] == list(identity):
            segments[-1]['samples'] += 1
            segments[-1]['last_tick'] = int(r['tick'])
        else:
            segments.append(dict(identity=list(identity),first_tick=int(r['tick']),
                                 last_tick=int(r['tick']),samples=1))
    return dict(sample_count=len(rows),segments=segments,
                note='Segments are observational, not automatically classified as whip or stairs')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('rom',type=Path)
    p.add_argument('--openmsx',type=Path,required=True)
    p.add_argument('--system-data',type=Path,required=True)
    p.add_argument('--output',type=Path,default=ROOT/'build/cache/simon-live')
    p.add_argument('--seconds',type=int,default=38)
    p.add_argument('--experimental-key',type=int,nargs=2,metavar=('ROW','COL'), help='UNVERIFIED MSX key matrix pulse after title start')
    p.add_argument('--probe-delay',type=float,default=6.0,help='Seconds after title-start pulse')
    p.add_argument('--probe-hold',type=float,default=0.15,help='Pulse duration in seconds')
    a=p.parse_args()
    probe=(*a.experimental_key,a.probe_delay,a.probe_hold) if a.experimental_key else None
    print(json.dumps(run(a.rom,a.openmsx,a.system_data,a.output,a.seconds,probe),indent=2))
if __name__=='__main__': main()
