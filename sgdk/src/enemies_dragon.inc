/*
 * Stage 11, object 0E (the dragon of the room 5 : routine A9B7, init AACD, states A9D8, AA0B, AA37), made by the fixed list (type 0Eh at (208, 112)) :
 * research/stage11-enemies.md. Included by enemies.c after enemies_stage11.inc ; it uses the statics of enemies.c.
 *
 * The object holds the SAT image of its four pieces (two layers each, 8 entries of 5 bytes at +22 : Y, X, pattern, colour) and the 8 targets +10..+17 of the pieces
 * (X first, then Y), moved 2 px a pass by AA67. State 0 (A9D8) : the pieces slide to the X targets (X + D0h, E0h, F0h, 0 : the neck comes out of the wall to the
 * left), +C = 20h passes, then the Y targets of the wave 0 (AA8A), +C = 1Eh, +18 = 20h, +19 = 0. State 1 (AA0B) : closed head (80h / 84h), the wave timer +18 (AA7D :
 * 10h + R & 0Fh) takes a new row of the table AAAD, the pieces move in Y, +C = 18h after 1Eh passes. State 2 (AA37) : open head (70h / 74h), the pieces stay, at +C = 8
 * the shot (A0E5 aim, D700 type 1 by 9F6D at (X of the piece 0, Y + 12)) if the aim goes to the left (vx < 0), at +C = 0 back to the state 1 with +C = 1Eh.
 * The object box follows the piece 0 : Y = Y piece + 10h, X = X piece + 8 (A9C7), after the state routine.
 * Port : x, y = +5, +3 (box), state = +1 ; the rest in DragonExt (one per C800 block, like S8Ext).
 */

#define DRAGON_HP 12                     /* 60E1[0E] */
#define DRAGON_DAMAGE 6                  /* 81CE pair 10h 03, doubled */
#define DRAGON_SCORE 10                  /* 81CE : 10h BCD = 1000 points */

typedef struct { EnemyU8 tgt[8]; EnemyU8 ent[8][5]; EnemyU8 c0c, t18, i19, y1a, x1b; } DragonExt;
static DragonExt dg[ENEMY_MAX];

static DragonExt *dgx(const EnemyState *s, const Enemy *e) { return &dg[e - s->enemy]; }

/* AA67 : each of the 8 values of the column (stride 5) goes 1 px towards its target. */
static void dragonApproach(EnemyU8 *col, const EnemyU8 *tgt)
{
    int i;
    for (i = 0; i < 8; i++) {
        EnemyU8 *c = &col[5 * i];
        if (tgt[i] == *c) continue;
        *c = (EnemyU8)(*c + (tgt[i] >= *c ? 1 : -1));
    }
}

/* AA8A : the targets (pairs) = the row +19 & 7 of the table AAAD + the initial Y, +19 + 1. */
static void dragonWave(DragonExt *d)
{
    static const EnemyU8 rows[8][4] = {
        { 0x00, 0x00, 0x00, 0x00 }, { 0xEC, 0xF2, 0xF8, 0xFE }, { 0x20, 0x18, 0x10, 0x02 }, { 0x10, 0x14, 0x0C, 0x02 },
        { 0xFC, 0xFC, 0x04, 0xFE }, { 0x04, 0x0C, 0x00, 0x02 }, { 0x20, 0x18, 0x10, 0x02 }, { 0x00, 0xF4, 0xFC, 0x00 } };
    const EnemyU8 *row = rows[d->i19 & 7];
    int k;
    d->i19++;
    for (k = 0; k < 4; k++) d->tgt[2 * k] = d->tgt[2 * k + 1] = (EnemyU8)(row[k] + d->y1a);
}

/* A0E5 : the speeds of a shot from (X, Y) of the object to Simon : *vy = HL, *vx = DE (the 8.8 words that 9F6D stores in +7/+8 and +9/+A).
 * Magnitude 80h + 8 D012 ; the angle code from the table A1A2 (|dy| / 4 & 38h, |dx| / 32 & 7), the sine table A1E2. */
