/* AF4A/AF5F, fixed enemy 09h. q: +11 delay, +12 direction, +0C animation.
 * The dormant object has +0E=0 and only its first two SAT layers visible. */
static void type09Spawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_09);
    S8Ext *q;
    if (!e) return;
    q = ext(s,e); e->x=x; e->y=y; e->vy=0; e->extraPhase=2;
    q->mobile=0; q->c11=0x30;
}
static void type09Face(Enemy *e, S8Ext *q, const EnemyRoomVars *v)
{
    q->c18 = (EnemyU8)(v->simonX < e->x);
    e->pose = q->c18 ? 0x23 : 0x21;
    setVx(q,e,q->c18 ? -0x220 : 0x220,v,1);
}
static void type09Step(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *q=ext(s,e);
    int k,reverse=0;
    if (e->state==0) {
        if (--q->c11) e->pose=q->c11 < 12 ? 0x25 : 0x26;
        else {type09Face(e,q,v);q->c12=0x10;e->state=1;q->mobile=1;}
    } else if (e->state==1) {
        if (!floorAt(v,(EnemyU8)(e->x+(q->vx<0 ? -16:16)),e->y)) reverse=1;
        else if ((!q->c18 && e->x>=0xF0) || (q->c18 && e->x<0x10)) reverse=1;
        else for(k=0;k<4;k++)
            if(s4Solid(v,(EnemyU8)(e->x+(q->c18 ? -8:8)),(EnemyU8)(e->y-8-k*8))) reverse=1;
        if(reverse) {q->c18^=1;setVx(q,e,-q->vx,v,1);}
        e->pose=(EnemyU8)(0x21+((q->c12&4)!=0)+(q->c18 ? 0:2));
        if(--q->c12==0) {q->mobile=0;q->c12=4;e->state=2;}
    } else if (--q->c12==0) {type09Face(e,q,v);q->c12=0x30;q->mobile=1;e->state=1;}
    moveObj(e,q);
    if(s4Remove(e))e->active=0;
}
