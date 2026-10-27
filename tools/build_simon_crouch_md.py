#!/usr/bin/env python3
"""Generate ROM-derived crouch and crouch-attack candidate frames for SGDK."""
from pathlib import Path
import hashlib
import json
from PIL import Image
from decode_extended_simon import components, render
from extract_rooms import decode_room
from probe import load_rom, reference_rom

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'sgdk/src/generated'
# Crouch compositions (lower, upper) in the order of the crouch whip sequence.
# The openMSX trace of a crouched attack (C42E stays 6 while crouched, C42F =
# 6,6,7,7,8x8 then 0; 12 ticks = timer 14..3) shows the upper indices 6, 7 and 8,
# the same ones as the standing attack with lower index 0 (build_simon_attack_md.py).
# Index 0 is the idle crouch (C42F = 0).
PAIRS = [
    (6, 0),  # idle crouch
    (6, 6),  # attack, timer 14..13 (MSX ticks 1-2)
    (6, 7),  # attack, timer 12..11 (MSX ticks 3-4)
    (6, 8),  # attack, timer 10..3 (MSX ticks 5-12)
]
# 64x32 anchored buffer, as build_simon_attack_md.py: the whip reaches x+39.
# Two 32x32 halves are emitted, one SGDK sprite each.
W = 64
H = 32
TILES_PER_HALF = 16
CANVAS_WORLD_LEFT = -24
CANVAS_WORLD_TOP = -30


def _find_rom() -> Path:
    candidate = reference_rom()
    if candidate.exists():
        return candidate
    raise FileNotFoundError(f'Reference ROM/ZIP not found: {candidate}')


