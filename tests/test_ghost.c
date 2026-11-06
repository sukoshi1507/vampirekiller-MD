/*
 * Object 1C released by the candles of object 15 (stages 7, 8 and 9): research/stage9-candle-object.md.
 *
 * - ghost_vectors (tests/ghost_vectors.h, tools/gen_ghost_vectors.py): the ROM routines B4D2, B4EA, 99B9 and 99E5 run pass by pass in a
 *   Z80 emulator on the maps of the stages 7 to 9, Simon moved by a scripted triangle wave (above and below the object, so that the
 *   jump B58E, the fall B5EF, the edge stops B59E and the walls are all covered); the port must give the same state, place, pose,
 *   speeds and counter at every pass.
 * - kTrace: the openMSX trace of stage 9 room 2 (candle (48, 64), Simon (36, 112)): the object seen at the samples, in passes of two
 *   frames from its creation.
 * - the rules read in the ROM and seen in the traces: +E = 0 while it waits, 1 point of life, 100 points, damage 4, box 8 x 16, sprites.
 * enemies.c is included to reach the per-object state (s8x).
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enemies.c"
#include "gameplay.h"
#include "room_manager.h"
#include "ghost_vectors.h"

static EnemyRoomVars roomVars(VKGame *g, int stage, int room, int sx, int sy)
{
    EnemyRoomVars v;
    memset(&v, 0, sizeof v);
    assert(vk_game_debug_start(g, stage, room)); vk_game_room_map(g);
    v.stage = (EnemyU8)stage; v.room = (EnemyU8)room; v.simonX = (EnemyU8)sx; v.simonY = (EnemyU8)sy; v.map = vk_game_collision(g);
    return v;
}

static int findGhost(const EnemyState *s)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_GHOST) return i;
    return -1;
}

static int runVector(const GhostVector *v)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r;
    VKGame g = { 0 };
    int p, k, bad = 0;
    const int span = v->hi - v->lo;
    r = roomVars(&g, v->stage, v->room, v->lo, v->sy);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_god = 1;
    Enemies_frame(&s, &t, &r);                               /* the room is loaded */
    for (k = 0; k < ENEMY_MAX; k++) s.enemy[k].active = 0;   /* the fixed objects of the room are not the subject */
    Enemies_candleObject(&s, (EnemyU8)v->stage, (EnemyU8)v->room, (EnemyU8)v->x, (EnemyU8)(v->y - 0x10));
    k = findGhost(&s);
    assert(k >= 0 && s.enemy[k].x == v->x && s.enemy[k].y == v->y);
    for (p = 0; p < v->passes; p++) {
        const int *row = &ghost_rows[(v->first + p) * 8];
        const int tt = span ? (p * v->step) % (2 * span) : 0;
        const Enemy *e;
        r.simonX = (EnemyU8)(v->lo + (tt > span ? 2 * span - tt : tt));
        Enemies_frame(&s, &t, &r);                           /* the pass */
        Enemies_frame(&s, &t, &r);
        for (k = 0; k < ENEMY_MAX; k++) if (s.enemy[k].active && s.enemy[k].type != ENEMY_TYPE_GHOST) s.enemy[k].active = 0;
        k = findGhost(&s);
        e = &s.enemy[k < 0 ? 0 : k];
        if (!row[7] != (k < 0)) bad++;
        else if (k >= 0 && (e->state != row[0] || e->x != row[1] || e->y != row[2] || e->pose != row[3] ||
                            (e->vy & 0xFFFF) != row[5] || (s8x[k].vx & 0xFFFF) != row[4] || s8x[k].c10 != row[6])) {
            if (bad < 3)
                printf("stage %d room %d X %d Y %d pass %d : ROM st %d (%d,%d) pose %02X vx %04X vy %04X t %d, port st %d (%d,%d) pose %02X vx %04X vy %04X t %d\n",
                       v->stage, v->room, v->x, v->y, p, row[0], row[1], row[2], row[3], row[4], row[5], row[6],
                       e->state, e->x, e->y, e->pose, s8x[k].vx & 0xFFFF, e->vy & 0xFFFF, s8x[k].c10);
            bad++;
        }
        if (!row[7]) break;
    }
    return bad;
}

/* openMSX trace, stage 9 room 2, the candle (48, 64) put out at the frame 843 (trace out4 of scratchpad/o1c) : the samples of the
 * object from its creation (frame 874), in passes of two frames. (state, Y, X, vx, vy, pose, +10h) */
