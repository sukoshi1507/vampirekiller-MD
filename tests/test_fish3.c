/*
 * Objects 03 and 21h of the stage 10 (the fishman that spits, A310) : research/stage10-enemies.md.
 *
 * - fish3_vectors (tests/fish3_vectors.h, tools/gen_fish3_vectors.py) : the ROM routines A2E0 or A2C7, A310, 99B9 and 99E5 run pass by pass in a
 *   Z80 emulator on the maps of the stage 10 (object 03 in the rooms 1 to 5 from the water, object 21h in the rooms 6 to 8), Simon moved by a
 *   scripted triangle wave ; the port must give the same state, place, pose, speeds and counters at every pass, and the shot (D700 type 1) at
 *   the pass the ROM makes it (constructor position before the separate D700 pass).
 * - kTrace : the openMSX trace R of the stage 10, room 6, the object 21h (48, 176) : the pass where Simon reaches its height and the next ones.
 * - the rules read in the ROM : 2 points of life, damage 4 (81CE pair 02 02), 200 points, box 5 x 18h, sprites.
 * enemies.c is included to reach the per-object state (s8x, f3).
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enemies.c"
#include "gameplay.h"
#include "room_manager.h"
#include "fish3_vectors.h"

static EnemyRoomVars roomVars(VKGame *g, int stage, int room, int sx, int sy)
{
    EnemyRoomVars v;
    memset(&v, 0, sizeof v);
    assert(vk_game_debug_start(g, stage, room)); vk_game_room_map(g);
    v.stage = (EnemyU8)stage; v.room = (EnemyU8)room; v.simonX = (EnemyU8)sx; v.simonY = (EnemyU8)sy; v.map = vk_game_collision(g);
    return v;
}

static int findFish(const EnemyState *s)
{
    int i;
    for (i = 0; i < ENEMY_ALL; i++)
        if (Enemies_object(s,i)->active && Enemies_object(s,i)->type == ENEMY_TYPE_FISH3) return i;
    return -1;
}

static int findBall(const EnemyState *s)
{
    int i;
    for (i = 0; i < ENEMY_ALL; i++)
        if (Enemies_object(s,i)->active && Enemies_object(s,i)->type == ENEMY_TYPE_FIREBALL) return i;
    return -1;
}

static int runVector(const Fish3Vector *v, int *fires)
{
    EnemyState s;
    EnemyRoomVars r;
    VKGame g = { 0 };
    int p, k, bad = 0, seenFire = 0, lands = 0, dives = 0;
    const int span = v->hi - v->lo;
    r = roomVars(&g, 10, v->room, v->lo, v->sy);
    Enemies_init(&s);
    Enemies_god = 1;
    if (v->type == 3) fish3Spawn(&s, (EnemyU8)v->x, &r);
    else fish3SpawnFixed(&s, (EnemyU8)v->x, (EnemyU8)v->y);
    k = findFish(&s);
    assert(k >= 0 && Enemies_slot(&s,k)->x == v->x && Enemies_slot(&s,k)->y == v->y);
    /* A2F9 : seules les créations par A2E0 (type 03) déclenchent 08. */
    if (v->type == 3) assert(s.fishSoundCount == 1 && s.fishSounds[0] == 0x08);
    else assert(s.fishSoundCount == 0);
    for (p = 0; p < v->passes; p++) {
        const int *row = &fish3_rows[(v->first + p) * 13];
        const int tt = span ? (p * v->step) % (2 * span) : 0;
        const Enemy *e = Enemies_slot(&s,k);
        const int wasAir = e->active && e->state == 0;
        s.fishSoundCount = 0; /* horloge des appels A4E8 de cette passe */
        r.simonX = (EnemyU8)(v->lo + (tt > span ? 2 * span - tt : tt));
        fish3Step(&s, Enemies_slot(&s,k), &r);
        if (s.fishSoundCount) {
            assert(s.fishSoundCount == 1);
            if (s.fishSounds[0] == 0x07) {
                assert(wasAir && e->active && e->state == 1); lands++;
            } else if (s.fishSounds[0] == 0x09) {
                assert(wasAir && findFish(&s) < 0); dives++;
            } else assert(!"son absent des appels A4E8 du stage 10");
        }
        if (!row[9] != !(e->active && e->type == ENEMY_TYPE_FISH3)) {   /* a dive frees the block, the drops take it (as in the emulator) */
            if (bad < 3) printf("type %02X room %d X %d pass %d : ROM alive %d, port alive %d (state %d, Y %d)\n", v->type, v->room, v->x, p, row[9], e->active, e->state, e->y);
            bad++;
        }
        else if (e->active && e->type == ENEMY_TYPE_FISH3 && (e->state != row[0] || e->x != row[1] || e->y != row[2] || e->pose != row[3] ||
                               (s8x[k].vx & 0xFFFF) != row[4] || (e->vy & 0xFFFF) != row[5] || s8x[k].c10 != row[6] ||
                               s8x[k].c11 != row[7] || s8x[k].c12 != row[8])) {
            if (bad < 3)
                printf("type %02X room %d X %d Y %d pass %d : ROM st %d (%d,%d) pose %02X vx %04X vy %04X +10 %d +11 %d +12 %d, port st %d (%d,%d) pose %02X vx %04X vy %04X +10 %d +11 %d +12 %d\n",
                       v->type, v->room, v->x, v->y, p, row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8],
                       e->state, e->x, e->y, e->pose, s8x[k].vx & 0xFFFF, e->vy & 0xFFFF, s8x[k].c10, s8x[k].c11, s8x[k].c12);
            bad++;
        }
        if (row[10] && !seenFire) {                                       /* the first shot of the object (D700 block 0) */
            const int b = findBall(&s);
            const int dir = (s8x[k].vx & 0x8000) ? -1 : 1;
            seenFire = 1;
            (*fires)++;
            if (b < 0 || Enemies_slot(&s,b)->x != row[11] || Enemies_slot(&s,b)->y != row[12] || Enemies_slot(&s,b)->dx != dir || row[10] != 1) {
                printf("type %02X room %d pass %d : ROM shot type %d (%d,%d), port %d (%d,%d) dx %d\n", v->type, v->room, p, row[10], row[11], row[12], b, b < 0 ? 0 : Enemies_slot(&s,b)->x, b < 0 ? 0 : Enemies_slot(&s,b)->y, b < 0 ? 0 : Enemies_slot(&s,b)->dx);
                bad++;
            }
        }
        if (!row[9]) break;
    }
    (void)lands; (void)dives; /* compteurs vérifiés par les assert ci-dessus (GCC 16 : -Wunused-but-set-variable) */
    return bad;
}

