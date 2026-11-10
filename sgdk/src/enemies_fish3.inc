/*
 * Objects 03 and 21h of the stage 10 : the fishman that spits, routine A310 (the same routine as the object 02 of the stage 2, whose port is
 * fishStep) with the type bit 0 set. Init A2E0 (object 03, made by the channel 9D52 of the room flag 04 in the rooms 1 to 5) or A2C7 (object 21h,
 * the fixed list of the rooms 6 to 8, dormant on the ground until Simon is at its height; its first pass turns it into an object 03).
 * research/stage10-enemies.md.
 *
 * State 0 (A31E): jump out of the water or fall, vy + 80h a pass, ground at (X or X + 8, Y) : walk (state 1, +10 = 30h - 4 D012, +11 = 10h) ;
 * Y >= C0h without ground : dives (sound 09, two drops). State 1 (A3A0, A3DE): walks towards Simon (vx +-180h, +15 turn timer of the table
 * A4F3 or A4F7, +13 pose timer 8), stops at an edge (state 0, +18 = 1) ; +10 passes later state 2 (A45A) : +11 from 18h, the pose of the
 * shot below 10h, the fireball (D700 type 1 : A08E[2], A492) when +11 = 8, then state 1 again for 30h passes. The shot : (X, Y - 14h), vx +-300h in the
 * direction of the walk, straight on (9E77 does nothing, 99B9 moves it).
 *
 * State in the shared fields: Y and X with their fractions in y, sub, x, timer ; vy in vy ; vx, mobile (+6), +10, +11, +12 (facing), +18 in S8Ext ;
 * +13, +14, +15, +19, +1B, +1C, +1D in Fish3Ext ; points of life (+D) and the hit flag in extraPhase ; the pose (+B) in pose.
 * Included by enemies.c after fishSpawn : it uses splashSpawn, moveObj, addVy, floorAt, s4Remove, fireballStep.
 */

typedef struct { EnemyU8 c13, c14, c15, c19, c1b, c1c, c1d; } Fish3Ext;
static Fish3Ext f3[ENEMY_MAX];

static Fish3Ext *fx(const EnemyState *s, const Enemy *e) { return &f3[e - s->enemy]; }

static void fish3Reset(void)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++)
        f3[i].c13 = f3[i].c14 = f3[i].c15 = f3[i].c19 = f3[i].c1b = f3[i].c1c = f3[i].c1d = 0;
}

/* La ROM émet A4E8 avec A=08 à A2F9, A=07 à A34D, A=09 à A386.
 * Enregistrer les appels, sans produire un son depuis la logique pure. Le pilote
 * effectue lui-même l'arbitrage de priorité (509F, C096). */
static void fish3Sound(EnemyState *s, EnemyU8 id)
{
    if (s->fishSoundCount < ENEMY_MAX)
        s->fishSounds[s->fishSoundCount++] = id;
}

#define FISH3_HP 2                 /* 60E1[3] */
#define FISH3_FIRE_SPEED 3         /* A4A1 : vx 300h */

/* A4B3 : facing Simon, pose 12h (right) if C427 >= X, else 0Fh. */
static void fish3Face(Enemy *e, EnemyU8 simonX)
{
    e->pose = (EnemyU8)(simonX >= (EnemyU8)e->x ? 0x12 : 0x0F);
}

/* A492 -> 9F6D : the fireball (D700 type 1, A08E[2]) in (X, Y - 14h), in the direction of vx ; moved in the pass of its creation (the D700 loop follows
 * the C800 loop), like the ball of the statue. */
static void fish3Fire(EnemyState *s, const Enemy *fish, const S8Ext *q)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FIREBALL);
    if (!e) return;
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
    e->anim = FISH3_FIRE_SPEED;
    e->x = fish->x;
    e->y = (EnemyU8)(fish->y - 0x14);
    /*9E31 owns the first movement after the complete C800 traversal. */
    e->age = 0;
}

