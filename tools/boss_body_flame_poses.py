import sys,re
f=sys.argv[1];x=int(sys.argv[2])   # SAT x of the flame
runs=[]
for l in open(f):
    p=l.rstrip('\n').split('|')
    if len(p)<6 or not p[0].isdigit():continue
    fr=int(p[0])
    pose=None
    for m in re.finditer(r'\d+:(\d+):(\d+):(F[48]):08/',p[5]):
        if int(m.group(2))==x and int(m.group(1))<224:
            pose=(m.group(3),int(m.group(1)))
    if pose:
        if runs and runs[-1][0]==pose and runs[-1][2]==fr-1:
            runs[-1][2]=fr;runs[-1][3]+=1
        else:runs.append([pose,fr,fr,1])
for r in runs:print(r)
