#!/usr/bin/env python3
"""C800 fixed-pose SAT layouts from executed5F1D/6445 (no ROM patches)."""
import argparse,json
from probe import ROOT,load_rom,reference_rom
from verify_item_trace import setup
SPECS=((9,22,(0,*range(0x21,0x27))),(11,11,range(0x47,0x4B)),(13,14,(0x67,0x68,0x6A,0x6B)),(15,18,(0x6D,0x6E,0x8D)),(16,20,range(0x5F,0x63)),(18,23,(0x4E,0x4F)),(27,15,range(0x9B,0xA3)),(36,25,(0,)),(38,26,(1,)))
# Species without a hand-picked row above: (ROM type) -> port types.  Poses come from the openMSX species traces
# (research/late-species-msx.json for stages 12-18, research/legacy-species-msx.json for stages 1-11, both made by
# tools/run_species_trace.py).  Types 01 (A/B) and 04 (internal 0, 1, 2) are left out on purpose: the port keeps their Y in SAT coordinates
# (enemies.h, ENEMY_TYPE4_OBJ_Y), not the object Y that the rows are relative to: they get their own rows below (RAW_Y_SPECIES).  Not listed on purpose: 0E (dynamic chain, verify_c800_sat.py), 11/12/13/14/15/18/19/21/22/24 bosses and
# bodies (their SAT is held for the whole fight, never at Y=E0), 1F (shares B0F8 with 04, poses 0 and 1A only seen once).
LEGACY_INTERNAL={2:(4,),3:(17,),5:(3,),6:(6,),7:(8,),8:(7,),10:(9,),12:(12,),32:(5,),33:(17,),35:(14,)}
# Poses the ROM handlers reach but the openMSX room traces never showed (tools/verify_legacy_poses.py runs the real 5F1D + 98E5/9E31 and
# asserts that each of them is reached): 05 panther facing right 43h..46h, 07 bat 16h/17h, 03 fish 13h/14h.
LEGACY_EXTRA={5:range(0x43,0x47),7:(0x16,0x17),3:(0x13,0x14)}
# ROM types whose port Y (Enemy.y) is the SAT byte of the first entry, not the object Y: 01 (internal A, B; hall zombie, port y = object Y - 32,
# 9D1B E=C0 -> sprite A0) and 04 (internal 2; port y = object Y - 15, ENEMY_TYPE4_OBJ_Y).  Their rows are taken from the real 5F1D + 6445 at every pose
# the ROM handler gives them (98E5/9E31 run for 400 frames from several Simon places), made relative to the first entry, and the ROM must give the
# same layout for every pose (so that the port, whose e->pose stays 0 for these species, can match on pose 0).
RAW_Y_SPECIES=((1,(0,1)),(4,(2,)))
def raw_y_rows(rom):
 rows=[]
 for raw,internals in RAW_Y_SPECIES:
  seen=set()
  for sx in (20,100,200):
   for x,y in ((128,160),(60,160),(200,160),(128,96),(20,176)):
    h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);m[0xC427]=sx;m[0xC425]=y;h.cpu.c=raw;h.cpu.de=(x<<8)|y;h.call(0x5F1D)
    assert m[0xC800]==raw,(raw,'constructor failure')
    for p in range(400):
     m[0xC003]=(p+1)&255;m[0xC440]=255;m[0xC43A]=255;h.cpu.ix=0xC800;h.call(0x6445)
     n=m[0xC820];assert 0<n<=8
     dy=[((m[0xC822+5*i]-m[0xC803]+128)&255)-128 for i in range(n)]
     seen.add((n,sum(bool(m[0xC825+5*i]&15)<<i for i in range(n)),tuple(d-dy[0] for d in dy)))   # the first frame of 04 (pose 0) is 7 lower: the provisional pose the port does not reproduce; the layout is the same
     h.call(0x98E5);h.call(0x9E31)
     if m[0xC800]!=raw:break
  assert len(seen)==1,('poses of ROM type',raw,'do not share one layout',seen)
  n,visible,rel=seen.pop();rel=list(rel)
  for internal in internals:rows.append((raw,internal,0,n,visible,rel))
 return rows