static const int kTrace[][7] = {
    { 0, 80, 48, 0x0000, 0x0000, 0x9F, 8 }, { 1, 82, 48, 0x0000, 0x0200, 0x9F, 0 }, { 1, 84, 48, 0x0000, 0x0240, 0x9F, 0 },
    { 1, 86, 48, 0x0000, 0x0280, 0x9F, 0 }, { 1, 89, 48, 0x0000, 0x02C0, 0x9F, 0 }, { 1, 92, 48, 0x0000, 0x0300, 0x9F, 0 },
    { 2, 112, 48, 0x0000, 0x0000, 0xA0, 0x1E },
};

static void rules(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r;
    VKGame g = { 0 };
    EnemySprite sp[ENEMY_SPRITES];
    int k, hits, i;

    /* Creation (8AD0 -> 5F1D, B4D2): at (X, candle Y + 16), 1 point of life, waits 8 passes with +E = 0. */
    r = roomVars(&g, 9, 2, 20, 144);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_frame(&s, &t, &r);
    for (k = 0; k < ENEMY_MAX; k++) s.enemy[k].active = 0;
    Enemies_candleObject(&s, 9, 2, 48, 64);
    k = findGhost(&s);
    assert(k >= 0 && s.enemy[k].x == 48 && s.enemy[k].y == 80 && s.enemy[k].extraPhase == 1 && s.enemy[k].pose == 0x9F);
    assert(s8x[k].c10 == 8 && s8x[k].mobile == 1 && s.enemy[k].state == 0 && ghostInert(&s.enemy[k]));
    Enemies_candleObject(&s, 3, 2, 48, 64);                  /* the groups 0 (stages 0 to 3) and 3 (stage 10) of D002 give nothing */
    Enemies_candleObject(&s, 10, 2, 48, 64);
    for (i = 0, hits = 0; i < ENEMY_MAX; i++) hits += s.enemy[i].active;
    assert(hits == 1);

    /* +E = 0: neither the whip nor contact reach it while it waits (Simon on it for 4 frames, 2 passes). */
    Enemies_god = 0;
    r.simonX = 48; r.simonY = 96;
    for (i = 0; i < 4; i++) Enemies_frame(&s, &t, &r);
    assert(s.energy == 32 && s.enemy[k].state == 0);
    assert(Enemies_whipHitStage4(&s, 48, 96 + 12, 0, 0, 0, &hits) == 0 && hits == 0);

    /* The trace: 8 passes of wait (the 8th makes vy = 200h), then the fall by 40h per pass and the rest at the floor (Y 112). */
    r = roomVars(&g, 9, 2, 20, 144);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_god = 1;
    Enemies_frame(&s, &t, &r);
    for (k = 0; k < ENEMY_MAX; k++) s.enemy[k].active = 0;
    Enemies_candleObject(&s, 9, 2, 48, 64);
    k = findGhost(&s);
    for (i = 0; i < 8; i++) { Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r); }
    assert(s.enemy[k].state == 1 && s.enemy[k].y == kTrace[1][1] && (s.enemy[k].vy & 0xFFFF) == kTrace[1][4] && s.enemy[k].pose == kTrace[1][5]);
    for (i = 2; i < 6; i++) {
        Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r);
        assert(s.enemy[k].state == 1 && s.enemy[k].y == kTrace[i][1] && (s.enemy[k].vy & 0xFFFF) == kTrace[i][4]);
    }
    for (i = 0; i < 40 && s.enemy[k].state != 2; i++) { Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r); }
    assert(s.enemy[k].state == 2 && s.enemy[k].y == 112 && s.enemy[k].x == 48 && s8x[k].c10 == 0x1E && s8x[k].mobile == 0);

    /* Contact 4 (81CE pair 01 02, doubled; trace: 32 -> 28), box of the class 2 (8 x 16) as the flea; 1 point of life; 100 points. */
    Enemies_god = 0;
    r.simonX = 48; r.simonY = 112;
    for (i = 0; i < 4 && s.energy == 32; i++) Enemies_frame(&s, &t, &r);
    assert(s.energy == 28 && s.invuln);
    assert(Enemies_whipHitStage4(&s, 60, 112, 1, 1, 0, &hits) == 1 && hits == 1 && Enemies_killScore == 1);
    assert(!Enemy_alive(s.enemy[k]));
    /* standing Simon: the whip passes over it (84A7: C425 - 12h - Y + 10h < 14h only from 2 px below the object's Y) */
    Enemies_init(&s);
    Enemies_candleObject(&s, 9, 2, 48, 64);
    k = findGhost(&s);
    s.enemy[k].state = 2;
    assert(Enemies_whipHitStage4(&s, 60, 112, 1, 0, 0, &hits) == 0 && hits == 0);

    /* Sprites: the cells B4 (odd pose) and B8 (even pose), SAT (X - 8, Y - 15). */
    Enemies_init(&s);
    Enemies_candleObject(&s, 9, 2, 48, 64);
    k = findGhost(&s);
    s.enemy[k].pose = 0x9F;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 1 && sp[0].pattern == 0xB4 && sp[0].x == 40 && sp[0].y == 80 - 14 && sp[0].bank == 1);
    s.enemy[k].pose = 0xA0;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 1 && sp[0].pattern == 0xB8);
    /* Pose bases of the rooms (B647 tables). */
    assert(Enemies_ghostBase(9, 2) == 0x9F && Enemies_ghostBase(7, 1) == 0xA1 && Enemies_ghostBase(7, 3) == 0x9B &&
           Enemies_ghostBase(8, 0) == 0x9F && Enemies_ghostBase(8, 7) == 0x9B && Enemies_ghostBase(3, 0) == 0);
}

