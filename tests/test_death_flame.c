/*
 * Death flame (object 1E, created by 9A3E when an enemy dies): research/death-flame.md.
 *
 * Everything below comes from the openMSX traces of that note (scratchpad/dflame, plans mk.py / mk2.py, bp 98EC / 9A3E):
 * - kTypes: dead object (ROM X, Y) -> flame (X, Y - D), D from the table 9B33 (stages 1, 2, 4, 6, 7, 8, 9: types 1, 2, 4, 5, 6, 7, 8,
 *   0A, 0B, 0D, by whip, thrown weapon and contact with C434);
 * - kPass: the state (+B, +C) of the first flame of the trace A at the start of each of the 16 passes;
 * - 32 frames on screen (sprite from the pass of the death), +E = 0, not frozen by D010, first free block, 2 SAT entries.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "enemies.c"

static EnemyRoomVars roomVars(int sx, int sy)
{
    EnemyRoomVars v;
    memset(&v, 0, sizeof v);
    v.simonX = (EnemyU8)sx; v.simonY = (EnemyU8)sy;
    return v;
}

static int findFlame(const EnemyState *s)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_DEATH) return i;
    return -1;
}

/* (port type, ROM X, ROM Y of the dead object, flame Y): trace values, flame X = X. */
static const int kTypes[][4] = {
    { ENEMY_TYPE_A, 0x87, 0xA0, 0x90 },     { ENEMY_TYPE_B, 0x7B, 0xA0, 0x90 },     { ENEMY_TYPE_4, 0x8A, 0xB2, 0xB2 },
    { ENEMY_TYPE_5, 0x6C, 0x80, 0x80 },     { ENEMY_TYPE_2, 0xD0, 0xBD, 0xAD },     { ENEMY_TYPE_6, 0xDD, 0xC0, 0xB0 },
    { ENEMY_TYPE_8, 0x64, 0xC4, 0xC4 },     { ENEMY_TYPE_7, 0x58, 0x78, 0x78 },     { ENEMY_TYPE_STATUE, 0xC0, 0x50, 0x40 },
    { ENEMY_TYPE_HOPPER, 0x44, 0x90, 0x80 }, { ENEMY_TYPE_FLEA, 0x78, 0xBB, 0xBB }, { ENEMY_TYPE_LEAPER, 0x70, 0x80, 0x80 },
    { ENEMY_TYPE_GHOST, 0x40, 0x70, 0x70 },
};

/* Trace A, flame created in the pass 631, state at the start of the passes 632 to 647: (+B, +C). */
static const int kPass[16][2] = {
    { 0x85, 0x10 }, { 0x85, 0x0F }, { 0x86, 0x0E }, { 0x86, 0x0D }, { 0x86, 0x0C }, { 0x86, 0x0B }, { 0x85, 0x0A }, { 0x85, 0x09 },
    { 0x85, 0x08 }, { 0x85, 0x07 }, { 0x86, 0x06 }, { 0x86, 0x05 }, { 0x86, 0x04 }, { 0x86, 0x03 }, { 0x85, 0x02 }, { 0x85, 0x01 },
};

/* A dead object as the port stores it (object coordinates for 5, 2 and the objects of the stages 4 to 9, SAT-based for 1 and 4). */
static Enemy deadAt(int type, int x, int y)
{
    Enemy d;
    memset(&d, 0, sizeof d);
    d.type = (EnemyU8)type;
    if (type == ENEMY_TYPE_A || type == ENEMY_TYPE_B) { d.romBox = 1; d.x = (EnemyS16)(x - ENEMY_OBJ_X); d.y = (EnemyU8)(y - ENEMY_OBJ_Y); }
    else if (type == ENEMY_TYPE_4) { d.x = (EnemyS16)(x - ENEMY_OBJ_X); d.y = (EnemyU8)(y - ENEMY_TYPE4_OBJ_Y); }
    else { d.x = (EnemyS16)x; d.y = (EnemyU8)y; }
    return d;
}

