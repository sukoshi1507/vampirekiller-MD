/* BE50/BE70: fixed 12h at stage16, initial idle=1 (stage3 uses 60).
 * S8Ext: c12=+C, c10=+10, c18=+11, c1e=+12, c11=+13, c14=+14. */
static void type12Spawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e=newObject(s,ENEMY_TYPE_12); S8Ext *q;
    if(!e)return;
    q=ext(s,e);e->x=x;e->y=y;e->pose=0x4F;e->extraPhase=16;
    q->mobile=0;q->vx=0;e->vy=0;q->c11=1;
}
static void type12Target(Enemy *e,S8Ext *q,const EnemyRoomVars *v)
{
    EnemyRoomVars target=*v;
    q->c18=v->simonY<0x30 ? 0x30 : (EnemyU8)(v->simonY-0x30);
    q->c1e=(EnemyU8)(v->simonX+((q->c14&1) ? -0x48:0x48));
    target.simonX=q->c1e;target.simonY=q->c18;
    dragonAim((EnemyU8)e->x,e->y,&target,&e->vy,&q->vx);e->state=3;
}
static void type12Step(EnemyState *s,Enemy *e,const EnemyRoomVars *v)
{
    S8Ext *q=ext(s,e);q->c14++;q->c12++;
    e->pose=(q->c12&8)?0x4F:0x4E;
    switch(e->state) {
    case 0:if(--q->c11==0){q->mobile=1;e->state=1;}break;
    case 1:case 2:type12Target(e,q,v);break;
    case 3: {
        int stop=(e->vy<0&&e->y<0x30)||e->y>=0x90||(EnemyU8)(e->x-0x20)>=0xB0;
        if(!stop) {
            if((EnemyU8)(e->y+8-q->c18)>=0x10)break;
            if((EnemyU8)(e->x+8-q->c1e)>=0x10&&(EnemyU8)(e->x-0x20)<0xC0)break;
        }
        q->vx=0;e->vy=0;q->c10=(EnemyU8)(0x40-(s7Random()>>2));e->state=4;break;
    }
    case 4:
        if(q->c10==0x18) {
            EnemyS16 vy,vx;dragonAim((EnemyU8)e->x,e->y,v,&vy,&vx);
            Enemy *shot=dragonFire(s,e,vy,vx,(EnemyU8)e->x,(EnemyU8)(e->y-16));
            if(shot)shot->romBox=2;
        }
        if(--q->c10==0) {
            int dx=v->simonX-e->x;e->vy=0x280;
            q->vx=(EnemyS16)(dx*8);e->state=5;
        }
        break;
    default:
        addVy(e,-16);
        if(e->x<0x19)q->vx=0x200;else if(e->x>=0xE7)q->vx=-0x200;
        if(e->vy<0&&e->y<0x41)type12Target(e,q,v);
        break;
    }
    moveObj(e,q);if(s4Remove(e))e->active=0;
}
static int type12Sprites(const Enemy *e,EnemySprite *out)
{
    static const EnemyS8 offset[2][4][2]={{{-21,-24},{-21,-8},{-21,8},{-5,-8}},{{-21,-16},{-21,0},{-5,-8},{-5,-8}}};
    static const EnemyU8 pat[2][4]={{0x40,0x48,0x58,0x50},{0x60,0x68,0x70,0x70}};
    int i,pose=e->pose==0x4F;
    for(i=0;i<4;i++){out[i].y=(EnemyU8)(e->y+offset[pose][i][0]+1);out[i].x=(EnemyS16)((EnemyU8)(e->x+offset[pose][i][1]));out[i].pattern=pat[pose][i];out[i].bank=1;}
    return 4;
}