/* Object 1A (stages 4, 5 and 6, D002 = 1): openMSX traces (tools/build_ghost_a_md.py, research/stage9-candle-object.md), the 17 candles
 * and blocks of object 15 put out one by one. Same routine as the 1C: the creation at (X, candle Y + 16) 30 passes after the hit, 8
 * passes of wait, the fall (Y 82, 84, 86, 89, 92, 95, 99 from a start at 80), the rest on the floor at Y & F8, the pose base of the
 * room (9F, 9D in the stage 4 rooms 1 and 3, 9B in the stage 6 rooms 0, 1 and 2). */
static const int kFall[7] = { 2, 4, 6, 9, 12, 15, 19 };   /* Y - start, every pass from the 8th */

static void trace456(int stage, int room, int x, int candleY, int floorY, int base, int hole)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r;
    VKGame g = { 0 };
    int i, k;
    const int y0 = candleY + 16;
    r = roomVars(&g, stage, room, 20, 40);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_god = 1;
    Enemies_frame(&s, &t, &r);
    for (k = 0; k < ENEMY_MAX; k++) s.enemy[k].active = 0;
    Enemies_candleObject(&s, (EnemyU8)stage, (EnemyU8)room, (EnemyU8)x, (EnemyU8)candleY);
    k = findGhost(&s);
    assert(k >= 0 && s.enemy[k].x == x && s.enemy[k].y == y0 && s.enemy[k].state == 0 && s8x[k].c10 == 8);
    assert(s.enemy[k].pose == base && Enemies_ghostBase((EnemyU8)stage, (EnemyU8)room) == base);
    for (i = 0; i < 7; i++) { Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r); assert(s.enemy[k].state == 0 && s.enemy[k].y == y0); }
    Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r);
    assert(s.enemy[k].state == 1 && s.enemy[k].y == y0 + kFall[0] && (s.enemy[k].vy & 0xFFFF) == 0x200);
    for (i = 1; i < 7; i++) {
        Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r);
        if (hole) { assert(s.enemy[k].state == 1 && s.enemy[k].y == y0 + kFall[i]); }
        else if (i < 6) assert(s.enemy[k].state == 1 && s.enemy[k].y == y0 + kFall[i] && (s.enemy[k].vy & 0xFFFF) == 0x200 + 0x40 * i);
    }
    if (hole) {
        /* 5 room 5 (144, 144): no floor under it, it falls on (Y 162 to 222 in the trace) and goes at Y >= E4 (99E5). */
        static const int ys[] = { 183, 187, 191, 195, 200, 205, 210, 216, 222 };
        for (i = 0; i < 9; i++) {
            Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r);
            assert(s.enemy[k].active && s.enemy[k].state == 1 && s.enemy[k].y == ys[i]);
        }
        Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r);
        assert(!s.enemy[k].active);
        return;
    }
    for (i = 0; i < 20 && s.enemy[k].state != 2; i++) { Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r); }
    assert(s.enemy[k].state == 2 && s.enemy[k].y == floorY && s.enemy[k].x == x && s8x[k].c10 == 0x1E && s8x[k].mobile == 0);
}