/* openMSX trace R, stage 10 room 6, object 21h (48, 176), samples of the passes (state, X, X fraction, vx, pose, +10, +11, mobile) from the one where
 * Simon (32, Y 174) comes into its height (Y - 8 < C425 <= Y + 8) : the first walking pass, the stop, then the shot poses. */
static const int kTrace[][8] = {
    { 1, 46, 0x80, 0xFE80, 0x10, 1, 0x10, 1 },
    { 2, 45, 0x00, 0xFE80, 0x10, 0, 0x18, 1 },
    { 2, 45, 0x00, 0xFE80, 0x0F, 0, 0x17, 0 },
    { 2, 45, 0x00, 0xFE80, 0x0F, 0, 0x16, 0 },
    { 2, 45, 0x00, 0xFE80, 0x0F, 0, 0x15, 0 },
};

/* Stage 10 interactions: ROM tables 81CE/60E1/82CB and common Z80 paths
 * 7DAD (whip), 8048/8066 (thrown weapons), 717C/992F (time stop).
 * These are code-derived assertions, NOT new openMSX contact/weapon traces. */
static int fish3AlwaysTouch(const void *ctx, int slot, int x, int y, int h, int l)
{
    (void)ctx;
    assert(x == 100 && y == 112 && h == 0x18 && l == 5);
    return slot == 0;
}

