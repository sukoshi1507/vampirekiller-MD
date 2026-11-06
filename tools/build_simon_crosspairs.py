#!/usr/bin/env python3
"""Render lower/upper ROM sprite combinations without assuming animation timing."""
import argparse, json, hashlib
from pathlib import Path
from PIL import Image, ImageDraw
from decode_extended_simon import components, render
from extract_rooms import decode_room
from probe import load_rom

LOWERS=(0,1,2,6,10,11,12,16)
UPPERS=(0,2,6,9,12,15,16,17)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('rom',type=Path)
    ap.add_argument('--output',type=Path,default=Path('research/simon-assets/crosspairs'))
    args=ap.parse_args()
    rom=load_rom(args.rom)
    args.output.mkdir(parents=True,exist_ok=True)
    rgb=decode_room(rom,1,0)[0]['palette_rgb333']
    colors=[tuple(round(v*255/7) for v in col) for col in rgb]
    cache={('lower',i):components(rom,i,'lower') for i in LOWERS}
    cache.update({('upper',i):components(rom,i,'upper') for i in UPPERS})
    cell_w,cell_h=145,145
    sheet=Image.new('RGB',(cell_w*len(UPPERS),cell_h*len(LOWERS)),(28,34,45))
    draw=ImageDraw.Draw(sheet)
    records=[]
    for r,low in enumerate(LOWERS):
        for c,up in enumerate(UPPERS):
            parts=[cache['lower',low],cache['upper',up]]
            im,orphans,bounds=render(parts,colors)
            name=f'lower-{low:02d}_upper-{up:02d}.png'
            im.save(args.output/name)
            factor=max(1,min(3,110//max(im.width,im.height)))
            scaled=im.resize((im.width*factor,im.height*factor),Image.Resampling.NEAREST)
            x=c*cell_w+(cell_w-scaled.width)//2
            y=r*cell_h+7
            sheet.paste(scaled,(x,y),scaled)
            draw.text((c*cell_w+8,r*cell_h+119),f'LOW {low:02d} / UP {up:02d}',fill='white')
            if orphans:draw.text((c*cell_w+8,r*cell_h+132),f'{len(orphans)} layers without coords',fill=(255,177,115))
            records.append({'lower':low,'upper':up,'png':name,'bounds':bounds,'width':im.width,'height':im.height,
                            'orphan_layers':len(orphans),'status':'partial' if orphans else 'fully_positioned',
                            'interpretation':'candidate composition, NOT a verified animation frame'})
    sheet.save(args.output/'crosspair-sheet.png')
    meta={'rom_sha256':hashlib.sha256(rom).hexdigest(),'lower_indices':LOWERS,'upper_indices':UPPERS,
          'entries':records,'warning':'Spatial compositions based on ROM sprite tables. Selection order, timing, and meaning are not established. No changes made to the Mega Drive runtime.'}
    (args.output/'crosspair-manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(f'{len(records)} combinations; {sum(v["status"]=="fully_positioned" for v in records)} fully positioned')
if __name__=='__main__':main()
