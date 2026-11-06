#!/usr/bin/env python3
"""Check enemy object update dispatch table 9942 (bank 02, 8 KiB mapper).

Usage: python tools/verify_late_enemy_routines.py ROM [--write]
Read-only verification by default. No inference of AI behavior from an address.
"""
import argparse
import json
from pathlib import Path
from probe import load_rom,physical_offset
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('rom',type=Path)
p.add_argument('--write',action='store_true')
a=p.parse_args()
rom=load_rom(a.rom)
# The table at CPU 9942 contains update addresses for raw object ids 01h..24h.
# Bank 02 is selected at CPU window 8000h for this routine (mapper table).
o=physical_offset(2,0x9942,0x8000)
types=(0x09,0x0B,0x0D,0x0E,0x10,0x12)
addresses={t:int.from_bytes(rom[o+2*(t-1):o+2*t],'little') for t in types}
expected={0x09:0xAF5F,0x0B:0xB37E,0x0D:0xB248,0x0E:0xA9B7,0x10:0xADF8,0x12:0xBE70}
assert addresses==expected,(addresses,expected)
hdr='''/* ROM dispatch table at CPU 9942, mapper bank 02 in window 8000.
 * These addresses identify updates only; they do not port the routines. */
#ifndef VK_LATE_ENEMY_ROUTINES_H
#define VK_LATE_ENEMY_ROUTINES_H
static const unsigned short vk_late_update_address[6] = {'''+','.join('0x%04X'%addresses[t] for t in types)+'''};
static const unsigned char vk_late_update_type[6] = {'''+','.join('0x%02X'%t for t in types)+'''};
#endif
'''
path=ROOT/'sgdk/src/generated/late_enemy_routines.h'
if a.write: path.write_text(hdr)
else: assert path.read_text()==hdr
payload={'rom_sha256':'99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2','bank':2,'cpu_table':'0x9942','file_offset':hex(o),'objects':[{ 'type':hex(t),'update_cpu':hex(addresses[t]),'ported_existing':t in (11,13,14)} for t in types]}
out=ROOT/'research/late-enemy-update-dispatch.json'
if a.write: out.write_text(json.dumps(payload,indent=2)+'\n')
else: assert json.loads(out.read_text())==payload
print('ROM 9942 dispatch: six late fixed types, six exact update pointers PASS')
