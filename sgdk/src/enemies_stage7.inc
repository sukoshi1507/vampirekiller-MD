/*
 * Stage 7 (D000 = 7): object 0D, the flea (run routine B248, init B212, HP 1, 200 points), states B31A (4), B2C7 (3),
 * B285 (2), B268 (1), B258 (0). Included by enemies.c after boneStep; it uses the statics of that file.
 * Objects 0B (skeleton) and 0C (flying creature) of stage 7 and the bone are the stage 8 ones (hopperStep, leaperStep,
 * boneStep): same ROM routines B37E, A67D and 9F07, proved against the openMSX traces of stage 8.
 * The flea state is in the shared Enemy and S8Ext fields: Y/X and their fractions in y, sub, x, timer; vy in vy; vx in
 * s8x.vx; mobile (+6) in s8x.mobile; the +C counter in s8x.c12; HP (+D) and the hit flag in extraPhase.
 * Checked against the ROM routines run in a Z80 emulator (tests/test_stage7.c) and against an openMSX trace
 * (research/stage7-enemies.md).
 */

/* LD A,R of B216, B29E and B4AD (flea count, flea jump height, bone arc). R depends on the exact number of Z80
 * instructions run, which the port cannot reproduce: a generator stands in for it. Traced in openMSX (research/hasard-R.md,
 * 20 + 776 + 496 + 137 calls in stages 7 to 9): A is always below 80h (bit 7 of R never set) and the other seven bits are
 * uniform. The generator returns 0..7Fh, each value equally often (16-bit full-period LCG, bits 8-14). One stream for all
 * uses, like the single R register. Tests set Enemies_s7R to 0..255 to get a fixed value. */
volatile EnemyS16 Enemies_s7R = -1;
#ifndef VK_RANDOM_PROBE
static EnemyU16 s7seed = 0x1D35;
#endif

static EnemyU8 s7Random(void)
{
#ifdef VK_RANDOM_PROBE
    return VK_RANDOM_PROBE(); /* native harness: actual observed LD A,R inputs */
#else
    if (Enemies_s7R >= 0) return (EnemyU8)Enemies_s7R;
    s7seed = (EnemyU16)(s7seed * 25173u + 13849u);
    return (EnemyU8)((s7seed >> 8) & 0x7F);
#endif
}

EnemyU8 Enemies_randomR(void) { return s7Random(); }

static void fleaPose(Enemy *e, const EnemyRoomVars *v, int c)   /* AD37 -> A62F, table B33A */
{
    static const EnemyU8 poses[4] = { 0x67, 0x68, 0x6A, 0x6B };
    if (v->simonX >= e->x) c += 2;
    e->pose = poses[c];
}

static void fleaJump(Enemy *e, S8Ext *q, const EnemyRoomVars *v)    /* B2AA */
{
    q->mobile = 1;
    setVx(q, e, v->simonX >= e->x ? 0x220 : -0x220, v, 0);
    fleaPose(e, v, 1);
}

/* 7BC0 / 7C1C with BC = 0410 and two probes (mode 2), 7B98 for the ceiling and the ground. */
static int fleaWall(const EnemyRoomVars *v, const Enemy *e, int left)
{
    const int px = (EnemyU8)(e->x + (left ? -0x10 : 0x10));
    int k;
    for (k = 0; k < 2; k++)
        if (s4Solid(v, px, (EnemyU8)(e->y - 4 - 8 * k))) return 1;
    return 0;
}

static void fleaLand(Enemy *e, S8Ext *q)    /* B279 */
{
    e->state = 2;
    e->y = (EnemyU8)(e->y & 0xF8);     /* A9A2 : only Y, the fraction of Y and vy stay */
    q->mobile = 0;
    q->c12 = 2;
}

static void fleaStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    S8Ext *const q = ext(s, e);
    const int ad = (v->simonX - e->x) < 0 ? -(v->simonX - e->x) : (v->simonX - e->x);   /* AF40 */
    switch (e->state) {
    case 4:                                                                  /* B31A */
        fleaPose(e, v, 0);
        if (ad >= 0x3C && --q->c12 != 0) break;
        e->vy = -1;
        e->state = 3;
        fleaJump(e, q, v);
        break;
    case 3:                                                                  /* B2C7 */
        addVy(e, 0xA0);
        if (q->vx != 0 && fleaWall(v, e, q->vx < 0)) setVx(q, e, -q->vx, v, 0);   /* B300, A642 */
        if (e->vy < 0) {                                                     /* rising : ceiling 16 px above */
            if (!floorAt(v, e->x, (EnemyU8)(e->y - 0x10))) break;
            e->vy = 0;
        } else {
            if (!floorAt(v, e->x, e->y)) break;
            fleaPose(e, v, 0);
            fleaLand(e, q);
        }
        break;
    case 2: {                                                                /* B285 */
        int de;
        fleaPose(e, v, 0);
        if (--q->c12 != 0) break;
        e->state = 3;
        de = -0x260;
        if (e->y >= 0x50 && (s7Random() & 3) == 0) de = -0x720;
        e->vy = (EnemyS16)de;
        fleaJump(e, q, v);
        break;
    }
    case 1:                                                                  /* B268 */
        addVy(e, 0xA0);
        fleaLand(e, q);
        break;
    default:                                                                 /* B258 */
        if (ad >= 0x3C) break;
        setVx(q, e, 0, v, 0);
        e->state++;
        break;
    }
    moveObj(e, q);
    if (s4Remove(e)) e->active = 0;
}

static void fleaSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_FLEA);
    S8Ext *q;
    if (!e) return;
    q = ext(s, e);
    e->x = x;
    e->y = y;
    e->extraPhase = 1;                                      /* 60E1 : +D */
    q->c12 = (EnemyU8)(0x3C - (s7Random() >> 3 & 0x0F));    /* B212 : R is 7 bits */
    e->state = 4;
    q->mobile = 0;
    setVx(q, e, -0x280, 0, 0);
    e->pose = 0x67;
    q->c10 = 1;
    e->vy = 0;
}
