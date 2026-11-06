/*
 * Objects of the stage 11 : research/stage11-enemies.md.
 *
 * - stage11_vectors (tests/stage11_vectors.h, tools/gen_stage11_vectors.py) : the eagle 0F (5F1D, init B193, routine B1C3) and the flea 23h it carries (init and
 *   routine B212 / B248) run pass by pass by the ROM routines in a Z80 emulator on the maps of the rooms 2 to 4, Simon moved by a scripted triangle wave ;
 *   the port must give the same state, place, fractions, pose, speeds and counters at every pass, for both objects.
 * - the channel 9DE7 (period, Y by the parity of the shot count, no eagle with Simon at X >= C0h) and the rules read in the ROM (points of life, damage, score,
 *   box, SAT entries).
 * enemies.c is included to reach the per-object state (s8x).
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enemies.c"
#include "gameplay.h"
#include "room_manager.h"
#include "stage11_vectors.h"

static EnemyRoomVars roomVars(VKGame *g, int stage, int room, int sx, int sy)
{
    EnemyRoomVars v;
    memset(&v, 0, sizeof v);
    assert(vk_game_debug_start(g, stage, room)); vk_game_room_map(g);
    v.stage = (EnemyU8)stage; v.room = (EnemyU8)room; v.simonX = (EnemyU8)sx; v.simonY = (EnemyU8)sy; v.map = vk_game_collision(g);
    return v;
}

static int same(const Enemy *e, int k, const int *row, int isEagle, const char *who, int vec, int p)
{
    if (!row[0]) {
        if (e->active) { printf("%s vector %d pass %d : ROM gone, port alive\n", who, vec, p); return 1; }
        return 0;
    }
    if (!e->active) { printf("%s vector %d pass %d : ROM alive (state %d X %d Y %d), port gone\n", who, vec, p, row[1], row[2], row[3]); return 1; }
    if (e->state != row[1] || e->x != row[2] || e->y != row[3] || e->pose != row[4] || (s8x[k].vx & 0xFFFF) != row[5] || (e->vy & 0xFFFF) != row[6] ||
        s8x[k].mobile != row[7] || s8x[k].c12 != row[8] || (isEagle && s8x[k].c11 != row[9]) || e->timer != row[10] || e->sub != row[11]) {
        printf("%s vector %d pass %d : ROM st %d (%d,%d) pose %02X vx %04X vy %04X mob %d +C %d +11 %d fx %d fy %d, port st %d (%d,%d) pose %02X vx %04X vy %04X mob %d +C %d +11 %d fx %d fy %d\n",
               who, vec, p, row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8], row[9], row[10], row[11],
               e->state, e->x, e->y, e->pose, s8x[k].vx & 0xFFFF, e->vy & 0xFFFF, s8x[k].mobile, s8x[k].c12, s8x[k].c11, e->timer, e->sub);
        return 1;
    }
    return 0;
}

static int runVector(int vec, int *hovers, int *drops)
{
    const Stage11Vector *v = &stage11_vectors[vec];
    EnemyState s;
    EnemyRoomVars r;
    VKGame g = { 0 };
    Enemy *flea = 0, *eagle;
    int p, bad = 0;
    const int span = v->hi - v->lo;
    r = roomVars(&g, 11, v->room, v->lo, v->sy);
    Enemies_init(&s);
    Enemies_god = 1;
    Enemies_s7R = v->r;
    eagle = eagleSpawnRaw(&s, 0xE0, (EnemyU8)v->ey, &flea);
    assert(eagle && flea && eagle < flea);
    for (p = 0; p < v->passes; p++) {
        const int *row = &stage11_rows[(v->first + p) * 24];
        const int tt = span ? (p * v->step) % (2 * span) : 0;
        int i;
        r.simonX = (EnemyU8)(v->lo + (tt > span ? 2 * span - tt : tt));
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s.enemy[i];
            if (!e->active) continue;
            if (e->type == ENEMY_TYPE_EAGLE) eagleStep(&s, e, &r);
            else if (e->type == ENEMY_TYPE_FLEA) fleaStep(&s, e, &r);
        }
        bad += same(eagle, (int)(eagle - s.enemy), row, 1, "eagle", vec, p);
        bad += same(flea, (int)(flea - s.enemy), row + 12, 0, "flea", vec, p);
        if (p && row[1] == 1 && row[7] == 0) (*hovers)++;
        if (p && row[13] == 1) (*drops)++;
        if (bad) break;
    }
    Enemies_s7R = -1;
    return bad;
}

int main(void)
{
    EnemyState s;
    EnemyRoomVars r;
    VKGame g = { 0 };
    EnemySprite sp[ENEMY_SPRITES];
    EnemySpawnTimer t;
    int i, bad = 0, passes = 0, hovers = 0, drops = 0;
    const int nvec = (int)(sizeof stage11_vectors / sizeof stage11_vectors[0]);

    for (i = 0; i < nvec; i++) {
        const int b = runVector(i, &hovers, &drops);
        bad += b;
        passes += stage11_vectors[i].passes;
        if (b) printf("vector %d (room %d eagle Y %02X R %d) : %d bad\n", i, stage11_vectors[i].room, stage11_vectors[i].ey, stage11_vectors[i].r, b);
    }
    assert(bad == 0);
    assert(hovers > 0 && drops > 0);

    /* The channel 9DE7 : counter 8 at the room entry, then one eagle every 25 groups of 4 calls (100 calls, 200 images), Y 30h then 40h, X E0h. */
    r = roomVars(&g, 11, 2, 40, 176);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    {
        int first[3] = { 0, 0, 0 }, n = 0, call, k;
        EnemyU8 y[3] = { 0, 0, 0 };
        for (call = 1; call < 8 + 4 * 25 * 3 + 8 && n < 3; call++) {
            eagleChannel(&s, &t, &r);
            for (k = 0; k < ENEMY_MAX; k++)
                if (s.enemy[k].active && s.enemy[k].type == ENEMY_TYPE_EAGLE) {
                    first[n] = call; y[n] = s.enemy[k].y; n++;
                    for (k = 0; k < ENEMY_MAX; k++) s.enemy[k].active = 0;     /* free the SAT entries */
                    break;
                }
        }
        assert(n == 3 && first[0] > 0 && first[1] - first[0] == 100 && first[2] - first[1] == 100);
        assert(y[0] == 0x30 && y[1] == 0x40 && y[2] == 0x30);
    }
    /* Simon at X >= C0h : no eagle, the count is set to 1 (a retry 4 calls later). */
    r = roomVars(&g, 11, 2, 0xC0, 176);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    for (i = 0; i < 200; i++) eagleChannel(&s, &t, &r);
    for (i = 0; i < ENEMY_MAX; i++) assert(!(s.enemy[i].active && s.enemy[i].type == ENEMY_TYPE_EAGLE));

    /* The first pass is made at the creation : the eagle and its flea have moved by 2.5 px (vx FD80h) : X DDh (E0 - 3) with the fraction 80h, the flea at (E1, 3C). */
    r = roomVars(&g, 11, 3, 40, 176);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    Enemies_s7R = 0;
    s.type4Count = 1;
    t.phase = 1;
    eagleChannel(&s, &t, &r);
    {
        int ke = -1, kf = -1, k;
        for (k = 0; k < ENEMY_MAX; k++) {
            if (s.enemy[k].active && s.enemy[k].type == ENEMY_TYPE_EAGLE) ke = k;
            if (s.enemy[k].active && s.enemy[k].type == ENEMY_TYPE_FLEA) kf = k;
        }
        assert(ke >= 0 && kf > ke);
        assert(s.enemy[ke].x == 0xDD && s.enemy[ke].timer == 0x80 && s.enemy[ke].y == 0x30);
        assert(s.enemy[kf].x == 0xE1 && s.enemy[kf].y == 0x3C && s.enemy[kf].state == 0);
        assert(s.enemy[ke].extraPhase == EAGLE_HP && s.enemy[kf].extraPhase == 1);
        /* the rules read in the ROM : damage 4 (81CE pair 04 02), 400 points, class 4 box 0Ch x 18h, 6 SAT entries, flame 18h above, 8 points of life */
        assert(isObjS4(ENEMY_TYPE_EAGLE) && hasHp(ENEMY_TYPE_EAGLE) && s4Score(ENEMY_TYPE_EAGLE) == 4 && satCount(ENEMY_TYPE_EAGLE) == 6 && deathOffset(ENEMY_TYPE_EAGLE) == 0x18);
        {
            int l, h;
            boxOf(ENEMY_TYPE_EAGLE, &l, &h);
            assert(l == 0x0C && h == 0x18);
        }
        s.enemy[ke].x = 100; s.enemy[ke].y = 80; s.enemy[ke].timer = 0; s.enemy[ke].pose = 0x6D;
        assert(Enemies_sprites(&s, (EnemyU8)ke, sp) == 3);
        assert(sp[0].pattern == 0x40 && sp[0].x == 86 && sp[0].y == 80 - 22 && sp[1].pattern == 0x48 && sp[1].x == 102 && sp[1].y == 80 - 30 &&
               sp[2].pattern == 0x50 && sp[2].x == 102 && sp[2].y == 80 - 14);
        s.enemy[ke].pose = 0x6E; s.enemy[ke].timer = 0x80;
        assert(Enemies_sprites(&s, (EnemyU8)ke, sp) == 3 && sp[1].pattern == 0x58 && sp[2].pattern == 0x60 && sp[0].x == 87 && sp[1].x == 103);
        s.enemy[ke].pose = 0x8D;
        assert(Enemies_sprites(&s, (EnemyU8)ke, sp) == 3 && sp[0].y == 80 - 24 && sp[1].pattern == 0x68 && sp[2].pattern == 0x70);
    }
    Enemies_s7R = -1;

    printf("object 0F / 23h (eagle and flea of the stage 11): %d vectors, %d passes against the ROM routines, channel and rules PASS\n", nvec, passes);
    return 0;
}