static void places(void)
{
    EnemyState s;
    size_t n;
    for (n = 0; n < sizeof kTypes / sizeof kTypes[0]; n++) {
        const Enemy d = deadAt(kTypes[n][0], kTypes[n][1], kTypes[n][2]);
        int k;
        Enemies_init(&s);
        deathFlame(&s, &d);
        k = findFlame(&s);
        assert(k >= 0);
        assert(s.enemy[k].x == kTypes[n][1] && s.enemy[k].y == kTypes[n][3]);
        assert(s.enemy[k].anim == 0x10 && s.enemy[k].pose == 0x85);
    }
    /* The objects of the table D700 (ball, bone) and the drops do not leave a flame (822A, 99F6). */
    {
        const Enemy ball = deadAt(ENEMY_TYPE_FIREBALL, 100, 100), bone = deadAt(ENEMY_TYPE_BONE, 100, 100);
        Enemies_init(&s);
        deathFlame(&s, &ball); deathFlame(&s, &bone);
        assert(findFlame(&s) < 0);
    }
}

static void timeline(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r = roomVars(40, 100);
    EnemySprite sp[ENEMY_SPRITES];
    int i, k, shown = 0;
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_god = 1;
    /* A zombie of the ROM struck by the whip: Simon at 100, facing right, standing; zombie at (X 0x87, Y 0xA0). */
    i = Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0 - ENEMY_OBJ_Y, 1);
    assert(i == 0);
    s.enemy[0].romBox = 1;
    s.enemy[0].x = 0x87 - ENEMY_OBJ_X;
    s.enemy[0].y = 0xA0 - ENEMY_OBJ_Y;
    assert(Enemies_whipHitType1Rom(&s, 0x87 - 20, 0xA0 - 2, 0, 0, 0) == 1);
    assert(!s.enemy[0].active || s.enemy[0].type == ENEMY_TYPE_DEATH);
    k = findFlame(&s);
    assert(k == 0 && s.enemy[k].x == 0x87 && s.enemy[k].y == 0x90);
    /* Frame 0 is the pass of the death (drawn), the update comes two frames later, then every two frames: 32 frames on screen. */
    for (i = 0; i < 40; i++) {
        const int step = i / 2;
        if (!s.enemy[k].active) break;
        shown++;
        if (i % 2 == 0 && step < 16) {
            assert(s.enemy[k].anim == kPass[step][1] && s.enemy[k].pose == kPass[step][0]);
        } else if (step < 16) {
            assert(s.enemy[k].anim == kPass[step][1] && s.enemy[k].pose == kPass[step][0]);   /* unchanged between two passes */
        }
        Enemies_frame(&s, &t, &r);
    }
    assert(shown == 32 && !s.enemy[k].active);
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 0);
}

static int touchAlways(const void *ctx, int slot, int x, int y, int h, int l)
{
    (void)ctx; (void)slot; (void)x; (void)y; (void)h; (void)l;
    return 1;
}

