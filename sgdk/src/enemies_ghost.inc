/*
 * Object 1A / 1B / 1C released by a candle of object 15 (8AD0): routine B4EA (init B4D2), shared by the types 19 to 1C. The type
 * comes from the table 8AF0 [(D002 - 1) & 7] = 1A 1C 1B 1B 1B 1B 1C 1C, D002 being the object group of the stage (5E6A: 0 for
 * the stages 0 to 3, 1 for 4 to 6, 2 for 7 to 9, 3 for 10). The groups 1 (1A, stages 4 to 6) and 2 (1C, stages 7 to 9) are traced and
 * ported (same routine and same image, another colour); the group 3 (1B, stage 10) is not: no candle or block of object 15 exists in the stage 10 (research/stage10-level.md).
 * Included by enemies.c after enemies_stage7.inc; it uses the statics of that file (S8Ext, setVx, addVy, moveObj, s4Solid).
 * research/stage9-candle-object.md.
 *
 * State in the shared fields: Y and X with their fractions in y, sub, x, timer; vy in vy; vx in s8x.vx; mobile (+6) in
 * s8x.mobile; the +C counter in s8x.c12; the wait counter +10h in s8x.c10; HP (+D) and the hit flag in extraPhase. The
 * pose (+B) is the cell 9B..A2 of the room (table B647 + B665..): odd = first pose, even = second. +E (contact and whip)
 * is 0 in the state 0 and 7 afterwards: ghostInert().
 */

/* B63F + 2 x D000 -> nibble tables (B665, B668, B66B, B66E, B673, B677, B67C, B681): the pose base of each room, 0 = none.
 * Indexes 0 to 4 of 00 9B 9D 9F A1 (B642). Stages 4 to 9 (B665, B668, B66B: 6 rooms each for the stages 4 to 6). */
static EnemyU8 ghostBase(EnemyU8 stage, EnemyU8 room)
{
    static const EnemyU8 st4[6] = { 0x9F, 0x9D, 0x9F, 0x9D, 0x9F, 0x9F };
    static const EnemyU8 st5[6] = { 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F };
    static const EnemyU8 st6[6] = { 0x9B, 0x9B, 0x9B, 0x9F, 0x9F, 0x9B };
    static const EnemyU8 st7[10] = { 0xA1, 0xA1, 0x9B, 0x9B, 0x00, 0x9B, 0x9B, 0x9F, 0x9F, 0x00 };
    static const EnemyU8 st8[8] = { 0x9F, 0x9F, 0x00, 0x9B, 0x9B, 0x9F, 0x9F, 0x9B };
    static const EnemyU8 st11[6] = { 0x9B, 0xA1, 0x00, 0x00, 0x00, 0xA1 };
    static const EnemyU8 st9[10] = { 0x9F, 0x9F, 0x9F, 0x9F, 0x00, 0xA1, 0x9B, 0x9B, 0x00, 0x00 };
    if (stage == 4 && room < 6) return st4[room];
    if (stage == 5 && room < 6) return st5[room];
    if (stage == 6 && room < 6) return st6[room];
    if (stage == 7 && room < 10) return st7[room];
    if (stage == 8 && room < 8) return st8[room];
    if (stage == 9 && room < 10) return st9[room];
    if (stage == 11 && room < 6) return st11[room];          /* B681 : bytes 14 00 04 -> 9B A1 - - - A1 */
    return 0;
}

/* First cell of the object in the tile block (odd pose; the even pose is 4 further): B4 in the stages 7 to 9, 70 in the stages 4 and 5
 * (free there, after the moving platform at 60), 80 in the stage 6 (70 to 7B are taken). The tiles are loaded with the room (actors.c). */
EnemyU8 Enemies_ghostCell(EnemyU8 stage)
{
    return stage == 11 ? 0xA0 : stage == 6 ? 0x80 : (stage == 4 || stage == 5) ? 0x70 : 0xB4;
}

static EnemyU8 ghostCell = 0xB4;   /* of the stage of the last frame or candle */

/* The pose base of a room (9B, 9D, 9F, A1, 0 none), for the tiles loaded with the room (actors.c). */
EnemyU8 Enemies_ghostBase(EnemyU8 stage, EnemyU8 room)
{
    return ghostBase(stage, room);
}

/* B611: +C + 1, the pose is the base, + 1 while bit 4 of +C is set. */
static void ghostPose(Enemy *e, S8Ext *q, EnemyU8 stage, EnemyU8 room)
{
    const EnemyU8 base = ghostBase(stage, room);
    q->c12++;
    e->pose = (EnemyU8)(base + ((q->c12 & 0x10) ? 1 : 0));
}

