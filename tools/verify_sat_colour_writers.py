#!/usr/bin/env python3
"""SAT colour bytes (+25+5k of a C800/D700 block, read by 651B) and CE03 (read by 9FC2): who writes them, in the ROM.

The allocator frees a SAT entry only when its Y is E0 after 64EC/64E5, and 651B forces Y=E1 for a colour whose low nibble is 0, so
a colour change can only matter if it makes a low nibble 0 (or non-zero again).  This checks, on the reference ROM:
 1. every direct store into IX+25/2A/2F/34/39/3E/43/48 (LD (IX+d),n / LD (IX+d),r) and its value;
 2. the only SET 5,L users (the +20 record walkers): 6029 (colour table copy at construction), 6445, 6501 (copy to D638), 9A00 (frees
    the Y of a dead object's entries), A01A (colour table copy of a D700 constructor);
 3. nothing stores to CE03 by address, and no routine loads a CE00-based pointer other than CE01/CE02 (so CE03 stays 0: it is
    read once, at 9FC2, and never set).
Run: python3 tools/verify_sat_colour_writers.py  (no emulator needed)."""
import re
import sys
from probe import load_rom, reference_rom

COLOURS = {0x25 + 5 * k for k in range(8)}
# (cpu address, offset, how the value is known, owner) -- exhaustive list found by the scan below.
EXPECTED_IX = {
    0x9C2A: ('LD (IX+25),A', 'table 9C59 = 08 01 0E 01 (C003 & 3)', 'type 26h flame, never 0'),
    0x9C75: ('LD (IX+25),n', 0x0B, 'type 26h flame, stage 10'),
    0xA28A: ('LD (IX+25),n', 0x02, 'type 0Ah statue after 9F6D: 02/4C/02/4C'),
    0xA28E: ('LD (IX+2A),A', 0x4C, 'type 0Ah statue'),
    0xA291:('LD (IX+2F),n', 0x02, 'type 0Ah statue'),
    0xA295: ('LD (IX+34),A', 0x4C, 'type 0Ah statue'),
    0xACB2: ('LD (IX+25),A', 0x00, 'type 11h (stage 18 body) A=0: all four layers invisible'),
    0xACB5: ('LD (IX+2A),A', 0x00, 'type 11h'),
    0xACB8: ('LD (IX+2F),A', 0x00, 'type 11h'),
    0xACBB: ('LD (IX+34),A', 0x00, 'type 11h'),
    0xAF55: ('LD (IX+2F),A', 0x00, 'type 09h AF4A: hit/dormant'),
    0xAF58: ('LD (IX+34),A', 0x00, 'type 09h AF4A'),
    0xAF85: ('LD (IX+2F),A', 'copy of IX+25 (02)', 'type 09h AF7E: active'),
    0xAF88: ('LD (IX+34),n', 0x45, 'type 09h AF7E'),
    0xB053: ('LD (IX+2F),A', 0x00, 'type 09h B048 (killed 09h becomes dormant)'),
    0xB056: ('LD (IX+34),A', 0x00, 'type 09h B048'),
    0xB96F: ('LD (IX+25),n', 0x02, 'type 14h stage 9 boss flash'),
    0xB973:('LD (IX+2A),A', 'A = 08 or 4C (bit 0 of IX+11)', 'type 14h stage 9 boss flash, never 0'),
    0xB976: ('LD (IX+2F),n', 0x02, 'type 14h'),
    0xB97A: ('LD (IX+34),A', 'A = 08 or 4C', 'type 14h'),
    0xB97D: ('LD (IX+39),n', 0x02, 'type 14h'),
    0xB981: ('LD (IX+3E),A', 'A = 08 or 4C', 'type 14h'),
}
# LD (IX+d),n / LD (IX+d),A are the only encodings that exist; the 25 offsets 25..48 also appear in 'ADD A,(IX+d)' etc. (reads).
EXPECTED_SET5L = {0x6037, 0x6464, 0x6501, 0x9A00, 0xA01A}


def main():
    rom = bytes(load_rom(reference_rom()))
    found = {}
    for m in re.finditer(b'\xdd', rom):
        i = m.start()
        if i + 3 >= len(rom):
            continue
        op, d = rom[i + 1], rom[i + 2]
        if d in COLOURS and op in (0x36, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x77, 0x34, 0x35):
            found[i + 0x4000] = (op, d, rom[i + 3] if op == 0x36 else None)   # file offset + 4000 (banks 0-3 at 4000-BFFF)
    got = found
    for a in sorted(got):
        if a not in EXPECTED_IX:
            sys.exit('unexpected colour writer at %04X: %s' % (a, got[a]))
    for a in EXPECTED_IX:
        if a not in got:
            sys.exit('missing colour writer at %04X' % a)
    # Immediate values.
    for a, (_, val, _) in ((a, EXPECTED_IX[a]) for a in EXPECTED_IX):
        if isinstance(val, int) and got[a][0] == 0x36:
            assert got[a][2] == val, (hex(a), got[a], val)
    flame = bytes(rom[0x9C59 - 0x4000:0x9C5D - 0x4000])
    assert flame == bytes((0x08, 0x01, 0x0E, 0x01)), flame.hex()
    assert all(v & 15 for v in flame)
    # SET 5,L users in the whole 128 KB image (file offset + 4000 is the CPU address of banks 0-3; no other bank holds one).
    users = {0x4000 + m.start() for m in re.finditer(b'\xcb\xed', rom)}
    assert users == EXPECTED_SET5L, sorted(hex(a) for a in users)
    # CE03: no direct store, and no CE00-based pointer except CE01/CE02 loads.
    for m in re.finditer(b'\x03\xce', rom):
        i = m.start()
        assert i >= 1 and rom[i - 1] in (0x3A,), ('CE03 not read-only', hex(0x4000 + i))   # LD A,(CE03) only (9FC2)
    for base in (b'\x21\x00\xce', b'\x11\x00\xce', b'\x01\x00\xce', b'\xdd\x21\x00\xce', b'\xfd\x21\x00\xce', b'\x21\x03\xce',
                 b'\x22\x03\xce', b'\x32\x03\xce'):
        assert base not in rom, base.hex()
    print('SAT colour writers: %d direct stores (all expected), %d SET 5,L users, CE03 read-only (9FC2): PASS'
          % (len(got), len(users)))


if __name__ == '__main__':
    main()