static void rules(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r = roomVars(0x87, 0xA0);
    EnemySprite sp[ENEMY_SPRITES];
    Enemy d = deadAt(ENEMY_TYPE_A, 0x87, 0xA0);
    int i, k, hits;

    /* +E = 0: neither contact nor whip nor thrown weapon (even on Simon), +7E = 0: D010 does not freeze it. */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_god = 0;
    deathFlame(&s, &d);
    k = findFlame(&s);
    for (i = 0; i < 6; i++) Enemies_frame(&s, &t, &r);
    assert(s.energy == ENEMY_ENERGY_MAX && s.invuln == 0);
    assert(Enemies_whipHit(&s, 0x80, 0x80, 0x90, 0xB0) == 0);
    assert(Enemies_whipHitType1Rom(&s, 0x87 - 20, 0xA0, 0, 0, 0) == 0);
    assert(Enemies_whipHitStage4(&s, 0x87 - 20, 0xA0, 0, 0, 0, &hits) == 0 && hits == 0);
    {
        int slot = -1, score = -1;
        assert(Enemies_shotHit(&s, touchAlways, 0, &slot, &score) == 0);
    }
    s.frozen = 1;
    {
        const EnemyU8 before = s.enemy[k].anim;
        Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r);
        assert(s.enemy[k].anim == before - 1);
    }
    s.frozen = 0;

    /* Sprites: one SAT entry (X - 8, Y - 16 pose 85, Y - 15 pose 86), cells F4 / F8 of the bank 2; 2 SAT entries counted. */
    Enemies_init(&s);
    deathFlame(&s, &d);
    k = findFlame(&s);
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 1);
    assert(sp[0].x == 0x87 - 8 && sp[0].y == 0x90 - 15 && sp[0].pattern == 0 && sp[0].bank == 2);
    s.enemy[k].pose = 0x86;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 1 && sp[0].y == 0x90 - 14 && sp[0].pattern == 4 && sp[0].bank == 2);
    assert(satCount(ENEMY_TYPE_DEATH) == 2);

    /* The flame takes a block (5F1D): the first free one, and a full table loses the spawn. */
    Enemies_init(&s);
    for (i = 0; i < ENEMY_MAX; i++) { s.enemy[i].active = 1; s.enemy[i].type = ENEMY_TYPE_8; s.enemy[i].x = 100 + i; s.enemy[i].y = 100; }
    s.enemy[1].active = 0;
    s.enemy[3].active = 0;
    d = deadAt(ENEMY_TYPE_8, 0x64, 0xC4);
    deathFlame(&s, &d);
    assert(findFlame(&s) == 1);
    assert(Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1) == 3);
    assert(Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1) == -1);

    /* The SAT table has 14 entries (satCount): a flame needs 2 free ones, else it is lost. */
    Enemies_init(&s);
    for (i = 0; i < 4; i++) { s.enemy[i].active = 1; s.enemy[i].type = ENEMY_TYPE_5; }   /* 4 x 4 = 16 > 14 */
    s.enemy[2].active = 0;                                                               /* 12 used */
    deathFlame(&s, &d);
    assert(findFlame(&s) == 2);                                                          /* 12 + 2 = 14 */
    s.enemy[4].active = 1; s.enemy[4].type = ENEMY_TYPE_4;                               /* 16 used: no room */
    s.enemy[5].active = 0;
    {
        int before = 0;
        for (i = 0; i < ENEMY_MAX; i++) before += s.enemy[i].active;
        deathFlame(&s, &d);
        for (i = 0; i < ENEMY_MAX; i++) before -= s.enemy[i].active;
        assert(before == 0);
    }

    /* Whip kill of the type 4, 5, 2 and thrown weapon on the type 1: the flame is created by the kill itself. */
    Enemies_init(&s);
    k = Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_4, ENEMY_TYPE4_Y, 0);
    assert(k == 0);
    s.enemy[0].x = 0x8A - ENEMY_OBJ_X; s.enemy[0].y = 0xB2 - ENEMY_TYPE4_OBJ_Y;
    assert(Enemies_whipHitType4(&s, 0x8A - 20, 0xB2 + 18 - 8, 0, 0, 0) == 1);
    k = findFlame(&s);
    assert(k >= 0 && s.enemy[k].x == 0x8A && s.enemy[k].y == 0xB2);
    Enemies_init(&s);
    d = deadAt(ENEMY_TYPE_A, 0x87, 0xA0);
    s.enemy[0] = d; s.enemy[0].active = 1;
    {
        int slot = -1, score = -1;
        assert(Enemies_shotHit(&s, touchAlways, 0, &slot, &score) == 1 && score == 1);
    }
    k = findFlame(&s);
    assert(k == 0 && s.enemy[k].x == 0x87 && s.enemy[k].y == 0x90);

    /* Contact with C434 (7EB0 -> 81AB, 9A3E): the touched object becomes a flame, the ball does not. */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_god = 0;
    s.killTimer = 20;
    s.enemy[0] = d; s.enemy[0].active = 1;
    r = roomVars(0x87, 0xA0);
    Enemies_frame(&s, &t, &r);
    k = findFlame(&s);
    assert(k >= 0 && s.energy == ENEMY_ENERGY_MAX && s.itemSound == 0x0D);
    assert(s.enemy[k].x == 0x87 && s.enemy[k].y == 0x90);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    s.killTimer = 20;
    s.enemy[0] = deadAt(ENEMY_TYPE_FIREBALL, 0x87, 0xA0); s.enemy[0].active = 1;
    Enemies_frame(&s, &t, &r);
    assert(findFlame(&s) < 0 && !s.enemy[0].active);

    /* A new room clears it. */
    Enemies_init(&s);
    deathFlame(&s, &d);
    Enemies_setRoom(&s);
    assert(findFlame(&s) < 0);
}

