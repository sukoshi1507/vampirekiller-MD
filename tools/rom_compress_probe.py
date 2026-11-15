#!/usr/bin/env python3
"""Measured compression and estimated 68000 unpack cost of the ROM data blocks (research/rom-budget.md).

For each data symbol of sgdk/out/rom.out, packs the real bytes with the SGDK packers (LZ4W and aPLib
"pp", the jars of $GDK/bin) whole and in units (a room, a bust set...), checks that aPLib unpacks back
to the original, and counts the 68000 cycles of the SGDK unpackers (tools_a.s) on the real packed
streams with a per-instruction model (bsr/rts/dbf/move timings of the 68000; no wait states, ROM reads
are 4-cycle accesses as RAM ones). 488 cycles = one display line at 7.67 MHz (NTSC 262 lines).
Nothing is written in the repository; temporaries live in a mktemp directory.

Usage: python3 tools/rom_compress_probe.py [--unit N] [--elf rom.out] SYMBOL...
"""
import argparse
import fnmatch
import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GDK = Path(os.environ.get('GDK', '/home/sukoshi/Documents/githubsuko/SGDK'))
CYCLES_PER_LINE = 488

JAVA = r'''
import java.nio.file.*;
import java.util.*;
public class RomPack {
    public static void main(String[] a) throws Exception {
        byte[] d = Files.readAllBytes(Paths.get(a[0]));
        List<String> sizes = Files.readAllLines(Paths.get(a[1]));
        Path out = Paths.get(a[2]);
        int n = 0, i = 0;
        for (String sz : sizes) {
            byte[] u = Arrays.copyOfRange(d, i, i + Integer.parseInt(sz.trim()));
            i += u.length;
            Files.write(out.resolve(String.format("%05d.lz4w", n)), sgdk.lz4w.LZ4W.pack(u, 0, true));
            Files.write(out.resolve(String.format("%05d.apl", n)), sgdk.aplib.APJ.pack(u, true, true));
            n++;
        }
    }
}
'''


def symbols(elf):
    out = {}
    for line in subprocess.run(['m68k-elf-nm', '-S', '-t', 'd', str(elf)], check=True, capture_output=True,
                               text=True).stdout.splitlines():
        f = line.split(None, 3)
        name = re.sub(r'(\.(constprop|isra|part|lto_priv|cold)\.\d+)+$', '', f[-1])
        if len(f) == 4 and f[2] in 'tTrR':
            out[name] = (int(f[0]), int(f[1]))
        elif len(f) == 3 and f[1] == 'T':          # BIN resource: symbol without size, NAME_size beside it
            out.setdefault(f[2], (int(f[0]), 0))
        elif len(f) == 3 and f[1] == 'A' and f[2].endswith('_size') and f[2][:-5] in out:
            out[f[2][:-5]] = (out[f[2][:-5]][0], int(f[0]))
    return out


def lz4w_cycles(p, size):
    """Cycles of lz4w_unpack on stream p (each block: LLLL MMMM OOOOOOOO, see tools_a.s)."""
    cyc = 60                                    # movem, lea, first LZ4W_NEXT
    i, out = 0, 0
    while True:
        seg = struct.unpack_from('>H', p, i)[0]
        i += 2
        lit, mat, off = seg >> 12, (seg >> 8) & 15, seg & 255
        if not (lit or mat or off):
            break
        cyc += 56 + 10 * lit + (2 if lit & 1 else 0)    # header NEXT + bra, move.l pairs (10 c/word), odd move.w
        i += 2 * lit
        out += 2 * lit
        if mat:                                          # short match: add/neg/lea then move.w x (M+1)
            cyc += 20 + 12 * (mat + 1)
            out += 2 * (mat + 1)
        elif off:                                        # long match: 62 + words (RAM move.w 12, ROM pairs 10)
            v = struct.unpack_from('>H', p, i)[0]
            i += 2
            words = off + 2
            cyc += 62 + (10 if v & 0x8000 else 12) * words
            out += 2 * words
        else:
            cyc += 14
    cyc += 40
    out += 1 if struct.unpack_from('>H', p, i)[0] & 0x8000 else 0
    return cyc, out