static void test_stage10_interactions(void)
{
    EnemyState s;
    EnemyRoomVars r;
    EnemySpawnTimer t;
    EnemyU8 hearts;
    int k, hits, slot, score;
    memset(&r, 0, sizeof r);
    memset(&t, 0, sizeof t);
    r.stage = 10; r.room = 6; r.simonX = 100; r.simonY = 112;
    Enemies_god = 0;

    /* 81CE : contact 4, class 1 (5 x 18h), invulnerability and recoil. */
    Enemies_init(&s);
    fish3SpawnFixed(&s, 100, 112);
    k = findFish(&s);
    assert(k >= 0 && Enemies_touches(Enemies_slot(&s,k), &r));
    s.frozen = 1; /* hold position so only the contact path is exercised */
    assert(Enemies_frame(&s, &t, &r) == 1);
    assert(s.energy == ENEMY_ENERGY_MAX - 4 && s.recoil && s.invuln == ENEMY_INVULN_START);
    assert(Enemies_frame(&s, &t, &r) == 0 && s.energy == ENEMY_ENERGY_MAX - 4);

    /* 60E1 : two HP; 7DE6 : bare whip -1 HP, rearm 999F, chain -2 HP. */
    Enemies_init(&s);
    fish3SpawnFixed(&s, 100, 112);
    k = findFish(&s);
    assert(Enemies_whipHitStage4(&s, 80, 130, 0, 0, 0, &hits) == 0 && hits == 1);
    assert(Enemies_slot(&s,k)->active && (Enemies_slot(&s,k)->extraPhase & ENEMY_T6_HP_MASK) == 1);
    Enemies_rearm(&s);
    assert(Enemies_whipHitStage4(&s, 80, 130, 0, 0, 0, &hits) == 1 && hits == 1);
    assert(Enemies_slot(&s,k)->type != ENEMY_TYPE_FISH3 && Enemies_killScore == 2);
    Enemies_init(&s);
    fish3SpawnFixed(&s, 100, 112);
    k = findFish(&s);
    assert(Enemies_whipHitStage4(&s, 76, 130, 0, 0, 1, &hits) == 1 && hits == 1);
    assert(Enemies_slot(&s,k)->type != ENEMY_TYPE_FISH3 && Enemies_killScore == 2);

    /* 8048 : knife 1 point, axe 4 points; shot slot 0, 8066/8083. */
    Enemies_init(&s);
    fish3SpawnFixed(&s, 100, 112);
    k = findFish(&s);
    Enemies_shotPower[0] = 1;
    assert(Enemies_shotHit(&s, fish3AlwaysTouch, NULL, &slot, &score) && slot == 0 && score == 0);
    assert(Enemies_slot(&s,k)->active && (Enemies_slot(&s,k)->extraPhase & ENEMY_T6_HP_MASK) == 1);
    Enemies_rearm(&s);
    Enemies_shotPower[0] = 4;
    assert(Enemies_shotHit(&s, fish3AlwaysTouch, NULL, &slot, &score) && slot == 0 && score == 2);
    assert(Enemies_slot(&s,k)->type != ENEMY_TYPE_FISH3);

    /* 717C/992F : while frozen the fixed type 21h cannot advance. */
    Enemies_init(&s);
    fish3SpawnFixed(&s, 100, 112);
    k = findFish(&s);
    r.simonX = 0; r.simonY = 0; /* no contact */
    hearts = 8;
    assert(Enemies_useTimeStop(&s, &hearts, 0, ENEMY_TIME_STOP_ITEM));
    assert(hearts == 3 && s.frozen);
    { Enemy saved = (*Enemies_slot(&s,k));
      int i;
      for (i = 0; i < 12; i++) (void)Enemies_frame(&s, &t, &r);
      assert(Enemies_slot(&s,k)->active && Enemies_slot(&s,k)->x == saved.x && Enemies_slot(&s,k)->y == saved.y &&
             Enemies_slot(&s,k)->pose == saved.pose && Enemies_slot(&s,k)->state == saved.state);
    }
    Enemies_shotPower[0] = 1; /* restore the knife power for following ROM vectors */
    Enemies_god = 1;
}

