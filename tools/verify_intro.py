#!/usr/bin/env python3
"""Check the new MD boot/title/castle/courtyard/interior flow and scene pixels.
Scene references are reconstructed by the bounded Z80 graphics harness.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw
from probe import ROOT, load_rom
from intro_harness import IntroHarness
from extract_player import extract_pose, POSES
from verify_viewer import gx_rgb565
from md_harness import BUTTONS, FRONT_MODES as MODES, Machine, load_symbols, lookup


def quantize(image, palette):
    colors = {tuple(round(v * 255 / 7) for v in rgb): gx_rgb565(rgb) for rgb in palette}
    result = Image.new('RGB', image.size)
    raw = image.tobytes()
    result.putdata([colors[p] for p in zip(raw[0::3], raw[1::3], raw[2::3])])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core', type=Path)
    parser.add_argument('source_rom', type=Path)
    parser.add_argument('md_rom', type=Path)
    parser.add_argument('symbols', type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / 'validation',
                        help='output directory (default: validation/)')
    args = parser.parse_args()
    data = load_rom(args.source_rom)
    # Regression for the faulty routine adapters: HMMV consumes HL and returns
    # HL=the original BC. Its caller uses that result to draw the blue frame.
    probe = IntroHarness(data)
    probe.cpu.hl, probe.cpu.bc, probe.cpu.d, probe.cpu.a = 0x1020, 0x0802, 0, 0xFF
    probe.call(0x490A)
    assert probe.cpu.hl == 0x0802 and probe.cpu.a == 0xC0
    assert all(probe.pixel(x, y) == 15 for y in range(32, 34) for x in range(16, 24))
    probe.call(0x4B59); probe.call(0x4D47)
    assert probe.pixel(16, 40) == 15 and probe.palette[15] == (0, 0, 7)
    assert probe.pixel(6, 24) == 0  # Black perforation in the blue film frame.
    # Palette writes must reach both the VDP palette port and the VRAM mirror.
    for i, (r, g, b) in enumerate(probe.palette):
        rb, green = probe.vram[0xF680 + i * 2:0xF682 + i * 2]
        assert (r, g, b) == ((rb >> 4) & 7, green & 7, rb & 7)
    symbols = load_symbols(None, args.symbols)
    address = {name: lookup(symbols, name)[0] for name in
               ('front_mode', 'front_timer', 'courtyard_room', 'player')}
    images, states = {}, {}
    output = args.out
    output.mkdir(parents=True, exist_ok=True)
    machine = Machine(args.core, args.md_rom)

    def state():
        result = {name: machine.rd16(a) for name, a in address.items() if name != 'player'}
        result['player'] = dict(zip(('x', 'y', 'facing', 'mode', 'phase', 'clock', 'pose'),
                                    [machine.rd8(address['player'] + i) for i in range(7)]))
        return result

    def capture(step):
        states[step] = state()
        images[step] = machine.image()

    def until(condition, buttons=(), limit=3000):
        for _ in range(limit):
            if condition(state()):
                return
            machine.run(1, frozenset(buttons))
        raise AssertionError(f'condition not reached in {limit} frames: {state()}')

    def hold(buttons, frames):
        machine.run(frames, frozenset(buttons))

    # The flow is driven by RAM states rather than fixed delays: walking speed and
    # room drawing time may change without breaking the check.
    START, UP, RIGHT = BUTTONS['START'], BUTTONS['UP'], BUTTONS['RIGHT']
    until(lambda s: s['front_mode'] == MODES.index('TITLE'), limit=300)
    hold((), 60)                                          # title faded in
    capture(0)
    hold((START,), 5)
    capture(1)
    hold((), 60)
    capture(2)
    hold((), 180)
    capture(3)
    until(lambda s: s['front_mode'] == MODES.index('COURTYARD'), limit=400)
    hold((), 30)                                          # room, HUD and Simon drawn
    capture(4)
    until(lambda s: s['courtyard_room'] == 1, (RIGHT,))
    until(lambda s: s['courtyard_room'] == 2, (RIGHT,))
    # Courtyard 2: walk to the door, then stop while the scene is still the courtyard.
    until(lambda s: s['player']['x'] >= 192 or s['front_mode'] != MODES.index('COURTYARD'), (RIGHT,))
    hold((), 2)
    capture(8)
    until(lambda s: s['front_mode'] == MODES.index('INTERIOR'), (RIGHT,))
    hold((), 30)
    capture(10)
    machine.close()
    assert states[0]['front_mode'] == MODES.index('TITLE')
    assert all(states[i]['front_mode'] == MODES.index('CASTLE') for i in (1, 2, 3))
    assert states[4]['front_mode'] == MODES.index('COURTYARD') and states[4]['courtyard_room'] == 0
    assert states[8]['front_mode'] == MODES.index('COURTYARD') and states[8]['player']['x'] >= 192
    assert states[10]['front_mode'] == MODES.index('INTERIOR')
    assert (states[10]['player']['x'], states[10]['player']['y']) == (16, 192)
    manifest = json.loads((ROOT / 'assets/intro/manifest.json').read_text())
    comparisons = []

    def compare(expected, actual, name):
        diff = ImageChops.difference(expected, actual)
        if diff.getbbox():
            diff.save(output / f'intro-difference-{name}.png')
            expected.save(output / f'intro-expected-{name}.png')
            actual.save(output / f'intro-actual-{name}.png')
            raise AssertionError(f'Scene pixels differ: {name}')
        comparisons.append(name)

    # The title prompt is intentionally adapted from keyboard to controller.
    reference = Image.open(ROOT / 'assets/intro/title.png').convert('RGB')
    reference = quantize(reference, manifest['scenes'][1]['palette'])
    actual = images[0].copy()
    for im in (reference, actual): ImageDraw.Draw(im).rectangle((72, 160, 191, 167), fill=(0, 0, 0))
    compare(reference, actual, 'title_except_controller_prompt')

    h = IntroHarness(data)
    h.call(0x4B59); h.call(0x4D47); h.call(0x44C6)
    h.cpu.memory[0xC41A] = 1; h.call(0x63D3)
    reference_frames = []
    for frame in range(320):
        h.cpu.memory[0xC003] = (frame + 1) & 255
        h.call(0x4E93)
        reference_frames.append(quantize(h.composite(), h.palette))
    matched = []
    for step in (1, 2, 3):
        timer = states[step]['front_timer']
        crop = images[step].crop((0, 0, 256, 212))
        # Work RAM may be one VBlank ahead of the completed video frame.
        candidates = [frame for frame in (timer, timer - 1) if 0 <= frame < 320]
        equal = [frame for frame in candidates if ImageChops.difference(crop, reference_frames[frame]).getbbox() is None]
        if not equal:
            raise AssertionError(f'Castle reference mismatch at timer {timer}')
        matched.append({'md_timer': timer, 'original_frame': equal[0]})
        comparisons.append(f'castle_frame_{equal[0]}')

    # Courtyard torch flames (room_view.c, courtyard_flame_x/y) and hall candles
    # (candle_view.c) are animated sprites over the baked scene: their 16x16 boxes
    # are left out of the comparison. In the hall the play note (row 26, y 208)
    # covers the last four MSX lines.
    flames = {3: ((64, 144), (192, 144)), 5: ((64, 144),), 6: ((112, 144), (176, 144))}
    note_rows = {6: 208}
    for step, scene in ((4, 3), (8, 5), (10, 6)):
        palette = manifest['scenes'][scene]['palette']
        expected = quantize(Image.open(ROOT / 'assets/intro' /
                             (manifest['scenes'][scene]['name'] + '.png')).convert('RGB'), palette)
        actor = states[step]['player']
        pixels, _ = extract_pose(data, *POSES[actor['pose']], actor['pose'] >= 6)
        for yy in range(32):
            for xx in range(24):
                color = pixels[yy * 24 + xx]
                x, y = actor['x'] - 8 + xx, actor['y'] - 31 + yy
                if color and 0 <= x < 256 and 0 <= y < 212:
                    expected.putpixel((x, y), gx_rgb565(palette[color]))
        actual = images[step].crop((0, 0, 256, 212))
        expected = expected.crop((0, 0, 256, 212))
        for x, y in flames.get(scene, ()):
            for im in (expected, actual): ImageDraw.Draw(im).rectangle((x, y, x + 15, y + 15), fill=(0, 0, 0))
        if scene in note_rows:
            for im in (expected, actual): ImageDraw.Draw(im).rectangle((0, note_rows[scene], 255, 211), fill=(0, 0, 0))
        compare(expected, actual, f'scene_{scene}_with_player')
    # Logo capture is made while its adapted fade has reached full intensity.
    machine = Machine(args.core, args.md_rom)
    machine.run(60)
    machine.image().save(output / 'md-konami.png')
    machine.close()
    logo = quantize(Image.open(ROOT / 'assets/intro/konami.png').convert('RGB'), manifest['scenes'][0]['palette'])
    compare(logo, Image.open(output / 'md-konami.png').convert('RGB'), 'konami_logo')
    for step, name in [(0, 'title'), (2, 'castle-entry'), (4, 'courtyard'),
                        (8, 'castle-door'), (10, 'interior')]:
        images[step].save(output / f'md-{name}.png')
    report = {'all_passed': True, 'md_rom_sha256': hashlib.sha256(args.md_rom.read_bytes()).hexdigest(),
              'flow': ['logo', 'title', 'animated castle scene', 'courtyard 0', 'courtyard 1',
                       'courtyard 2', 'castle interior'],
              'pixel_comparisons': comparisons, 'castle_matches': matched, 'states': states,
              'reference_scope': 'Original Z80 palette and command routines executed through VDP ports; synchronous drawing, not full MSX emulation.',
              'graphics_regressions': ['HMMV register side effects', 'blue title frame index 15',
                                       'black frame perforation', 'VDP palette matches VRAM mirror'],
              'masked': ['title controller prompt', 'animated torch and candle flames',
                         'hall play note over lines 208-211']}
    (output / 'intro-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'states'}, indent=2))


if __name__ == '__main__':
    main()
