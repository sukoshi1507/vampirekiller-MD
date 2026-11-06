#!/usr/bin/env python3
"""Export verified Simon mask pairs and report incompatible layouts; no guessed animation labels."""
import argparse, json, hashlib, sys
from pathlib import Path
from PIL import Image, ImageDraw
from extract_player import extract_pose, decode_rle
from extract_rooms import RomView, decode_room
from probe import load_rom

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('rom',type=Path)
    ap.add_argument('--output',type=Path,default=Path('research/simon-assets'))
    args=ap.parse_args()
    rom=load_rom(args.rom)
    args.output.mkdir(parents=True,exist_ok=True)
    palette=decode_room(rom,1,0)[0]['palette_rgb333']
    colors=[tuple(round(v*255/7) for v in color) for color in palette]
    pat=RomView(rom,(0,11,12,13))
    pos=RomView(rom,(0,1,2,3))
    data=[]; preview=[]
    for i in range(24):
        entry={'index':i, 'role':'unidentified', 'source_type':'MSX2 ROM'}
        for name,tab,coords in [('lower',0xA281,0x7985),('upper',0xA2D1,0x79D5)]:
            try:
                address=pat.word(tab+i*2)
                raw=decode_rle(pat,address)
                coord=pos.word(coords+i*2)
                entry[name]={'pattern_cpu':f'{address:04X}', 'pattern_file_offset':f'{pat.offset(address):05X}', 'decoded_bytes':len(raw),'coordinate_cpu':f'{coord:04X}', 'component_count':pos.byte(coord)}
            except (ValueError,IndexError) as e:
                entry[name]={'error':str(e)}
        try:
            pixels,sources=extract_pose(rom,i,i)
            entry['status']='decoded_24x32';entry['sources']=sources
            pic=Image.new('RGBA',(24,32))
            pic.putdata([(*colors[p],255) if p else (0,0,0,0) for p in pixels])
            filename=f'candidate-{i:02d}.png';pic.save(args.output/filename)
            entry['image']=filename;entry['opaque_pixels']=sum(bool(x) for x in pixels)
            preview.append((i,pic))
        except (ValueError,IndexError) as e:
            entry['status']='different_layout';entry['reason']=str(e)
        data.append(entry)
    cols=5; cell_w=132; cell_h=158
    sheet=Image.new('RGB',(cols*cell_w,((len(preview)+cols-1)//cols)*cell_h),(31,35,45))
    draw=ImageDraw.Draw(sheet)
    for j,(index,im) in enumerate(preview):
        x=(j%cols)*cell_w;y=(j//cols)*cell_h
        scaled=im.resize((96,128),Image.Resampling.NEAREST)
        sheet.paste(scaled,(x+18,y+6),scaled)
        draw.text((x+18,y+137),f'MSX2 index {index:02}', fill='white')
    sheet.save(args.output/'catalog-sheet.png')
    manifest={'sha256':hashlib.sha256(rom).hexdigest(),'note':'Source-confirmed pixel masks; animation meanings not identified. Jump frame may need positional correction and nonmatching lower/upper pairing.', 'decoded':len(preview),'total':len(data),'frames':data}
    (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'{len(preview)} decoded of {len(data)}; output: {args.output}')
if __name__=='__main__':main()
