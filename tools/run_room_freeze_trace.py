#!/usr/bin/env python3
"""openMSX measure of the room change freeze (state 9) for every arrival room.

Usage: run_room_freeze_trace.py OUTDIR [FIRST_STAGE LAST_STAGE [REPEATS [reverse]]]
Runs openMSX (C-BIOS_MSX2) with tools/vertical_links_trace.tcl and
tools/room_freeze_trace_extra.tcl, then prints one JSON row per state 9 episode:
stage, room, frames (VBLANK samples with C000 = 9) and dark (samples from the first
one to the display off, 47D4), the two numbers of research/room-links.md section 9.2.
The merged result of the runs is research/room-freeze-rooms.json.
"""
import csv,json,os,shutil,subprocess,sys,tempfile
from pathlib import Path
from probe import load_rom,reference_rom
from decode_room_links import decode

TOOLS=Path(__file__).resolve().parent

def plan(rom,first,last,repeats,reverse=False):
    lines=[]
    for stage in range(first,last+1):
        count=3 if stage==0 else len(decode(rom)[stage]['rooms'])
        if stage:lines+=[f'stage {stage}','wait 400']
        for _ in range(repeats):
            order=list(range(1,count))+[0]
            # Simon may fall and die during a run: the other order moves the lost change.
            for room in (order[::-1] if reverse else order):lines+=[f'trans {room}','wait 34']
    return '\n'.join(lines)

def analyse(out):
    rows=list(csv.DictReader((out/'frames.csv').open()))
    off={}
    for e in csv.DictReader((out/'rf_events.csv').open()):
        if e['what']=='off':off.setdefault(int(e['frame']),True)
    result=[];i=0
    while i<len(rows):
        if rows[i]['c000']!='9':i+=1;continue
        j=i
        while j<len(rows) and rows[j]['c000']=='9':j+=1
        if j==len(rows):break
        start=int(rows[i]['frame']);dark=[f for f in off if start-1<=f<start+j-i]
        # The arrival room is in D001 once B987 wrote it: read at the end of the episode.
        result.append(dict(stage=int(rows[j-1]['d000']),room=int(rows[j-1]['d001']),frames=j-i,
                           dark=min(dark)-start if dark else None,frame=start,forced=rows[i-1]['c41b']=='255'))
        i=j
    return result

def main():
    out=Path(sys.argv[1]).resolve();first,last=(int(sys.argv[2]),int(sys.argv[3])) if len(sys.argv)>3 else (0,18)
    repeats=int(sys.argv[4]) if len(sys.argv)>4 else 2
    rom=load_rom(reference_rom());out.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='vk-rf-') as tmp:
        tmp=Path(tmp);(tmp/'game.rom').write_bytes(rom)
        script='set vk_out {%s}\nset vk_plan {%s}\n'%(out,plan(rom,first,last,repeats,'reverse' in sys.argv[5:]))
        script+=(TOOLS/'vertical_links_trace.tcl').read_text()+'\n'+(TOOLS/'room_freeze_trace_extra.tcl').read_text()
        (tmp/'trace.tcl').write_text(script)
        env=dict(os.environ,OPENMSX_USER_DATA=str(tmp/'user'),SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
        with (out/'openmsx.log').open('w') as log:
            subprocess.run(['/usr/bin/openmsx','-control','stdio','-machine','C-BIOS_MSX2','-cart',str(tmp/'game.rom'),
                            '-script',str(tmp/'trace.tcl')],stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,env=env,timeout=1800)
    assert (out/'done.txt').is_file(),'openMSX did not finish'
    rows=analyse(out);(out/'episodes.json').write_text(json.dumps(rows,indent=1)+'\n')
    for r in rows:print(json.dumps(r))
if __name__=='__main__':main()