def trace_poses():
 seen={raw:set(p) for raw,p in LEGACY_EXTRA.items()}
 for name in ('late-species-msx.json','legacy-species-msx.json'):
  for room in json.loads((ROOT/'research'/name).read_text())['rooms'].values():
   for raw,poses in room.get('poses',{}).items():seen.setdefault(int(raw),set()).update(poses)
 done={(raw,p) for raw,_,poses in SPECS for p in poses}
 return [(raw,internal,tuple(sorted(p for p in seen.get(raw,()) if (raw,p) not in done))) for raw,internals in sorted(LEGACY_INTERNAL.items()) for internal in internals if seen.get(raw)]
def collect():
 rom=load_rom(reference_rom());rows=raw_y_rows(rom)
 for raw,internal,poses in SPECS+tuple(trace_poses()):
  for pose in poses:
   h=setup(rom,13,3);m=h.cpu.memory;h.call(0x63C5);h.cpu.c=raw;h.cpu.de=0x8060;h.call(0x5F1D)
   assert m[0xC800]==raw,(raw,pose,'constructor failure')
   if raw==9 and 0x21<=pose<0x25:
    m[0xC811]=1;m[0xC427]=200;h.cpu.ix=0xC800;h.call(0xAF5F)
   m[0xC803]=96;m[0xC80B]=pose;h.cpu.ix=0xC800;h.call(0x6445)
   n=m[0xC820];assert 0<n<=8
   dy=[((m[0xC822+5*i]-96+128)&255)-128 for i in range(n)]
   visible=sum(bool(m[0xC825+5*i]&15)<<i for i in range(n))
   rows.append((raw,internal,pose,n,visible,dy))
 # Two ROM types may share a port type (03/21 fish, 0D/23 flea): keep one row per (port type, pose), refusing silent conflicts.
 seen={}
 for r in rows:
  k=(r[1],r[2])
  if k in seen:assert seen[k][3:]==r[3:],('conflicting SAT rows',seen[k],r)
  else:seen[k]=r
 rows=list(seen.values())
 return rows

def generate(rows):
 # Rows grouped by port type (stable: collection order inside a type) with a per-type range, so that the per-frame SAT pass of
 # Enemies_sat_render only walks the rows of the object type instead of the whole table (the mixed pool scenarios sit at the frame budget).
 rows=sorted(rows,key=lambda r:r[1])
 out=['/* Generated by tools/build_c800_sat.py using real5F1D/6445. */','typedef struct {EnemyU8 type,pose,count,visible;EnemyS8 dy[8];} VKC800SATLayout;','typedef struct {EnemyU8 first,count;} VKC800SATRange;','static const VKC800SATLayout c800_sat_layouts[]={']
 for raw,t,p,n,v,dy in rows:out.append('{%d,%d,%d,%d,{%s}}, /* ROM type%02X */'%(t,p,n,v,','.join(map(str,dy)),raw))
 out.append('};')
 top=max(r[1] for r in rows)
 out.append('static const VKC800SATRange c800_sat_ranges[]={')
 for t in range(top+1):
  idx=[i for i,r in enumerate(rows) if r[1]==t]
  out.append('{%d,%d},'%((idx[0],len(idx)) if idx else (0,0)))
 out.append('};')
 assert len(rows)<256
 return '\n'.join(out+[''])
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--check',action='store_true');a=ap.parse_args();rows=collect();data=generate(rows);p=ROOT/'sgdk/src/generated/c800_sat.h'
 if a.check:assert p.read_text()==data,'generated C800 SAT differs'
 else:p.write_text(data)
 print('C800 SAT',len(rows),'executed constructor/pose layouts CHECK PASS' if a.check else 'layouts generated')
if __name__=='__main__':main()
