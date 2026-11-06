/*
 * Objects 03 and 21h of the stage 10 (the fishman that spits, A310) : research/stage10-enemies.md.
 *
 * - fish3_vectors (tests/fish3_vectors.h, tools/gen_fish3_vectors.py) : the ROM routines A2E0 or A2C7, A310, 99B9 and 99E5 run pass by pass in a
 *   Z80 emulator on the maps of the stage 10 (object 03 in the rooms 1 to 5 from the water, object 21h in the rooms 6 to 8), Simon moved by a
 *   scripted triangle wave ; the port must give the same state, place, pose, speeds and counters at every pass, and the shot (D700 type 1) at
 *   the pass the ROM makes it (the port has already moved it by 3 px).
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
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_FISH3) return i;
    return -1;
}

static int findBall(const EnemyState *s)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_FIREBALL) return i;
    return -1;
}

static int runVector(const Fish3Vector *v, int *fires)
{
    EnemyState s;
    EnemyRoomVars r;
    VKGame g = { 0 };
    int p, k, bad = 0, seenFire = 0;
    const int span = v->hi - v->lo;
    r = roomVars(&g, 10, v->room, v->lo, v->sy);
    Enemies_init(&s);
    Enemies_god = 1;
    if (v->type == 3) fish3Spawn(&s, (EnemyU8)v->x, &r);
    else fish3SpawnFixed(&s, (EnemyU8)v->x, (EnemyU8)v->y);
    k = findFish(&s);
    assert(k >= 0 && s.enemy[k].x == v->x && s.enemy[k].y == v->y);
    for (p = 0; p < v->passes; p++) {
        const int *row = &fish3_rows[(v->first + p) * 13];
        const int tt = span ? (p * v->step) % (2 * span) : 0;
        const Enemy *e = &s.enemy[k];
        r.simonX = (EnemyU8)(v->lo + (tt > span ? 2 * span - tt : tt));
        fish3Step(&s, &s.enemy[k], &r);
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
            if (b < 0 || s.enemy[b].x != row[11] + 3 * dir || s.enemy[b].y != row[12] || s.enemy[b].dx != dir || row[10] != 1) {
                printf("type %02X room %d pass %d : ROM shot type %d (%d,%d), port %d (%d,%d) dx %d\n", v->type, v->room, p, row[10], row[11], row[12], b, b < 0 ? 0 : s.enemy[b].x, b < 0 ? 0 : s.enemy[b].y, b < 0 ? 0 : s.enemy[b].dx);
                bad++;
            }
        }
        if (!row[9]) break;
    }
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

int main(void)
{
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
    assert(k >= 0 && s.enemy[k].state == 1 && s.enemy[k].pose == 0x12 && s8x[k].mobile == 0 && s8x[k].c10 == 2 && s8x[k].c11 == 0x10 && f3[k].c1b == 1);
    for (i = 0; i < 6; i++) { fish3Step(&s, &s.enemy[k], &r); assert(s.enemy[k].x == 48 && s8x[k].mobile == 0 && s.enemy[k].state == 1 && s.enemy[k].pose == 0x12); }
    r.simonY = 168;                                          /* Y - 8 < 168 is false : still waiting */
    fish3Step(&s, &s.enemy[k], &r);
    assert(s8x[k].mobile == 0);
    r.simonY = 174;
    for (i = 0; i < 5; i++) {
        const int *t = kTrace[i];
        fish3Step(&s, &s.enemy[k], &r);
        assert(s.enemy[k].state == t[0] && s.enemy[k].x == t[1] && s.enemy[k].timer == t[2] && (s8x[k].vx & 0xFFFF) == t[3] && s.enemy[k].pose == t[4] &&
               s8x[k].c10 == t[5] && s8x[k].c11 == t[6] && s8x[k].mobile == t[7]);
    }

    /* The rules : 2 points of life, damage 4, 200 points, sprites. */
    r = roomVars(&g, 10, 3, 100, 144);
    Enemies_init(&s);
    fish3Spawn(&s, 96, &r);
    k = findFish(&s);
    hp = (EnemyU8)(s.enemy[k].extraPhase & ENEMY_T6_HP_MASK);
    assert(hp == 2 && hasHp(ENEMY_TYPE_FISH3) && isObjS4(ENEMY_TYPE_FISH3) && s4Score(ENEMY_TYPE_FISH3) == 2 && satCount(ENEMY_TYPE_FISH3) == 4);
    assert(s.enemy[k].x == 96 && s.enemy[k].y == 0xC8 && s.enemy[k].pose == 0x12 && (s.enemy[k].vy & 0xFFFF) == 0xF480);
    {   /* the two drops of the dive (A2E0) */
        int drops = 0;
        for (i = 0; i < ENEMY_MAX; i++) drops += s.enemy[i].active && s.enemy[i].type == ENEMY_TYPE_SPLASH;
        assert(drops == 2);
    }
    s.enemy[k].pose = 0x11; s.enemy[k].x = 100; s.enemy[k].y = 160;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 2 && sp[0].pattern == 0x4C && sp[1].pattern == 0x50 && sp[0].x == 92 && sp[0].y == 160 - 31 && sp[1].y == 160 - 15 && sp[0].bank == 1);
    s.enemy[k].pose = 0x14;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 2 && sp[0].pattern == 0x64 && sp[1].pattern == 0x68);
    s.enemy[k].pose = 0x13;
    assert(Enemies_sprites(&s, (EnemyU8)k, sp) == 2 && sp[0].pattern == 0x54 && sp[1].pattern == 0x5C);

    {   /* the shot: SAT (X - 5, Y - 12) to the right, (X - 11, Y - 12) to the left (trace R), the ball of the statue (anim 0) keeps X - 4 */
        Enemy *ball = newObject(&s, ENEMY_TYPE_FIREBALL);
        assert(ball);
        ball->x = 100; ball->y = 80; ball->anim = 3; ball->dx = 1;
        assert(Enemies_sprites(&s, (EnemyU8)(ball - s.enemy), sp) == 1 && sp[0].x == 95 && sp[0].y == 80 - 11 && sp[0].pattern == 0x60);
        ball->dx = -1;
        assert(Enemies_sprites(&s, (EnemyU8)(ball - s.enemy), sp) == 1 && sp[0].x == 89);
        ball->anim = 0;
        assert(Enemies_sprites(&s, (EnemyU8)(ball - s.enemy), sp) == 1 && sp[0].x == 96);
        fireballStep(ball);
        assert(ball->x == 96);
    }

    printf("object 03 / 21h (fishman of the stage 10): %d vectors, %d passes against the ROM routines, %d shots, trace and rules PASS\n",
           (int)(sizeof fish3_vectors / sizeof fish3_vectors[0]), passes, fires);
    return 0;
}
