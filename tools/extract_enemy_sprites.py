#!/usr/bin/env python3
"""Extract the stage 1 hall enemy sprite layers (types A, B, C) from the reference ROM.

Usage: python3 -I tools/extract_enemy_sprites.py ROM.rom [OUTDIR]
OUTDIR defaults to scratchpad/enemy-assets/sprites (git-ignored). Nothing is written
into the repository tree except what the caller passes explicitly.

Provenance (verified in openMSX, see research/enemy-assets.md):
- Type A (stage 1, room 0/1 at Y A0/B0): VRAM pattern numbers 40/44 (top), 48/4C or
  50/54 (bottom, alternating). These layers are decoded from the RLE stream at file
  offset 0x153E8 (bank 10, offset 0x13E8), three 64-byte pairs.
- Type B: the same three pairs, horizontally mirrored (VRAM 58/5C, 60/64, 68/6C).
- Type C (stage 1, room 2 at Y 70): pattern numbers 70/74 and 78/7C, decoded from the
  stream at file offset 0x15947 at byte offsets 1002 and 1066.
Each pattern number pair N/N+4 is one 16x16 cell of two 1-bpp layers. A layer is
32 bytes: left column UL (bytes 0-7), LL (8-15), right column UR (16-23), LR (24-31).

Colour is NOT resolved: the sprite colour bytes of the SAT are 0 in the captured
frames and the VRAM colour table at F400 changes between scenes. The composite PNG
therefore uses layer indices (1 = layer N, 2 = layer N+4, 3 = both), not game colours.
The palette written in the PNG is the stage 1 room 0 palette (ROM), which the captured
VDP palette matched exactly. Index 0 is transparent.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe import load_rom, ROOT  # noqa: E402
from extract_rooms import decode_room  # noqa: E402

STREAMS = {
    'A': {'file_offset': 0x153E8, 'pairs': [0, 64, 128], 'mirror': False},
    'C': {'file_offset': 0x15947, 'pairs': [1002, 1066], 'mirror': False},
}
# Frames: list of (name, [(dx, dy, source_key, pair_index)]); source_key is A/B/C.
FRAMES = [
    ('typeA_frame0', [(0, 0, 'A', 0), (0, 16, 'A', 1)]),
    ('typeA_frame1', [(0, 0, 'A', 0), (0, 16, 'A', 2)]),
    ('typeB_frame0', [(0, 0, 'B', 0), (0, 16, 'B', 1)]),
    ('typeB_frame1', [(0, 0, 'B', 0), (0, 16, 'B', 2)]),
    ('typeC_frame0', [(0, 0, 'C', 0), (16, 0, 'C', 1)]),
]
PATTERN_NUMBERS = {'A': [(0x40, 0x44), (0x48, 0x4C), (0x50, 0x54)],
                   'B': [(0x58, 0x5C), (0x60, 0x64), (0x68, 0x6C)],
                   'C': [(0x70, 0x74), (0x78, 0x7C)]}


def rle_decode(rom, start):
    """RLE of the pattern streams: 0 ends, 80 jumps (bank-local), >80 literal, <80 repeat."""
    base = start & ~0x1FFF
    out = bytearray()
    q = start
    for _ in range(100000):
        tag = rom[q]
        q += 1
        if tag == 0:
            return bytes(out)
        if tag == 128:
            q = base + (rom[q] | (rom[q + 1] << 8)) % 0x2000
            continue
        count = tag & 127
        if tag > 128:
            out.extend(rom[q:q + count])
            q += count
        else:
            out.extend(bytes((rom[q],)) * count)
            q += 1
    raise ValueError('RLE stream does not terminate')


def mirror_layer(layer):
    """Horizontal flip of a 16x16 layer: quadrants swap and each row is bit-reversed."""
    ul, ll, ur, lr = layer[0:8], layer[8:16], layer[16:24], layer[24:32]
    rev = lambda q: bytes(int('{:08b}'.format(b)[::-1], 2) for b in q)
    return rev(ur) + rev(lr) + rev(ul) + rev(ll)


def layer_mask(layer):
    """Return 256 values (16x16, row-major), 1 where the layer has a pixel."""
    mask = []
    for y in range(16):
        for x in range(16):
            byte = layer[(16 if x >= 8 else 0) + y]
            mask.append((byte >> (7 - (x & 7))) & 1)
    return mask


def write_png4(path, width, height, indices, palette_rgb):
    """Indexed PNG, 4 bits per pixel, index 0 transparent."""
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        row = indices[y * width:(y + 1) * width]
        for x in range(0, width, 2):
            rows.append((row[x] << 4) | (row[x + 1] if x + 1 < width else 0))

    def chunk(kind, data):
        body = kind + data
        return struct.pack('>I', len(data)) + body + struct.pack('>I', zlib.crc32(body) & 0xFFFFFFFF)

    plte = b''.join(bytes(c) for c in palette_rgb)
    trns = bytes([0]) + bytes([255] * 15)
    data = (b'\x89PNG\r\n\x1a\n'
            + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 4, 3, 0, 0, 0))
            + chunk(b'PLTE', plte)
            + chunk(b'tRNS', trns)
            + chunk(b'IDAT', zlib.compress(bytes(rows), 9))
            + chunk(b'IEND', b''))
    path.write_bytes(data)


def main():
    if len(sys.argv) not in (2, 3):
        print(__doc__)
        return 2
    rom_path = Path(sys.argv[1])
    out = Path(sys.argv[2]) if len(sys.argv) == 3 else ROOT / 'scratchpad/enemy-assets/sprites'
    out.mkdir(parents=True, exist_ok=True)
    data = load_rom(rom_path)
    palette = decode_room(data, 1, 0)[0]['palette_rgb333']
    palette_rgb = [tuple(round(v * 255 / 7) for v in c) for c in palette]
    palette_rgb[0] = (0, 0, 0)

    layers = {}
    manifest = {'rom_sha256': hashlib.sha256(data).hexdigest(), 'streams': {},
                'palette_rgb333': [list(c) for c in palette], 'layers': {}, 'frames': {},
                'notes': ['index 0 transparent; 1 = layer N, 2 = layer N+4, 3 = both',
                          'colour not resolved: palette is stage 1 room 0 (ROM), preview only']}
    for key, spec in STREAMS.items():
        stream = rle_decode(data, spec['file_offset'])
        manifest['streams'][key] = {'file_offset': f"{spec['file_offset']:05X}",
                                    'length': len(stream)}
        for index, (pn_a, pn_b) in enumerate(PATTERN_NUMBERS[key]):
            start = spec['pairs'][index]
            layer_a = stream[start:start + 32]
            layer_b = stream[start + 32:start + 64]
            layers[(key, index, 0)] = layer_a
            layers[(key, index, 1)] = layer_b
            for side, pn in ((0, pn_a), (1, pn_b)):
                manifest['layers'][f'{key}_{index}_{pn:02X}'] = {
                    'pattern': f'{pn:02X}', 'stream_offset': start + 32 * side,
                    'file_offset': f"{spec['file_offset']:05X}", 'mirror': False,
                    'sha256': hashlib.sha256(stream[start + 32 * side:start + 32 * side + 32]).hexdigest()}
    # Type B: mirrored copies of the type A pairs, with the VRAM pattern numbers of B.
    b_pattern = {0: (0x58, 0x5C), 1: (0x60, 0x64), 2: (0x68, 0x6C)}
    for index in range(3):
        for side in (0, 1):
            layer = mirror_layer(layers[('A', index, side)])
            layers[('B', index, side)] = layer
            pn = b_pattern[index][side]
            manifest['layers'][f'B_{index}_{pn:02X}'] = {
                'pattern': f'{pn:02X}', 'source': f'A_{index}_{PATTERN_NUMBERS["A"][index][side]:02X}',
                'mirror': True, 'sha256': hashlib.sha256(layer).hexdigest()}

    def cell(source, index):
        return layers[(source, index, 0)], layers[(source, index, 1)]

    for name, parts in FRAMES:
        width = 32 if name.startswith('typeC') else 16
        height = 16 if name.startswith('typeC') else 32
        indices = [0] * (width * height)
        cells = []
        for dx, dy, source, index in parts:
            first, second = cell(source, index)
            cells.append((dx, dy, first, second))
            m0, m1 = layer_mask(first), layer_mask(second)
            for y in range(16):
                for x in range(16):
                    k = y * 16 + x
                    value = (1 if m0[k] else 0) | (2 if m1[k] else 0)
                    if value:
                        indices[(dy + y) * width + dx + x] = value
        write_png4(out / f'{name}.png', width, height, indices, palette_rgb)
        manifest['frames'][name] = {'size': [width, height], 'cells': [
            {'dx': dx, 'dy': dy, 'source': src, 'pair': index} for dx, dy, src, index in parts]}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'{len(FRAMES)} frames, {len(manifest["layers"])} layers -> {out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
