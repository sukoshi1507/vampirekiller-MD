/*
 * Stage 11 (D000 = 11), objects 0F (the eagle, routine B1C3, init B193) and 23h (the flea it carries, routine B248 and init B212 like the object 0D) :
 * research/stage11-enemies.md. Included by enemies.c after enemies_fish3.inc ; it uses the statics of enemies.c and of enemies_stage7.inc.
 *
 * The channel 9DE7 (bit 6 of the room flag, rooms 2 to 4) makes an eagle every 25 groups of 4 passes (200 images) at (E0h, 30h or 40h, by the parity
 * of the shot count CF0D), unless Simon is at X >= C0h (then the count CF0C is set to 1). B193 : the init of the flea B212 (vx FD80h, mobile, +10 = 1,
 * pose 67h), then a flea 23h at (X - 3, Y + 12) (X + 4 if X >= 80h) by 5F1D ; if that creation fails the eagle is freed and CF0C = 1.
 * Eagle (B1C3) : the wing pose B1E7 (+11 counts 1..17h : poses 6Dh, 6Eh, 8Dh, 6Eh, table B20F), state 0 : Simon within 38h in X : +C = 8, not mobile
 * (it hovers), state 1 ; state 1 : +C - 1, at 0 mobile again. The flea 23h starts in the state 0 and mobile (B212 sets the state 4 and +6 = 0 only for the
 * object 0D) : it flies on with the eagle until Simon is within 3Ch, then falls (fleaStep).
 * State in the shared fields : Y and X with their fractions in y, sub, x, timer ; vy ; vx, mobile (+6), +C, +11 in S8Ext ; points of life (+D) in extraPhase.
 */

#define EAGLE_HP 8                       /* 60E1[0F] */
#define EAGLE_CHANNEL_PERIOD 0x18        /* 9E0E : eight times 18h */

/* B1E7 : +11 + 1, C = 0 below 7, 1 below 13, 2 below 13h, 1 below 18h (else +11 = 0, C = 1), pose = B20F[C]. */
static void eaglePose(Enemy *e, S8Ext *q)
{
    static const EnemyU8 poses[3] = { 0x6D, 0x6E, 0x8D };
    int c;
    q->c11++;
    if (q->c11 < 7) c = 0;
    else if (q->c11 < 13) c = 1;
    else if (q->c11 < 0x13) c = 2;
    else if (q->c11 < 0x18) c = 1;
    else { q->c11 = 0; c = 1; }
    e->pose = poses[c];
}

static void eagleStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *const q = ext(s, e);
    eaglePose(e, q);
    if (!(e->state & 1)) {                                               /* B1CC */
        const int ad = (v->simonX - e->x) < 0 ? -(v->simonX - e->x) : (v->simonX - e->x);   /* AF40 */
        if (ad < 0x38) {
            q->c12 = 8;
            q->mobile = 0;
            e->state++;
        }
    } else if ((EnemyU8)(--q->c12) == 0) q->mobile = 1;                  /* B1DE */
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

/* B212 for the flea 23h : mobile, vx FD80h, +10 = 1, pose 67h, vy 0, state 0 ; +C = 3Ch - R / 8 (used from the state 2 on, set again by the landing). */
static Enemy *flea23Spawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FLEA);
    S8Ext *q;
    if (!e) return 0;
    q = ext(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = 1;                                      /* 60E1[23h] : +D */
    q->c12 = (EnemyU8)(0x3C - (s7Random() >> 3 & 0x0F));
    e->state = 0;
    q->mobile = 1;
    setVx(q, e, -0x280, 0, 0);
    e->pose = 0x67;
    q->c10 = 1;
    e->vy = 0;
    return e;
}

/* 5F1D with C = 0F then B193, without the first pass (the caller runs it). Returns the eagle, or 0 when the eagle or the flea cannot be made. */
static Enemy *eagleSpawnRaw(EnemyState *s, EnemyU8 x, EnemyU8 y, Enemy **flea)
{
    Enemy *e = newObject(s, ENEMY_TYPE_EAGLE);
    S8Ext *q;
    EnemyU8 fx;
    if (!e) return 0;
    q = ext(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = EAGLE_HP;
    q->c12 = (EnemyU8)(0x3C - (s7Random() >> 3 & 0x0F));   /* B216 */
    e->state = 0;
    q->mobile = 1;
    setVx(q, e, -0x280, 0, 0);
    e->pose = 0x67;
    q->c10 = 1;
    e->vy = 0;
    q->c11 = 0;                                             /* B196 */
    fx = (EnemyU8)(x + (x < 0x80 ? -3 : 4));                /* B1A5-B1AE */
    *flea = flea23Spawn(s, fx, (EnemyU8)(y + 12));
    if (!*flea) {                                           /* B1B6 : CF31 = 0 : the eagle goes (99F6) */
        e->active = 0;
        s->type4Count = 1;                                  /* CF0C = 1 */
        return 0;
    }
    return e;
}

/* 9DE7 -> 9CC3 : one call per pass of the state 5, the counter CF0C of CF10 & 3 = 0 passes, period 18h - 2 D012 + 1. */
static void eagleChannel(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 c;
    Enemy *e, *flea = 0;
    t->phase--;
    if ((t->phase & 0x03) != 0) return;
    if (--s->type4Count != 0) return;
    s->type4Shots++;
    c = (EnemyU8)(v->difficulty << 1);
    s->type4Count = (EnemyU16)(((EAGLE_CHANNEL_PERIOD >= c) ? EAGLE_CHANNEL_PERIOD - c : 0) + 1);
    if (v->simonX >= 0xC0) { s->type4Count = 1; return; }  /* 9DF1-9DFD */
    e = eagleSpawnRaw(s, 0xE0, (EnemyU8)((s->type4Shots & 1) ? 0x30 : 0x40), &flea);
    if (!e) return;
    /* 98E5 runs the channel before the object loop : the eagle and the flea make their first pass in the pass that creates them. */
    eagleStep(s, e, v);
    fleaStep(s, flea, v);
    e->age = flea->age = 1;
}

/* Eagle cells: executed6445 gives head(X-16,Y-23 or-25), wings(X,Y-31/-15).
 * The old openMSX-derived X-14+fraction estimate mixed display/update times.
 * LOT11 checks the direct SAT producer against native placement. */
static int eagleSprites(const Enemy *e, EnemySprite *out)
{
    const int pose = e->pose == 0x6E ? 1 : e->pose == 0x8D ? 2 : 0;
    /* Executed6445: integer X only; fractional X does not change the SAT. */
    out[0].y = (EnemyU8)(e->y - (pose == 2 ? 24 : 22));
    out[0].x = (EnemyS16)(e->x - 16);
    out[0].pattern = 0x40;
    out[1].y = (EnemyU8)(e->y - 30);
    out[1].x = (EnemyS16)(e->x);
    out[1].pattern = (EnemyU8)(0x48 + 0x10 * pose);
    out[2].y = (EnemyU8)(e->y - 14);
    out[2].x = out[1].x;
    out[2].pattern = (EnemyU8)(0x50 + 0x10 * pose);
    out[0].bank = out[1].bank = out[2].bank = 1;
    return 3;
}