/* A2E0 : an object 03 out of the water (5F1D then the init) : at (X, C8h), facing Simon, vy F480h, two drops. */
static void fish3Spawn(EnemyState *s, EnemyU8 x, const EnemyRoomVars *v)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FISH3);
    S8Ext *q;
    Fish3Ext *f;
    if (!e) return;
    q = ext(s, e);
    f = fx(s, e);
    e->x = x;
    e->y = ENEMY_TYPE2_Y;
    e->extraPhase = FISH3_HP;
    e->state = 0;
    e->vy = ENEMY_TYPE2_JUMP_VY;
    fish3Face(e, v->simonX);
    q->c18 = 0;
    f->c1b = 0;
    q->mobile = 1;
    f->c19 = 1;
    splashSpawn(s, x, ENEMY_TYPE2_Y);
    splashSpawn(s, x, ENEMY_TYPE2_Y);
    fish3Sound(s, 0x08); /* A2F9 : saut hors de l’eau */
}

/* A2C7 : an object 21h of the fixed list : state 1, not mobile, +1B = 1, +10 = 2, +11 = 10h, +13 = 8, +15 = 1 (A366). The pose is set by A3AC. */
static void fish3SpawnFixed(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FISH3);
    S8Ext *q;
    Fish3Ext *f;
    if (!e) return;
    q = ext(s, e);
    f = fx(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = FISH3_HP;
    e->state = 1;
    e->vy = 0;
    e->pose = 0x12;
    f->c1b = 1;
    f->c1c = 0;
    q->c12 = 0;
    q->c10 = 2;
    f->c19 = 0;
    f->c15 = 1;
    q->c11 = 0x10;
    f->c14 = 0;
    f->c13 = 8;
    q->mobile = 0;
}

/* A31E : state 0, the jump out of the water or the fall. */
static void fish3Air(EnemyState *s, Enemy *e, S8Ext *q, Fish3Ext *f, const EnemyRoomVars *v)
{
    if (!(q->c18 & 1)) fish3Face(e, v->simonX);                         /* A324 */
    q->vx = 0;                                                           /* A32A */
    addVy(e, ENEMY_TYPE2_GRAVITY);                                       /* A330 */
    if (e->vy < 0) return;                                               /* A333 : no test while rising */
    if (floorAt(v, (EnemyU8)(e->x + ((q->c12 & 1) ? 8 : 0)), e->y)) {    /* A338 : 7B98 */
        e->y = (EnemyU8)(e->y & 0xF8);                                   /* A352 : the fraction of Y is kept */
        q->c10 = (EnemyU8)(0x30 - 4 * v->difficulty);                    /* A35A */
        q->c11 = 0x10;
        f->c14 = 0;
        f->c15 = 1;
        e->state = 1;
        f->c13 = 8;
        e->vy = 0;
        fish3Sound(s, 0x07); /* A34D : atterrissage */
        return;
    }
    if (e->y >= ENEMY_TYPE2_WATER) {                                     /* A380 : dives, two drops */
        const EnemyU8 x = (EnemyU8)e->x, y = e->y;
        e->active = 0;
        splashSpawn(s, x, y);
        splashSpawn(s, x, y);
        fish3Sound(s, 0x09); /* A386 : plongeon */
    }
}

