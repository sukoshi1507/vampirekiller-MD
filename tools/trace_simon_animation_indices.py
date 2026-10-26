#!/usr/bin/env python3
"""Evidence-based ROM scan of the two independent Simon animation selectors.
This static scan finds direct writes and records bytes for reproducibility;
it intentionally does not infer timing from unexecuted branches.
"""
import argparse, json, hashlib
from pathlib import Path
from probe import load_rom

SELECTORS = {0xC42E: 'lower', 0xC42F: 'upper'}

def audit(rom):
    records = []
    for target, name in SELECTORS.items():
        for bank in range(16):
            chunk = rom[bank*8192:(bank+1)*8192]
            for opcode, operation in ((0x32, 'LD (nn),A'), (0x22, 'LD (nn),HL'), (0xED, 'LD (nn),BC [ED43]')):
                pat = (bytes((opcode, target&255, target>>8)) if opcode != 0xED else bytes((0xED,0x43,target&255,target>>8)))
                start=0
                while True:
                    ix=chunk.find(pat,start)
                    if ix<0:break
                    off=bank*8192+ix
                    # Only bank0 fixed 4000-5FFF or bank1 in default 6000-7FFF;
                    # others have ambiguous runtime mapper windows.
                    cpu = (0x4000+ix if bank==0 else 0x6000+ix if bank==1 else None)
                    records.append({'selector':name,'target_ram':f'{target:04X}', 'operation':operation,
                                    'bank':bank,'rom_offset':f'{off:05X}', 'cpu_address_default':f'{cpu:04X}' if cpu is not None else None,
                                    'nearby_hex':rom[max(0,off-8):off+len(pat)+5].hex(' ')})
                    start=ix+1
    records.sort(key=lambda r:(r['selector'],int(r['rom_offset'],16)))
    return {'rom_sha256':hashlib.sha256(rom).hexdigest(),'selector_roles':{'lower':'C42E -> table A281 and positions 7985','upper':'C42F -> table A2D1 and positions 79D5'},
            'warning':'Pattern matches are candidate instruction sites; they require control-flow confirmation. No frame timings are established.',
            'direct_write_candidates':records}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('--output',type=Path,default=Path('research/simon-assets/animation-selector-audit.json'));a=p.parse_args()
    result=audit(load_rom(a.rom));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(len(result['direct_write_candidates']),'direct-write candidates')