/* Drop of 9ABE (research/death-flame.md): CF40 + 1 at every death, a multiple of 32 -> flame 26 of the object 13h; else R & 3F = 0 -> flame 26 of the
 * object 02; else R & 3 = 0 -> the death flame carries +1F = 1 and the object 24 is made where it ends (16th update); else nothing. 250 traced
 * draws of R: A & 3F = 0 twice, A & 3 = 0 in 25.8 % of the others (research/hasard-R.md). */
static void drops(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r = roomVars(0, 0);
    const Enemy d = deadAt(ENEMY_TYPE_A, 0x87, 0xA0);
    int i, k, n;
    Enemies_god = 1;
    /* R = 1: no drop, the flame only. */
    Enemies_s7R = 1;
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    deathFlame(&s, &d);
    k = findFlame(&s);
    assert(k >= 0 && s.enemy[k].state == 0 && Enemies_dropCount == 0);
    for (i = 0; i < 40; i++) Enemies_frame(&s, &t, &r);
    assert(!s.enemy[k].active && Enemies_dropCount == 0);
    /* R & 3 = 0 (R = 4): +1F = 1, the heart comes in (X, Y) of the flame at the 16th update, not before. */
    Enemies_s7R = 4;
    Enemies_init(&s);
    deathFlame(&s, &d);
    k = findFlame(&s);
    assert(k >= 0 && s.enemy[k].state == 1 && Enemies_dropCount == 0);
    for (i = 0; i < 31; i++) Enemies_frame(&s, &t, &r);
    assert(s.enemy[k].active && Enemies_dropCount == 0);
    Enemies_frame(&s, &t, &r);
    assert(!s.enemy[k].active && Enemies_dropCount == 1);
    assert(Enemies_drops[0].kind == 1 && Enemies_drops[0].x == 0x87 && Enemies_drops[0].y == 0x90);
    /* R & 3F = 0 (R = 0, 40h): no flame, a flame 26 of the object 02 where the enemy died. */
    for (n = 0; n < 2; n++) {
        Enemies_s7R = n ? 0x40 : 0;
        Enemies_init(&s);
        deathFlame(&s, &d);
        assert(findFlame(&s) < 0 && Enemies_dropCount == 1);
        assert(Enemies_drops[0].kind == 2 && Enemies_drops[0].x == 0x87 && Enemies_drops[0].y == 0x90);
    }
    /* The 32nd death (CF40 & 1F = 0): the object 13h whatever R is; CF40 is cleared by Enemies_init only. */
    Enemies_s7R = 1;
    Enemies_init(&s);
    for (i = 1; i <= 64; i++) {
        deathFlame(&s, &d);
        if (i % 32 == 0) {
            assert(findFlame(&s) < 0 && Enemies_dropCount == 1 && Enemies_drops[0].kind == 0x13);
        } else {
            assert(Enemies_dropCount == 0);
            k = findFlame(&s);
            if (k >= 0) s.enemy[k].active = 0;
        }
        Enemies_dropCount = 0;
    }
    /* Distribution of the port's generator (-1): 1/4 - 1/64 hearts, 1/64 big hearts. */
    {
        int hearts = 0, big = 0, none = 0, total = 6400;
        Enemies_s7R = -1;
        Enemies_init(&s);
        for (i = 1; i <= total; i++) {
            Enemies_dropCount = 0;
            deathFlame(&s, &d);
            if (i % 32 == 0) { Enemies_dropCount = 0; continue; }
            k = findFlame(&s);
            if (k >= 0) { if (s.enemy[k].state) hearts++; else none++; s.enemy[k].active = 0; }
            else big++;
        }
        assert(big > 40 && big < 160);                  /* 6200 / 64 = 97 */
        assert(hearts > 1300 && hearts < 1700);         /* 6200 * (1/4 - 1/64) = 1453 */
        assert(hearts + none + big == total - total / 32);
    }
    Enemies_s7R = 1;
}

int main(void)
{
    Enemies_s7R = 1;           /* no drop: the other tests expect the flame */
    places();
    timeline();
    rules();
    drops();
    Enemies_s7R = -1;
    printf("death flame (object 1E): positions of 13 types, 16 passes of the trace, 32 frames, rules and sprites PASS\n");
    return 0;
}