def _tile_words_from_canvas(canvas: Image.Image, lookup: dict) -> list[int]:
    rgba = list(canvas.getdata())
    if any(pixel[3] and pixel not in lookup for pixel in rgba):
        extras = sorted({pixel[:3] for pixel in rgba if pixel[3] and pixel not in lookup})
        raise ValueError(f'Unexpected sprite palette entries: {extras}')
    pixels = [lookup.get(pixel, 0) if pixel[3] else 0 for pixel in rgba]
    words = []
    # Each 32x32 half is one SGDK sprite: 16 tiles in column-major order.
    for half in range(W // 32):
        for tx in range(4):
            for ty in range(4):
                for py in range(8):
                    word = 0
                    for px in range(8):
                        x = half * 32 + tx * 8 + px
                        word = (word << 4) | pixels[(ty * 8 + py) * W + x]
                    words.append(word)
    assert len(words) == (W // 32) * TILES_PER_HALF * 8
    return words


def _compress_vertical_gaps(img: Image.Image, keep_gap: int = 1):
    """Close large transparent gaps between occupied row bands.

    Some ROM crosspairs (notably LOW06/UP00 and LOW06/UP02) contain the right
    upper and lower body pieces but with a large vertical separation. For the
    playable SGDK reconstruction we preserve the sprite art while reducing only
    that empty gap so Simon reads as one coherent crouched figure.
    """
    rows = [any(img.getpixel((x, y))[3] for x in range(img.width)) for y in range(img.height)]
    spans = []
    inside = False
    start = 0
    for i, occ in enumerate(rows + [False]):
        if occ and not inside:
            start = i
            inside = True
        elif inside and not occ:
            spans.append((start, i - 1))
            inside = False
    if len(spans) <= 1:
        return img, []
    out = Image.new('RGBA', (img.width, img.height))
    removed = []
    dest_y = spans[0][0]
    for idx, (start, end) in enumerate(spans):
        band = img.crop((0, start, img.width, end + 1))
        out.alpha_composite(band, (0, dest_y))
        if idx + 1 < len(spans):
            next_start = spans[idx + 1][0]
            gap = next_start - end - 1
            if gap > keep_gap:
                removed.append({'after_span': idx, 'original_gap': gap, 'kept_gap': keep_gap, 'reduced_by': gap - keep_gap})
            dest_y += band.height + min(gap, keep_gap)
    return out.crop(out.getbbox()), removed

def generate(rom_path: Path | None = None):
    rom_path = rom_path or _find_rom()
    rom = load_rom(rom_path)
    palette = decode_room(rom, 1, 0)[0]['palette_rgb333']
    colors = [tuple(round(v * 255 / 7) for v in rgb) for rgb in palette]
    lookup = {(*colors[k], 255): k for k in range(4)}

    frames = []
    all_words: list[int] = []
    OUT.mkdir(parents=True, exist_ok=True)
    preview_sheet = Image.new('RGBA', (W * len(PAIRS), H))

    for frame_index, (low, up) in enumerate(PAIRS):
        lower = components(rom, low, 'lower')
        upper = components(rom, up, 'upper')
        # Compose at MSX pixel resolution: bring the leg component up by six
        # pixels. Do not detect/compress blank rows; the extended whip frames
        # contain intentional transparent rows which must not move independently.
        lower['offsets_dy_dx'] = [(dy - 6, dx) for dy, dx in lower['offsets_dy_dx']]
        pic, orphan, bounds = render([lower, upper], colors)
        if orphan:
            raise ValueError(f'Incomplete crouch frame {low}/{up}: {len(orphan)} unplaced layers')

        bbox = pic.getbbox()
        assert bbox is not None
        subject = pic.crop(bbox)
        gap_adjustments = []  # fixed component-space offset, no per-frame image compression
        world_left = bounds['min_x'] + bbox[0]
        world_top = bounds['min_y'] + bbox[1]
        dx = world_left - CANVAS_WORLD_LEFT
        dy = world_top - CANVAS_WORLD_TOP

        canvas = Image.new('RGBA', (W, H))
        if dx < 0 or dy < 0 or dx + subject.width > W or dy + subject.height > H:
            raise ValueError(f'Crouch candidate {low}/{up} cannot fit canonical 64x32 window')
        canvas.alpha_composite(subject, (dx, dy))
        canvas.save(OUT / f'crouch-candidate-{frame_index}.png')
        preview_sheet.alpha_composite(canvas, (frame_index * W, 0))
        all_words.extend(_tile_words_from_canvas(canvas, lookup))
        frames.append({
            'frame': frame_index,
            'lower_index': low,
            'upper_index': up,
            'png': f'crouch-candidate-{frame_index}.png',
            'bounds': bounds,
            'content_bbox': bbox,
            'canvas_offset_xy': [dx, dy],
            'gap_adjustments': gap_adjustments,
            'classification': 'ROM-derived candidate frame, lower component raised by 6 px; temporal role not dynamically confirmed',
        })

    preview_sheet.save(OUT / 'simon_crouch_preview.png')
    tiles_per_frame = (W // 32) * TILES_PER_HALF
    (OUT / 'simon_crouch_md.h').write_text(
        '/* Generated ROM-derived crouch frames. Never hand edit. */\n'
        '#ifndef VK_SIMON_CROUCH_MD_H\n'
        '#define VK_SIMON_CROUCH_MD_H\n'
        f'#define VK_CROUCH_FRAME_COUNT {len(PAIRS)}\n'
        f'#define VK_CROUCH_TILES_PER_FRAME {tiles_per_frame}\n'
        '#define VK_CROUCH_TILES_PER_HALF 16\n'
        '#define VK_CROUCH_TILE_COUNT (VK_CROUCH_FRAME_COUNT * VK_CROUCH_TILES_PER_FRAME)\n'
        f'extern const unsigned long vk_simon_crouch_tiles[{len(all_words)}];\n'
        '#endif\n'
    )
    lines = [
        '/* Generated from Vampire Killer MSX2 ROM. Crouch phase order from the openMSX trace. */',
        '#include "simon_crouch_md.h"',
        f'const unsigned long vk_simon_crouch_tiles[{len(all_words)}] = {{',
    ]
    for i in range(0, len(all_words), 8):
        lines.append('    ' + ', '.join('0x%08XUL' % v for v in all_words[i:i + 8]) + ',')
    lines.append('};')
    (OUT / 'simon_crouch_md.c').write_text('\n'.join(lines) + '\n')

    manifest = {
        'rom_sha256': hashlib.sha256(rom).hexdigest(),
        'asset_source': 'MSX2 original, position/mask tables',
        'phase_order_verified': True,
        'candidate_state': 'crouch / crouch-whip',
        'canvas_size': [W, H],
        'canvas_world_window': {'left': CANVAS_WORLD_LEFT, 'top': CANVAS_WORLD_TOP, 'width': W, 'height': H},
        'sgdk_anchor': 'buffer origin x-24,y-30 (64x32), two 32x32 halves at x-24 and x+8, SGDK y-25; left-facing: mirror, origin x-39',
        'phase_order_source': 'openMSX crouched-attack trace (C42E=6, C42F=6,6,7,7,8x8,0)',
        'frame_assignments': frames,
    }
    (OUT / 'simon_crouch_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


if __name__ == '__main__':
    generate()
