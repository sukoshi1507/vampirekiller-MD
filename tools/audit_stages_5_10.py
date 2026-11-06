import re,json,pathlib,sys
root=pathlib.Path(__file__).resolve().parents[1] / 'sgdk' / 'src' / 'generated'
results=[]
for stage in range(5,11):
 s=(root/f'stage{stage}_maps.c').read_text(); h=(root/f'stage{stage}_maps.h').read_text()
 n=int(re.search(rf'VK_STAGE{stage}_ROOM_COUNT\s+(\d+)',h).group(1))
 body=s.split(f'vk_stage{stage}_collision[',1)[1].split('= {',1)[1].split('};',1)[0]
 rooms=re.findall(r'\{([^{}]+)\}',body)
 maps=[[int(v) for v in re.findall(r'\b\d+\b',r)] for r in rooms]
 linkbody=s.split(f'vk_stage{stage}_links[',1)[1].split('= {',1)[1].split('};',1)[0]
 links=[]
 for r in re.findall(r'\{([^{}]+)\}',linkbody):
  vals=[x.strip() for x in r.split(',')];links.append([255 if x=='VK_ROOM_NONE' else int(x) for x in vals])
 invalid=[]; reciprocal=[]
 for i,l in enumerate(links):
  for k,v in enumerate(l[:4]):
   if v!=255 and not (0<=v<n): invalid.append([i,k,v])
   if v!=255 and 0<=v<len(links) and links[v][[1,0,3,2][k]]!=i: reciprocal.append([i,k,v])
 results.append(dict(stage=stage,rooms_expected=n,rooms_found=len(maps),map_lengths=sorted(set(map(len,maps))),link_count=len(links),invalid_targets=invalid,nonreciprocal=reciprocal,vertical_links=sum(sum(v!=255 for v in l[:2]) for l in links),horizontal_links=sum(sum(v!=255 for v in l[2:4]) for l in links),verified_collision_flags=[l[4] for l in links],tile_id_range=[min(map(min,maps)),max(map(max,maps))]))
print(json.dumps(results,indent=2))