/* A3CB : state 1, the walk. */
static void fish3Walk(Enemy *e, S8Ext *q, Fish3Ext *f, const EnemyRoomVars *v)
{
    static const EnemyU8 turnA[4] = { 0x10, 0x20, 0x18, 0x30 };         /* A4F3 : +19 bit 0 */
    static const EnemyU8 turnB[4] = { 0x08, 0x10, 0x0B, 0x06 };         /* A4F7 */
    static const EnemyU8 poseL[4] = { 0x08, 0x09, 0x0F, 0x10 };         /* A452 */
    static const EnemyU8 poseR[4] = { 0x0B, 0x0C, 0x12, 0x13 };         /* A456 */
    int dx;
    if (--q->c10 == 0) {                                                 /* A3D1 */
        e->state = 2;
        q->c11 = 0x18;
        return;
    }
    if (--f->c15 == 0) {                                                 /* A3DE : A4CF, the turn */
        f->c15 = ((f->c19 & 1) ? turnA : turnB)[f->c1d & 3];
        q->c12 = 0;
        dx = -0x180;
        if (v->simonX >= (EnemyU8)e->x) { q->c12 = 1; dx = 0x180; }      /* A3ED */
        q->vx = (EnemyS16)dx;
        e->dx = (EnemyS8)(dx < 0 ? -1 : 1);
        f->c13 = 1;
    }
    if (--f->c13 == 0) {                                                 /* A403 */
        f->c13 = 8;
        f->c14++;
        e->pose = ((q->c12 & 1) ? poseR : poseL)[(f->c14 & 1) + 2];
    }
    if (!floorAt(v, (EnemyU8)(e->x + (q->vx < 0 ? 0 : 8)), e->y)) {       /* A42F : an edge, the fall */
        e->vy = 0;
        e->state = 0;
        q->c18 = 1;
    }
}

/* A45A : state 2, the stop and the shot. */
static void fish3Shoot(EnemyState *s, Enemy *e, S8Ext *q)
{
    static const EnemyU8 pose[4] = { 0x0F, 0x12, 0x11, 0x14 };         /* A4AF */
    q->mobile = 0;
    if (--q->c11 == 0) {                                                 /* A486 */
        e->state = 1;
        q->c10 = 0x30;
        q->mobile = 1;
        return;
    }
    if (q->c11 == 8) { fish3Fire(s, e, q); return; }                     /* A492 */
    e->pose = pose[(q->c11 < 0x10 ? 2 : 0) + (q->c12 & 1)];
}

static void fish3Step(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *const q = ext(s, e);
    Fish3Ext *const f = fx(s, e);
    f->c1d++;                                                            /* A310 */
    if (e->state == 1) {                                                 /* A3A0 */
        int go = 1;
        if ((f->c1b & 1) && !(f->c1c & 1)) {                             /* object 21h waiting for Simon at its height */
            const EnemyU8 a = (EnemyU8)(e->y - 8);
            e->pose = 0x12;
            go = a < v->simonY && (EnemyU8)(a + 0x10) >= v->simonY;
            if (go) { q->mobile = 1; f->c1c = 1; }
        }
        if (go) fish3Walk(e, q, f, v);
    } else if (e->state == 2) {
        fish3Shoot(s, e, q);
    } else {
        fish3Air(s, e, q, f, v);
    }
    if (!e->active) return;
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
    moveObj(e, q);                                                       /* 99B9 */
    if (s4Remove(e)) e->active = 0;                                      /* 99E5 */
}

/* Poses 0F, 10 (left), 11 (shot, left), 12, 13 (right), 14 (shot, right) : two cells in a column in SAT (X - 8, Y - 32) and (X - 8, Y - 16), two
 * layers composed in each (colours 2 and 5 with CC). Cells 40 to 68 of the block of the stage 10 (tools/build_stage10_enemies_md.py). */
static int fish3Sprites(const Enemy *e, EnemySprite *out)
{
    static const EnemyU8 top[6] = { 0x40, 0x40, 0x4C, 0x54, 0x54, 0x64 };
    static const EnemyU8 bottom[6] = { 0x44, 0x48, 0x50, 0x58, 0x5C, 0x68 };
    const EnemyU8 k = (EnemyU8)(e->pose < 0x0F || e->pose > 0x14 ? 3 : e->pose - 0x0F);
    out[0].y = (EnemyU8)(e->y - 31);
    out[1].y = (EnemyU8)(e->y - 15);
    out[0].x = out[1].x = (EnemyS16)(e->x - 8);
    out[0].pattern = top[k];
    out[1].pattern = bottom[k];
    out[0].bank = out[1].bank = 1;
    return 2;
}