/* +E = 0 in the state 0 (B4DC): no contact (7E8C) and no whip (7DAD, 8033). B505 sets it to 7 when it starts to fall. */
static int ghostInert(const Enemy *e)
{
    return e->type == ENEMY_TYPE_GHOST && e->state == 0;
}

/* B600: vx = 1C0h towards Simon (C427 >= X), else FE40h. */
static void ghostAim(Enemy *e, S8Ext *q, const EnemyRoomVars *v)
{
    setVx(q, e, v->simonX >= e->x ? 0x1C0 : -0x1C0, v, 0);
}

/* B5E0: carry when the object is not rising and the tile at (X, Y) is solid (7BA3). */
static int ghostFloor(const EnemyRoomVars *v, const Enemy *e)
{
    return e->vy >= 0 && s4Solid(v, e->x, e->y);
}

/* B5AE: the wall 16 px ahead (7BB7 to the right, 7C13 to the left), two points Y - 4 and Y - 12: fleaWall. */
static void ghostWall(const EnemyRoomVars *v, Enemy *e, S8Ext *q)
{
    if (fleaWall(v, e, q->vx < 0)) setVx(q, e, 0, v, 0);
}

/* B54B: stops, state 2 for 30 passes. */
static void ghostRest(Enemy *e, S8Ext *q)
{
    e->vy = 0;
    q->vx = 0;
    e->y = (EnemyU8)(e->y & 0xF8);
    q->mobile = 0;
    q->c10 = 0x1E;
    e->state = 2;
}

static void ghostStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *const q = ext(s, e);
    ghostPose(e, q, v->stage, v->room);                                                  /* B4EA : B611 */
    switch (e->state) {
    case 0:                                                              /* B4FB */
        if (--q->c10 != 0) break;
        e->vy = 0x200;
        e->state = 1;
        break;
    case 1:                                                              /* B50D : B569 */
        ghostWall(v, e, q);
        if (ghostFloor(v, e)) {                                          /* B57F */
            e->y = (EnemyU8)(e->y & 0xF8);
            ghostRest(e, q);                                             /* B54B */
        } else {
            addVy(e, 0x40);
        }
        break;
    case 2:                                                              /* B513 */
        q->mobile = 0;
        if (--q->c10 != 0) break;
        ghostAim(e, q, v);                                               /* B51B */
        q->c10 = 0x1E;
        e->state = 3;
        break;
    default:                                                             /* B526 */
        q->mobile = 1;
        ghostPose(e, q, v->stage, v->room);
        {   /* B5C1 : Simon above the object and no floor 8 px ahead : jump (B58E) */
            const int simonAbove = (EnemyU8)(v->simonY - 0x10) < e->y;
            if (simonAbove && !s4Solid(v, (EnemyU8)(e->x + (q->vx < 0 ? -8 : 8)), e->y)) {
                e->state = 1;
                ghostAim(e, q, v);
                e->vy = (EnemyS16)0xFD00;
                break;
            }
        }
        if (!ghostFloor(v, e)) {                                         /* B5EF : falls straight down */
            q->vx = 0;
            e->vy = 0x200;
            e->state = 1;
            break;
        }
        ghostWall(v, e, q);                                              /* B539 */
        if (q->vx < 0 ? e->x < 0x10 : e->x >= 0xE8) { ghostRest(e, q); break; }   /* B59E */
        if (--q->c10 == 0) ghostRest(e, q);                              /* B547 */
        break;
    }
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

/* 8AD0: the candle of object 15 has burnt 16 passes; the object is made at (X, Y + 16) of the candle, the burst goes (8B1B).
 * B4D2: pose, mobile, no speed, +E = 0, 8 passes of wait. */
void Enemies_candleObject(EnemyState *s, EnemyU8 stage, EnemyU8 room, EnemyU8 x, EnemyU8 y)
{
    Enemy *e;
    S8Ext *q;
    if (stage < 4 || stage > 11 || stage == 10) return; /* D002 = 1 : object 1A, D002 = 2 : object 1C, D002 = 3 : object 1B (stage 11, 4 candles of object 15 ; none in the stage 10) */
    ghostCell = Enemies_ghostCell(stage);
    e = newObject(s, ENEMY_TYPE_GHOST);
    if (!e) return;
    q = ext(s, e);
    e->x = x;
    e->y = (EnemyU8)(y + 0x10);
    e->extraPhase = 1;                                   /* +D, table 60E1 */
    e->state = 0;
    e->vy = 0;
    q->vx = 0;
    q->mobile = 1;
    q->c12 = 0;
    q->c10 = 8;
    e->dx = 1;
    ghostPose(e, q, stage, room);
}