int main(void)
{
    test_stage10_interactions();
    EnemyState s;
    EnemyRoomVars r;
    VKGame g = { 0 };
    EnemySprite sp[ENEMY_SPRITES];
    int i, k, bad = 0, passes = 0, fires = 0, dives = 0;
    EnemyU8 hp;

    for (i = 0; i < (int)(sizeof fish3_vectors / sizeof fish3_vectors[0]); i++) {
        const int b = runVector(&fish3_vectors[i], &fires);
        bad += b;
        passes += fish3_vectors[i].passes;
        if (b) printf("vector %d (type %02X room %d X %d Y %d) : %d bad\n", i, fish3_vectors[i].type, fish3_vectors[i].room, fish3_vectors[i].x, fish3_vectors[i].y, b);
    }
    assert(bad == 0);
    assert(fires >= 20);
    (void)dives;

    /* The trace : the object waits (Simon too high), then walks towards Simon. */
    r = roomVars(&g, 10, 6, 32, 150);
    Enemies_init(&s);
    Enemies_god = 1;
    fish3SpawnFixed(&s, 48, 176);
    k = findFish(&s);
    assert(k >= 0 && Enemies_slot(&s,k)->state == 1 && Enemies_slot(&s,k)->pose == 0x12 && s8x[k].mobile == 0 && s8x[k].c10 == 2 && s8x[k].c11 == 0x10 && f3[k].c1b == 1);
    for (i = 0; i < 6; i++) { fish3Step(&s, Enemies_slot(&s,k), &r); assert(Enemies_slot(&s,k)->x == 48 && s8x[k].mobile == 0 && Enemies_slot(&s,k)->state == 1 && Enemies_slot(&s,k)->pose == 0x12); }
    r.simonY = 168;                                          /* Y - 8 < 168 is false : still waiting */
    fish3Step(&s, Enemies_slot(&s,k), &r);
    assert(s8x[k].mobile == 0);
    r.simonY = 174;
    for (i = 0; i < 5; i++) {
        const int *t = kTrace[i];
        fish3Step(&s, Enemies_slot(&s,k), &r);
        assert(Enemies_slot(&s,k)->state == t[0] && Enemies_slot(&s,k)->x == t[1] && Enemies_slot(&s,k)->timer == t[2] && (s8x[k].vx & 0xFFFF) == t[3] && Enemies_slot(&s,k)->pose == t[4] &&
               s8x[k].c10 == t[5] && s8x[k].c11 == t[6] && s8x[k].mobile == t[7]);
    }

    /* The rules : 2 points of life, damage 4, 200 points, sprites. */
    r = roomVars(&g, 10, 3, 100, 144);
    Enemies_init(&s);
    fish3Spawn(&s, 96, &r);
    k = findFish(&s);
    hp = (EnemyU8)(Enemies_slot(&s,k)->extraPhase & ENEMY_T6_HP_MASK);
    assert(hp == 2 && hasHp(ENEMY_TYPE_FISH3) && isObjS4(ENEMY_TYPE_FISH3) && s4Score(ENEMY_TYPE_FISH3) == 2 && satCount(ENEMY_TYPE_FISH3) == 4);
    assert(Enemies_slot(&s,k)->x == 96 && Enemies_slot(&s,k)->y == 0xC8 && Enemies_slot(&s,k)->pose == 0x12 && (Enemies_slot(&s,k)->vy & 0xFFFF) == 0xF480);
    {   /* the two drops of the dive (A2E0) */
        int drops = 0;
        for (i = 0; i < ENEMY_ALL; i++) drops += Enemies_slot(&s,i)->active && Enemies_slot(&s,i)->type == ENEMY_TYPE_SPLASH;
        assert(drops == 2);
    }
    Enemies_slot(&s,k)->pose = 0x11; Enemies_slot(&s,k)->x = 100; Enemies_slot(&s,k)->y = 160;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 2 && sp[0].pattern == 0x4C && sp[1].pattern == 0x50 && sp[0].x == 92 && sp[0].y == 160 - 31 && sp[1].y == 160 - 15 && sp[0].bank == 1);
    Enemies_slot(&s,k)->pose = 0x14;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 2 && sp[0].pattern == 0x64 && sp[1].pattern == 0x68);
    Enemies_slot(&s,k)->pose = 0x13;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 2 && sp[0].pattern == 0x54 && sp[1].pattern == 0x5C);

    {   /* the shot: SAT (X - 5, Y - 12) to the right, (X - 11, Y - 12) to the left (trace R), the ball of the statue (anim 0) keeps X - 4 */
        Enemy *ball = newObject(&s, ENEMY_TYPE_FIREBALL);
        assert(ball);
        ball->x = 100; ball->y = 80; ball->anim = 3; ball->dx = 1;
        assert(Enemies_sprites(&s, (EnemyU8)objectIndex(&s,ball), sp) == 1 && sp[0].x == 95 && sp[0].y == 80 - 11 && sp[0].pattern == 0x60);
        ball->dx = -1;
        assert(Enemies_sprites(&s, (EnemyU8)objectIndex(&s,ball), sp) == 1 && sp[0].x == 89);
        ball->anim = 0;
        assert(Enemies_sprites(&s, (EnemyU8)objectIndex(&s,ball), sp) == 1 && sp[0].x == 96);
        fireballStep(ball);
        assert(ball->x == 96);
    }

    printf("object 03 / 21h (fishman of the stage 10): %d vectors, %d passes against the ROM routines, %d shots, trace and rules PASS\n",
           (int)(sizeof fish3_vectors / sizeof fish3_vectors[0]), passes, fires);
    return 0;
}
