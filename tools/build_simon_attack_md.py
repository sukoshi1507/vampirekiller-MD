#!/usr/bin/env python3
"""Generate conservative 4-phase SGDK attack candidates from verified ROM layers.

The graphics are ROM-derived. Phase assignment is NOT verified by a live Z80 trace.
"""
import argparse,hashlib,json,sys
from pathlib import Path
from PIL import Image
from decode_extended_simon import components,render
from extract_rooms import decode_room
from probe import load_rom

PAIRS=[(0,0),(0,6),(0,9),(0,6)]
W=64;H=32

def generate(rom,out):
    palette=decode_room(rom,1,0)[0]['palette_rgb333']
    colors=[tuple(round(v*255/7) for v in c) for c in palette]
    lookup={(*colors[k],255):k for k in range(4)}
    alltiles=[];manifest=[]
    out.mkdir(parents=True,exist_ok=True)
    for index,(low,up) in enumerate(PAIRS):
        pic,orphans,bounds=render([components(rom,low,'lower'),components(rom,up,'upper')],colors)
        if orphans:raise ValueError(f'Incomplete candidate {low}/{up}: {len(orphans)} layers unplaced')
        canvas=Image.new('RGBA',(W,H))
        dx=bounds['min_x']+32;dy=bounds['min_y']+30
        if dx<0 or dy<0 or dx+pic.width>W or dy+pic.height>H:raise ValueError('Candidate cannot fit anchored 64x32 buffer')
        canvas.alpha_composite(pic,(dx,dy))
        canvas.save(out/f'attack-candidate-{index}.png')
        pixels=[lookup.get(pixel,0) if pixel[3] else 0 for pixel in canvas.getdata()]
        if any(pixel[3] and pixel not in lookup for pixel in canvas.getdata()):raise ValueError('ROM sprite palette exceeds 2-bit mask')
        # Each 32x32 half is one SGDK sprite: 16 contiguous tiles in column-major order.
        tiles=[]
        for half in range(2):
            for tx in range(4):
                for ty in range(4):
                    for py in range(8):
                        word=0
                        for px in range(8):
                            x=half*32+tx*8+px;y=ty*8+py
                            word=(word<<4)|pixels[y*W+x]
                        tiles.append(word)
        assert len(tiles)==256
        alltiles.extend(tiles)
        manifest.append({'frame':index,'lower_index':low,'upper_index':up,'bounds':bounds,'png':f'attack-candidate-{index}.png','classification':'ROM-derived candidate, NOT dynamically confirmed attack timing'})
    header='''/* Generated ROM-derived candidate frames. Never hand edit. */
#ifndef VK_SIMON_ATTACK_MD_H
#define VK_SIMON_ATTACK_MD_H
#define VK_ATTACK_FRAME_COUNT 4
#define VK_ATTACK_TILES_PER_FRAME 32
#define VK_ATTACK_TILES_PER_HALF 16
extern const unsigned long vk_simon_attack_tiles[1024];
#endif
'''
    (out/'simon_attack_md.h').write_text(header)
    lines=['/* Generated from Vampire Killer MSX2 ROM. Phase sequence HYPOTHETICAL. */','#include "simon_attack_md.h"','const unsigned long vk_simon_attack_tiles[1024] = {']
    for i in range(0,len(alltiles),8):lines.append('    '+', '.join('0x%08XUL'%x for x in alltiles[i:i+8])+',')
    lines.append('};')
    (out/'simon_attack_md.c').write_text('\n'.join(lines)+'\n')
    (out/'attack_candidates_manifest.json').write_text(json.dumps({'rom_sha256':hashlib.sha256(rom).hexdigest(),'asset_source':'MSX2 original, position/mask tables','phase_order_verified':False,'canvas_anchor':'image origin at Simon x-32,y-30; mirror only for left-facing','frame_assignments':manifest},indent=2)+'\n')
    return manifest

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('--output',type=Path,default=Path('sgdk/src/generated'))
    a=p.parse_args();print('Generated',len(generate(load_rom(a.rom),a.output)),'candidate frames')
