#!/usr/bin/env python3
"""Decode extended MSX2 sprite compositions; preserve uncertain layer mappings."""
import argparse, hashlib, json, struct
from pathlib import Path
from PIL import Image,ImageDraw
from extract_player import decode_rle
from extract_rooms import RomView,decode_room
from probe import load_rom

FAILED_INDICES=(6,7,8,9,10,11,12,13,14,19,20,21,22,23)

def components(rom,index,part):
    pat=RomView(rom,(0,11,12,13)); pos=RomView(rom,(0,1,2,3))
    tab=0xA281 if part=='lower' else 0xA2D1
    ctab=0x7985 if part=='lower' else 0x79D5
    addr=pat.word(tab+index*2); raw=decode_rle(pat,addr)
    if len(raw)%32: raise ValueError('RLE data not aligned to 16x16 planar bitmap')
    coord=pos.word(ctab+index*2); count=pos.byte(coord)
    if count>32: raise ValueError('Unreasonable position count')
    offsets=[struct.unpack('bb',pos.read(coord+1+2*j,2)) for j in range(count)]
    layers=[]
    for l in range(len(raw)//32):
        mask=[]
        for y in range(16):
            for x in range(16): mask.append(bool(raw[l*32+y+(16 if x>=8 else 0)]&(128>>(x&7))))
        layers.append(mask)
    return {'part':part,'pattern_cpu':f'{addr:04X}','pattern_file_offset':f'{pat.offset(addr):05X}',
            'coordinate_cpu':f'{coord:04X}','coordinate_file_offset':f'{pos.offset(coord):05X}',
            'layer_count':len(layers),'position_count':count,'offsets_dy_dx':offsets,'layers':layers}

def render(parts,colors):
    # Display as anchored MSX position data, no cropping, center offsets determined by bounds.
    located=[]; orphan=[]
    for part in parts:
        for k,mask in enumerate(part['layers']):
            if k>=part['position_count']:
                orphan.append((part['part'],k,mask)); continue
            dy,dx=part['offsets_dy_dx'][k]
            located.append((part['part'],k,dx,dy,mask))
    xmin=min((x for _,_,x,_,_ in located),default=0); ymin=min((y for _,_,_,y,_ in located),default=0)
    xmax=max((x+16 for _,_,x,_,_ in located),default=16);ymax=max((y+16 for _,_,_,y,_ in located),default=16)
    w=xmax-xmin;h=ymax-ymin
    pixels=[0]*(w*h)
    for part,k,dx,dy,mask in located:
        bit=1 if k%2==0 else 2
        for y in range(16):
            for x in range(16):
                if mask[y*16+x]:pixels[(dy-ymin+y)*w+(dx-xmin+x)]|=bit
    img=Image.new('RGBA',(w,h));img.putdata([(*colors[p],255) if p else (0,0,0,0) for p in pixels])
    return img,orphan, {'min_x':xmin,'min_y':ymin,'max_x_exclusive':xmax,'max_y_exclusive':ymax,'anchor_relative':'ROM signed dx/dy (no invented 24x32 crop)'}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('rom',type=Path);ap.add_argument('--output',type=Path,default=Path('research/simon-assets/extended'))
    args=ap.parse_args();rom=load_rom(args.rom);args.output.mkdir(parents=True,exist_ok=True)
    palette=decode_room(rom,1,0)[0]['palette_rgb333'];colors=[tuple(round(v*255/7) for v in rgb) for rgb in palette]
    records=[];previews=[]
    for index in FAILED_INDICES:
        parts=[components(rom,index,part) for part in ('lower','upper')]
        img,orphan,bounds=render(parts,colors)
        name=f'extended-{index:02d}.png';img.save(args.output/name)
        rec={'index':index,'status':'positioned_layers_complete' if not orphan else 'partial_positions',
             'image':name,'canvas':{'width':img.width,'height':img.height},'bounds':bounds,
             'layers':[{k:v for k,v in p.items() if k!='layers'} for p in parts],
             'unpositioned_layers':len(orphan),'role':'unidentified'}
        for part,k,mask in orphan:
            overlay=Image.new('RGBA',(16,16));overlay.putdata([(*colors[1 if k%2==0 else 2],255) if b else (0,0,0,0) for b in mask]);
            orphanname=f'extended-{index:02d}-{part}-unpositioned-{k:02d}.png';overlay.save(args.output/orphanname)
            rec.setdefault('unpositioned_images',[]).append(orphanname)
        records.append(rec);previews.append((index,img,rec['status']))
    cw,ch=175,170;cols=4;sheet=Image.new('RGB',(cw*cols,ch*((len(previews)+cols-1)//cols)),(33,37,48));d=ImageDraw.Draw(sheet)
    for j,(idx,img,status) in enumerate(previews):
        x=(j%cols)*cw;y=(j//cols)*ch
        scale=min(4, max(1,min(145//img.width,130//img.height)))
        big=img.resize((img.width*scale,img.height*scale),Image.Resampling.NEAREST)
        sheet.paste(big,(x+(cw-big.width)//2,y+5),big)
        d.text((x+8,y+141),f'Entry {idx:02d}  {img.width}x{img.height}',fill='white')
        if status!='positioned_layers_complete':d.text((x+8,y+155),'PARTIAL / layers missing positions',fill='#FFBB77')
    sheet.save(args.output/'extended-sheet.png')
    manifest={'rom_sha256':hashlib.sha256(rom).hexdigest(),'indices':list(FAILED_INDICES),'entries':records,
              'caution':'Decoding image layers does not establish frame purpose; layers are colored using alternating mask planes. Entry 12 lacks positions for some bitmap layers.'}
    (args.output/'extended-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Decoded',len(records),'entries;',sum(r['status']=='positioned_layers_complete' for r in records),'complete compositions;',sum(r['status']=='partial_positions' for r in records),'partial compositions')
if __name__=='__main__':main()
