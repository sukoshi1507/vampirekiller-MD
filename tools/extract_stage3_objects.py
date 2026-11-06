#!/usr/bin/env python3
"""Extract the six Stage 3 object lists with original Z80 routines.
Requires pip install z80 (kosarev/z80), reference ROM supplied by user.
No inferred or synthetic object coordinates are emitted.
"""
import argparse, hashlib, json, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
EXPECTED = '99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('rom',type=Path); p.add_argument('output',type=Path)
    args=p.parse_args()
    data=args.rom.read_bytes(); sha=hashlib.sha256(data).hexdigest()
    if sha != EXPECTED: p.error('ROM SHA256 different from reference: '+sha)
    try:
        from castle_room_objects import decode_room
    except ImportError as exc:
        p.error('Dependency unavailable: '+str(exc)+'; install kosarev/z80 in an internet-enabled Python environment')
    result={'reference_sha256':sha,'method':'Z80 ROM execution 5A49 then 5B1B','rooms':[decode_room(data,3,i) for i in range(6)]}
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('Six original Stage 3 room object lists extracted to',args.output)
if __name__=='__main__':main()