def aplib_decode(p):
    """aPLib decoder following aplib_unpack in tools_a.s: (output bytes, cycles)."""
    src, out = 0, bytearray()
    tag = 0x80
    cyc = 0

    def bit():
        nonlocal tag, src, cyc
        tag <<= 1
        c = tag >> 8
        tag &= 0xFF
        cyc += 18 + 4 + 16
        if tag:
            cyc += 10
        else:
            cyc += 8 + 8 + 4
            b = p[src]
            src += 1
            t = b * 2 + c
            c = t >> 8
            tag = t & 0xFF
        return c

    def gamma():
        nonlocal cyc
        v = 1
        cyc += 4 + 18 + 16                     # moveq, bsr, rts
        while True:
            v = v * 2 + bit()
            cyc += 8                           # addx.l
            more = bit()
            cyc += 8 if not more else 10       # bcs
            if not more:
                return v

    cyc += 120                                 # movem, lea x3, move, moveq
    out.append(p[src])
    src += 1
    cyc += 12
    lwm = 2
    old = 0
    while True:
        cyc += 4                               # moveq #2 / next_sequence
        if not bit():
            cyc += 10
            out.append(p[src])
            src += 1
            cyc += 12
            lwm = 2
            continue
        cyc += 8
        if not bit():                          # code pair %10
            cyc += 10
            g = gamma()
            cyc += 6
            if g - lwm == 0:
                off = old
                cyc += 8 + 4
                length = gamma()
            else:
                cyc += 8 + 8 + 22 + 8 + 4
                off = ((g - lwm - 1) << 8) | p[src]
                src += 1
                length = gamma()
                cyc += 6 + 8 + 6 + 8 + 6 + 8
                if off >= 32000:
                    length += 2
                elif off >= 1280:
                    length += 1
                elif off >= 128:
                    length += 0
                else:
                    length += 2
                old = off
                cyc += 4
            cyc += 4 + 8 + 8
            for _ in range(length):
                out.append(out[-off])
            cyc += 22 * length - 4
            cyc += 4 + 10
            lwm = 1
            continue
        cyc += 8
        if not bit():                          # short match %110
            cyc += 10
            b = p[src]
            src += 1
            cyc += 4 + 8 + 6
            off = b >> 1
            if off == 0:
                cyc += 8 + 6 + 8 + 40
                return bytes(out), cyc
            length = 3 if b & 1 else 2
            cyc += 10 + (4 if not b & 1 else 0) + 10
            old = off
            cyc += 4 + 4 + 8 + 8
            for _ in range(length):
                out.append(out[-off])
            cyc += 22 * length - 4 + 4 + 10
            lwm = 1
            continue
        cyc += 8
        v = 0
        for _ in range(4):
            v = v * 2 + bit()
            cyc += 8 + 10
        cyc += 4 + 8
        if v:
            cyc += 4 + 8 + 8
            v = out[-v]
        out.append(v)
        cyc += 8 + 10 + 4
        lwm = 2


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--elf', default=str(ROOT / 'sgdk/out/rom.out'))
    ap.add_argument('--unit', type=int, default=0, help='unit size in bytes (default: the whole block)')
    ap.add_argument('--lowbyte', action='store_true', help='keep the low byte of each big-endian u16 (checks the high byte is 0)')
    ap.add_argument('--per-symbol', action='store_true', help='with a * pattern: one unit per matching symbol')
    ap.add_argument('names', nargs='+')
    args = ap.parse_args()
    syms = symbols(args.elf)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / 'RomPack.java').write_text(JAVA)
        cp = f'{GDK / "bin" / "lz4w.jar"}:{GDK / "bin" / "apj.jar"}'
        subprocess.run(['javac', '-cp', cp, '-d', str(tmp), str(tmp / 'RomPack.java')], check=True)
        subprocess.run(['m68k-elf-objcopy', '-O', 'binary', args.elf, str(tmp / 'image.bin')], check=True)
        rom = (tmp / 'image.bin').read_bytes()
        for name in args.names:
            if any(c in name for c in '*?['):    # several symbols, their bytes one after the other
                found = sorted((k for k in syms if fnmatch.fnmatchcase(k, name)),
                               key=lambda k: [int(t) if t.isdigit() else t for t in re.split(r'(\d+)', k)])
                data = b''.join(rom[syms[k][0]:syms[k][0] + syms[k][1]] for k in found)
                size = len(data)
                sizes = [syms[k][1] for k in found]
                print(f'{name}: {len(found)} symbols')
            elif name.startswith('@'):          # a directory of files, one unit each
                files = sorted(Path(name[1:]).iterdir(), key=lambda f: [int(t) if t.isdigit() else t for t in re.split(r'(\d+)', f.name)])
                data = b''.join(f.read_bytes() for f in files)
                size = len(data)
                sizes = [f.stat().st_size for f in files]
            else:
                addr, size = syms[name]
                data = rom[addr:addr + size]
            if args.lowbyte:
                if any(data[0::2]):
                    sys.exit(f'{name}: a high byte is not 0')
                data = data[1::2]
                size //= 2
            unit = (args.unit // 2 if args.lowbyte else args.unit) or size
            if not name.startswith('@') and not (any(c in name for c in '*?[') and args.per_symbol):
                sizes = [min(unit, size - i) for i in range(0, size, unit)]
            (tmp / 'sizes.txt').write_text('\n'.join(map(str, sizes)))
            (tmp / 'in.bin').write_bytes(data)
            outd = tmp / 'out'
            outd.mkdir(exist_ok=True)
            for f in outd.iterdir():
                f.unlink()
            subprocess.run(['java', '-cp', f'{cp}:{tmp}', 'RomPack', str(tmp / 'in.bin'), str(tmp / 'sizes.txt'), str(outd)],
                           check=True)
            tl = ta = cl = ca = 0
            worst_l = worst_a = 0
            pos = 0
            for n, length in enumerate(sizes):
                chunk = data[pos:pos + length]
                pos += length
                pl = (outd / f'{n:05d}.lz4w').read_bytes()
                pa = (outd / f'{n:05d}.apl').read_bytes()
                got, ca_n = aplib_decode(pa)
                if got != chunk:
                    sys.exit(f'{name} unit {n}: aPLib model decode differs from the original')
                cl_n, olen = lz4w_cycles(pl, len(chunk))
                if olen != len(chunk):
                    sys.exit(f'{name} unit {n}: LZ4W model parsed {olen} bytes of {len(chunk)}')
                tl += len(pl)
                ta += len(pa)
                cl += cl_n
                ca += ca_n
                worst_l = max(worst_l, cl_n)
                worst_a = max(worst_a, ca_n)
            units = len(sizes)
            print(f'{name}: {size} B in {units} unit(s) of {"%d" % unit if len(set(sizes)) == 1 else "%d..%d" % (min(sizes), max(sizes))}')
            print(f'  LZ4W : {tl:>8} B ({100 * tl / size:5.1f} %), saves {size - tl:>7}; unpack {cl / size:5.1f} c/B, '
                  f'{cl / units / CYCLES_PER_LINE:6.1f} lines/unit (worst {worst_l / CYCLES_PER_LINE:6.1f}), '
                  f'all {cl / CYCLES_PER_LINE:7.0f} lines')
            print(f'  aPLib: {ta:>8} B ({100 * ta / size:5.1f} %), saves {size - ta:>7}; unpack {ca / size:5.1f} c/B, '
                  f'{ca / units / CYCLES_PER_LINE:6.1f} lines/unit (worst {worst_a / CYCLES_PER_LINE:6.1f}), '
                  f'all {ca / CYCLES_PER_LINE:7.0f} lines')


if __name__ == '__main__':
    sys.exit(main())
