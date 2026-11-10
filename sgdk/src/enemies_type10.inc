/* Type 10h: ADDE/ADF8. S8Ext holds +6, +9/A, +10, +11.
 * Type-9 returning projectile: A07B/9EC5; c18 is its source slot.
 * No change to EnemyState's public layout. */
static void type10Spawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_10);
    S8Ext *q;
    if (!e) return;
    q = ext(s, e);
    e->x = x; e->y = y; e->pose = 0x5F; e->extraPhase = 8;
    e->vy = 0; q->vx = 0; q->mobile = 1; q->c10 = 0; q->c11 = 0x3C;
}

static void type10ShotSpawn(EnemyState *s, const Enemy *src, const EnemyRoomVars *v)
{
    EnemyU8 i, slot;
    Enemy *e;
    S8Ext *q;
    for (slot = 0; slot < 8; slot++) {
        for (i = 0; i < ENEMY_ALL; i++)
            if (Enemies_slot(s,i)->active && isProjectile(Enemies_slot(s,i)->type) && ext(s, Enemies_slot(s,i))->c10 == slot) break;
        if (i == ENEMY_ALL) break;
    }
    if (slot == 8) return;
    e = newObject(s, ENEMY_TYPE_10_SHOT);
    if (!e) return;
    q = ext(s, e);
    q->c10 = slot; q->c12 = s8BoneC[slot]; q->c18 = (EnemyU8)(src - s->enemy);
    q->c11 = 0x18; q->mobile = 1;
    q->vx = v->simonX >= src->x ? 0x400 : -0x400;
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
    e->state = (EnemyU8)(q->vx >> 8); /* A07B: initial sign, retained during return */
    e->x = src->x; e->y = (EnemyU8)(src->y - 0x10); e->vy = 0; e->pose = 3;
}

static void type10Step(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *q = ext(s, e);
    int distance;
    if (!(e->state & 1)) {
        if (!floorAt(v, e->x, e->y)) addVy(e, 0xA0);
        else {
            e->y &= 0xF0; e->state++;
            e->pose = v->simonX >= e->x ? 0x61 : 0x5F;
            setVx(q, e, v->simonX >= e->x ? 0x140 : -0x140, v, 1);
            e->vy = 0;
        }
    } else {
        const int left = q->vx < 0;
        int wall = 0, k;
        e->pose = (EnemyU8)((v->simonX >= e->x ? 0x61 : 0x5F) + ((s->tick & 8) != 0));
        distance = v->simonX >= e->x ? v->simonX - e->x : e->x - v->simonX;
        for (k = 0; k < 4; k++)
            if (s4Solid(v, (EnemyU8)(e->x + (left ? -8 : 8)), (EnemyU8)(e->y - 8 - 8*k))) wall = 1;
        if (!floorAt(v, (EnemyU8)(e->x + (left ? -8 : 16)), e->y) || wall) {
            q->mobile = 0;
            if (distance >= 0x3E && distance < 0x4C) {
                q->mobile = 1; setVx(q, e, -q->vx, v, 0);
                goto moved;
            }
            q->c10 = (EnemyU8)(distance < 0x3E);
        } else {
            q->mobile = 1;
            if (distance < 0x2E) {
                setVx(q, e, v->simonX >= e->x ? -0x140 : 0x140, v, 1); q->c10 = 1;
            } else if (distance >= 0x5C) {
                setVx(q, e, v->simonX >= e->x ? 0x140 : -0x140, v, 1); q->c10 = 0;
            }
            if ((e->x >= 0xF0 && q->vx >= 0) || (e->x < 0x0F && q->vx < 0)) q->mobile = 0;
        }
        if (e->state & 2) {
            if (--q->c11 == 0) { q->c11 = 0x3C; e->state &= (EnemyU8)~2; }
        } else if ((distance < 0x2A && (q->c10 & 1)) || (distance >= 0x60 && !(q->c10 & 1))) {
            e->state |= 2; type10ShotSpawn(s, e, v);
        }
    }
moved:
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

/* Called in a separate D700 pass after all source enemies, as in 9E31. */
static void type10ShotStep(EnemyState *s, Enemy *e)
{
    S8Ext *q = ext(s, e);
    const Enemy *src = &s->enemy[q->c18];
    q->c12++; s8BoneC[q->c10] = q->c12;
    e->pose = (EnemyU8)(0x63 + ((q->c12 >> 2) & 3));
    q->vx = (EnemyS16)(q->vx + ((e->state & 0x80) ? 0x18 : -0x18));
    if (q->c11) q->c11--;
    else if (src->active && src->type == ENEMY_TYPE_10 && (EnemyU8)(src->x - e->x + 0x10) < 0x20) e->active = 0;
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}