static void dragonAim(EnemyU8 ox, EnemyU8 oy, const EnemyRoomVars *v, EnemyS16 *vy, EnemyS16 *vx)
{
    static const EnemyU8 angle[64] = {
        0x20, 0x08, 0x04, 0x03, 0x02, 0x02, 0x01, 0x01, 0x38, 0x20, 0x15, 0x0F, 0x0C, 0x09, 0x08, 0x07, 0x3B, 0x2B, 0x20, 0x19, 0x14, 0x10, 0x0E, 0x0C,
        0x3D, 0x31, 0x27, 0x20, 0x1A, 0x16, 0x13, 0x11, 0x3D, 0x34, 0x2C, 0x25, 0x20, 0x1C, 0x18, 0x15, 0x3E, 0x36, 0x2F, 0x29, 0x24, 0x20, 0x1C, 0x19,
        0x3E, 0x38, 0x32, 0x2C, 0x28, 0x23, 0x20, 0x1D, 0x3E, 0x39, 0x34, 0x2F, 0x2A, 0x26, 0x23, 0x20 };
    static const EnemyU8 sine[64] = {
        0x00, 0x06, 0x0C, 0x12, 0x19, 0x1F, 0x26, 0x2C, 0x32, 0x38, 0x3E, 0x44, 0x4A, 0x50, 0x56, 0x5C, 0x62, 0x68, 0x6D, 0x73, 0x79, 0x7E, 0x84, 0x89,
        0x8E, 0x93, 0x99, 0x9E, 0xA2, 0xA7, 0xAC, 0xB1, 0xB5, 0xB9, 0xBE, 0xC2, 0xC6, 0xCA, 0xCE, 0xD1, 0xD5, 0xD8, 0xDC, 0xDF, 0xE2, 0xE5, 0xE7, 0xEA,
        0xED, 0xEF, 0xF1, 0xF3, 0xF5, 0xF7, 0xF8, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF };
    const EnemyU8 mag = (EnemyU8)(0x80 + 8 * v->difficulty);
    const int dyNeg = v->simonY < oy, dxNeg = v->simonX < ox;
    const EnemyU8 ady = (EnemyU8)(dyNeg ? oy - v->simonY : v->simonY - oy);
    const EnemyU8 adx = (EnemyU8)(dxNeg ? ox - v->simonX : v->simonX - ox);
    const EnemyU8 a = angle[(((ady >> 2) & 0x38) + ((adx >> 5) & 7)) & 63];     /* A134 */
    const int first = ((mag * sine[a]) << 3) >> 8, second = ((mag * sine[0x3F - a]) << 3) >> 8;   /* A184 : H * E, then 3 shifts left, the high bytes */
    *vy = (EnemyS16)(dyNeg ? -first : first);
    *vx = (EnemyS16)(dxNeg ? -second : second);
}

static Enemy *dragonFire(EnemyState *s, const Enemy *src, EnemyS16 vy, EnemyS16 vx, EnemyU8 x, EnemyU8 y);

static void dragonStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    DragonExt *const d = dgx(s, e);
    switch (e->state) {
    case 0:                                                                  /* A9D8 */
        dragonApproach((EnemyU8 *)d->ent + 1, d->tgt);
        dragonApproach((EnemyU8 *)d->ent + 1, d->tgt);
        if (--d->c0c == 0) {
            dragonWave(d);
            e->state = 1;
            d->c0c = 0x1E;
            d->t18 = 0x20;
            d->i19 = 0;
        }
        break;
    case 1:                                                                  /* AA0B */
        d->ent[0][2] = 0x80;
        d->ent[1][2] = 0x84;
        if (--d->t18 == 0) {                                                 /* AA7D */
            d->t18 = (EnemyU8)(0x10 + (s7Random() & 0x0F));
            dragonWave(d);
        }
        dragonApproach((EnemyU8 *)d->ent, d->tgt);
        dragonApproach((EnemyU8 *)d->ent, d->tgt);
        if (--d->c0c == 0) {
            d->c0c = 0x18;
            e->state = 2;
        }
        break;
    default:                                                                 /* AA37 */
        d->ent[0][2] = 0x70;
        d->ent[1][2] = 0x74;
        if (--d->c0c == 0) {
            d->c0c = 0x1E;
            e->state = 1;
        } else if (d->c0c == 8) {
            EnemyS16 vy, vx;
            dragonAim(e->x, e->y, v, &vy, &vx);                              /* the box of the last pass : A0E5 reads +3 and +5 */
            if (vx < 0) dragonFire(s, e, vy, vx, d->ent[0][1], (EnemyU8)(d->ent[0][0] + 12));
        }
        break;
    }
    e->y = (EnemyU8)(d->ent[0][0] + 0x10);                                   /* A9C7 */
    e->x = (EnemyS16)((EnemyU8)(d->ent[0][1] + 8));
}