static void rules456(void)
{
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int k;
    /* Pose bases (B665, B668, B66B). */
    assert(Enemies_ghostBase(4, 0) == 0x9F && Enemies_ghostBase(4, 1) == 0x9D && Enemies_ghostBase(4, 3) == 0x9D && Enemies_ghostBase(4, 5) == 0x9F &&
           Enemies_ghostBase(5, 0) == 0x9F && Enemies_ghostBase(5, 5) == 0x9F && Enemies_ghostBase(6, 0) == 0x9B && Enemies_ghostBase(6, 3) == 0x9F &&
           Enemies_ghostBase(6, 5) == 0x9B && Enemies_ghostBase(4, 6) == 0 && Enemies_ghostBase(10, 0) == 0);
    /* The traces of the candles: (stage, room, X, Y of the candle + 16, floor, pose base). */
    trace456(4, 0, 128, 64, 96, 0x9F, 0);
    trace456(4, 1, 192, 48, 128, 0x9D, 0);           /* base 9D, falls to 128 */
    trace456(4, 2, 80, 48, 96, 0x9F, 0);
    trace456(5, 0, 64, 32, 96, 0x9F, 0);
    trace456(5, 4, 208, 64, 160, 0x9F, 0);
    trace456(5, 5, 80, 48, 96, 0x9F, 0);
    trace456(6, 1, 96, 32, 64, 0x9B, 0);
    trace456(6, 1, 32, 128, 160, 0x9B, 0);
    trace456(6, 4, 96, 96, 144, 0x9F, 0);
    trace456(5, 5, 144, 144, 0, 0x9F, 1);            /* over the hole: leaves the screen */
    /* Sprites: cells 70 / 74 (stages 4, 5), 80 / 84 (stage 6), SAT (X - 8, Y - 15), as the 1C. */
    Enemies_init(&s);
    {
        EnemyRoomVars r;
        VKGame g = { 0 };
        EnemySpawnTimer t;
        r = roomVars(&g, 4, 1, 20, 40);
        Enemies_spawnTimerInit(&t);
        Enemies_frame(&s, &t, &r);
    }
    Enemies_candleObject(&s, 4, 1, 192, 48);
    k = findGhost(&s);
    s.enemy[k].pose = 0x9D;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 1 && sp[0].pattern == 0x70 && sp[0].x == 184 && sp[0].y == 64 - 14 && sp[0].bank == 1);
    s.enemy[k].pose = 0x9E;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 1 && sp[0].pattern == 0x74);
    assert(Enemies_ghostCell(4) == 0x70 && Enemies_ghostCell(5) == 0x70 && Enemies_ghostCell(6) == 0x80 && Enemies_ghostCell(7) == 0xB4);
    /* Contact 4 and the crouched whip (+100 points), as the 1C (the traces of the stages 4, 5 and 6: energy 32 -> 28, score 000000 -> 000100). */
    {
        EnemyRoomVars r;
        VKGame g = { 0 };
        EnemySpawnTimer t;
        int hits, i;
        r = roomVars(&g, 4, 0, 20, 40);
        Enemies_init(&s); Enemies_spawnTimerInit(&t);
        Enemies_god = 1;
        Enemies_frame(&s, &t, &r);
        for (k = 0; k < ENEMY_MAX; k++) s.enemy[k].active = 0;
        Enemies_candleObject(&s, 4, 0, 128, 64);
        k = findGhost(&s);
        for (i = 0; i < 40 && s.enemy[k].state != 2; i++) { Enemies_frame(&s, &t, &r); Enemies_frame(&s, &t, &r); }
        assert(s.enemy[k].state == 2 && s.enemy[k].y == 96);
        Enemies_god = 0;
        r.simonX = 128; r.simonY = 96;
        for (i = 0; i < 4 && s.energy == 32; i++) Enemies_frame(&s, &t, &r);
        assert(s.energy == 28 && s.invuln);
        assert(Enemies_whipHitStage4(&s, 140, 96, 1, 1, 0, &hits) == 1 && hits == 1 && Enemies_killScore == 1);   /* crouched, facing left */
    }
}

int main(void)
{
    int n, bad = 0, rows = 0;
    for (n = 0; n < (int)(sizeof ghost_vectors / sizeof ghost_vectors[0]); n++) {
        const int b = runVector(&ghost_vectors[n]);
        if (b) printf("vector %d : %d passes differ\n", n, b);
        bad += b;
        rows += ghost_vectors[n].passes;
    }
    assert(bad == 0);
    rules();
    rules456();
    printf("object 1A / 1C (candles of object 15): %d vectors, %d passes against the ROM routines, trace and rules PASS\n",
           (int)(sizeof ghost_vectors / sizeof ghost_vectors[0]), rows);
    return 0;
}
