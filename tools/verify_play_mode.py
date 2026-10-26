#!/usr/bin/env python3
"""End-to-end MD checks: movement, jump/landing, facing and sprite composition."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import ImageChops
from probe import load_rom, ROOT
from extract_rooms import decode_room, render
from extract_player import extract_pose, POSES
from run_headless import run
from verify_viewer import gx_rgb565


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core', type=Path)
    parser.add_argument('source_rom', type=Path)
    parser.add_argument('md_rom', type=Path)
    parser.add_argument('symbols', type=Path)
    args = parser.parse_args()
    match = re.search(r'^([0-9a-fA-F]+)\s+[bB]\s+player$', args.symbols.read_text(), re.M)
    if not match:
        raise ValueError('player RAM symbol not found')
    address = int(match[1], 16) & 65535
    images, states = {}, {}
    out = ROOT / 'validation'

    def memory(raw, step):
        # Genesis Plus GX stores 68k work RAM in host word order on little-endian hosts.
        values = [raw[(address + i) ^ 1] for i in range(7)]
        states[step] = dict(zip(('x', 'y', 'facing', 'mode', 'phase', 'clock', 'pose'), values))

    def image(frame, step):
        images[step] = frame

    # Libretro controls: B=0, C=8, right=7, left=6.
    script = [([0], 5), ([], 5), ([7], 30), ([], 5), ([8], 5), ([], 7),
              ([], 25), ([6], 10), ([], 5)]
    run(args.core, args.md_rom, out / 'md-player-left.png', script=script,
        memory_observer=memory, observe=image, startup=[([1], 5), ([], 5)])
    assert (states[2]['x'], states[2]['y']) == (16, 192)
    assert (states[4]['x'], states[4]['y']) == (76, 192)
    assert states[6]['mode'] == 1 and states[6]['y'] < 192
    assert (states[7]['x'], states[7]['y'], states[7]['mode']) == (76, 192, 0)
    assert (states[9]['x'], states[9]['y'], states[9]['facing']) == (56, 192, 1)
    data = load_rom(args.source_rom)
    record, graphics, tilemap = decode_room(data, 1, 0)
    palette = [gx_rgb565(c) for c in record['palette_rgb333']]
    background = render(graphics, tilemap, record['palette_rgb333'], palette)
    comparisons = []
    for step in (2, 4, 6, 7, 9):
        state = states[step]
        pose = state['pose']
        pixels, _ = extract_pose(data, *POSES[pose], pose >= 6)
        expected = background.copy()
        for yy in range(32):
            for xx in range(24):
                color = pixels[yy * 24 + xx]
                x, y = state['x'] - 8 + xx, state['y'] - 31 + yy - 32
                if color and 0 <= x < 256 and 0 <= y < 176:
                    expected.putpixel((x, y), palette[color])
        diff = ImageChops.difference(expected, images[step].crop((0, 32, 256, 208)))
        if diff.getbbox():
            diff.save(out / f'player-difference-{step}.png')
            raise AssertionError(f'Sprite pixels do not match ROM masks at step {step}')
        comparisons.append({'step': step, 'pose': pose, 'pixels_equal': True})
    images[2].save(out / 'md-player-idle.png')
    images[6].save(out / 'md-player-jump.png')
    images[9].save(out / 'md-player-left.png')
    report = {'all_passed': True, 'md_rom_sha256': hashlib.sha256(args.md_rom.read_bytes()).hexdigest(),
              'movement': {'right_30_frames_px': 60, 'left_10_frames_px': 20},
              'jump_left_ground': True, 'landing_y': 192, 'left_facing_verified': True,
              'sprite_pixel_comparisons': comparisons, 'observed_states': states,
              'scope': 'First-room movement prototype; full original state machine not ported.'}
    (out / 'play-mode-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'observed_states'}, indent=2))


if __name__ == '__main__':
    main()