/* AACD : the pieces all at (X, Y) with the patterns of AB0E, the X targets X + D0h, E0h, F0h, 0 (twice each). */
static Enemy *dragonSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    static const EnemyU8 pat[8][2] = { { 0x80, 0x02 }, { 0x84, 0x4C }, { 0x78, 0x02 }, { 0x7C, 0x4C }, { 0x78, 0x02 }, { 0x7C, 0x4C }, { 0x78, 0x02 }, { 0x7C, 0x4C } };
    static const EnemyU8 dx[4] = { 0xD0, 0xE0, 0xF0, 0x00 };
    Enemy *e = newObject(s, ENEMY_TYPE_DRAGON);
    DragonExt *d;
    int k;
    if (!e) return 0;
    d = dgx(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = DRAGON_HP;
    e->state = 0;
    d->c0c = 0x20;
    d->y1a = y;
    d->x1b = x;
    for (k = 0; k < 4; k++) d->tgt[2 * k] = d->tgt[2 * k + 1] = (EnemyU8)(dx[k] + x);
    for (k = 0; k < 8; k++) {
        d->ent[k][0] = y;
        d->ent[k][1] = x;
        d->ent[k][2] = pat[k][0];
        d->ent[k][3] = pat[k][1];
    }
    d->t18 = d->i19 = 0;
    return e;
}

/* 9F6D, D700 type 1 (A08E[0E]) in (X, Y) with vy +7/+8, vx +9/+A : the fireball of the fishman with any speed. State 1 marks the aimed shot, vx in (pose, anim),
 * the fractions in sub and timer, moved in the pass of its creation (the D700 loop follows the C800 loop). */
static Enemy *dragonFire(EnemyState *s, const Enemy *src, EnemyS16 vy, EnemyS16 vx, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FIREBALL);
    if (!e) return 0;
    e->state = 1;
    e->vy = vy;
    e->pose = (EnemyU8)((EnemyU16)vx >> 8);
    e->anim = (EnemyU8)vx;
    e->dx = (EnemyS8)(vx < 0 ? -1 : 1);
    e->x = x;
    e->y = y;
    e->sub = 0;
    e->timer = 0;
    if (e > src) e->age = 0;
    else { fireballStep(e); e->age = 1; }
    return e;
}

/* The four cells of the pieces (entries 0, 2, 4, 6 : the two layers composed in the cell) in the type 5 block : the closed head 80h -> 90, the open head 70h -> 98, the
 * body 78h -> 94 ; SAT (X, Y) of the entry, out.y = Y + 1. */
static int dragonSprites(const EnemyState *s, const Enemy *e, EnemySprite *out)
{
    const DragonExt *const d = &dg[e - s->enemy];
    int k;
    for (k = 0; k < 4; k++) {
        const EnemyU8 p = d->ent[2 * k][2];
        out[k].y = (EnemyU8)(d->ent[2 * k][0] + 1);
        out[k].x = d->ent[2 * k][1];
        out[k].pattern = (EnemyU8)(p == 0x80 ? 0x90 : p == 0x70 ? 0x98 : 0x94);
        out[k].bank = 1;
    }
    return 4;
}
