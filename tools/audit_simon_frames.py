#!/usr/bin/env python3
"""Catalog ROM Simon frame pairs with the layout decoded by extract_player.
A pair decoding successfully is NOT evidence of its animation meaning.
"""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw
from extract_player import extract_pose
from probe import load_rom

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('rom', type=Path)
    parser.add_argument('--output', type=Path, default=Path('research/simon-frame-audit'))
    args = parser.parse_args()
    data = load_rom(args.rom)
    args.output.mkdir(parents=True, exist_ok=True)
    records=[]
    frames=[]
    for frame in range(24):
        try:
            pixels, sources=extract_pose(data,frame,frame)
        except (ValueError, IndexError) as ex:
            records.append({'frame':frame, 'status':'unsupported_layout', 'detail':str(ex)})
            continue
        records.append({'frame':frame,'status':'decoded','source':sources,
                        'opaque_pixels':sum(bool(x) for x in pixels)})
        frames.append((frame,pixels))
    sheet=Image.new('RGB',(6*112, ((len(frames)+5)//6)*144),(30,32,41))
    draw=ImageDraw.Draw(sheet)
    for index,(frame,pixels) in enumerate(frames):
        im=Image.new('RGB',(24,32))
        colors=[(30,32,41),(255,170,120),(135,200,255),(255,255,255)]
        im.putdata([colors[x] for x in pixels])
        x=(index%6)*112+20;y=(index//6)*144+8
        sheet.paste(im.resize((72,96), Image.Resampling.NEAREST),(x,y))
        draw.text((x,y+103),f'frame {frame:02}', fill='white')
    sheet.save(args.output/'candidate-frames.png')
    (args.output/'catalog.json').write_text(json.dumps({'warning':'Valid masks are NOT identified crouch, stair or whip animations. Frames require Z80 state-table reconstruction.', 'frames':records},indent=2)+'\n')
    print(f'{len(frames)} candidate mask pairs decoded; {24-len(frames)} need other layouts')

if __name__=='__main__': main()
