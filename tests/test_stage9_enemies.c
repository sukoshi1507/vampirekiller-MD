/*
 * Stage 9 objects shared with stages 7 and 8 (research/stage9-enemies.md): the hopper 0B (B37E) and its bone (D700 type 4,
 * 9F07) on the stage 9 maps, the statue 0A, the object 08 and the flea 0D. enemies.c is included to reach the per-object
 * state (s8x) that Enemy has no room for.
 *
 * kHop9 and kBone9 (stage9_enemy_trace.inc) are real transitions of the openMSX trace R of stage 9 (Simon still): the
 * object block of one sample and the next different sample; one pass of the port from the first block must give the second.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enemies.c"
#include "gameplay.h"
#include "room_manager.h"

typedef struct { int room, sx, sy; const char *a, *b; } StepCase;
#include "stage9_enemy_trace.inc"

static int hx(const char *s, int i) { char b[3] = { s[2 * i], s[2 * i + 1], 0 }; return (int)strtol(b, 0, 16); }

static void load(Enemy *e, S8Ext *q, const char *h, int type)
{
    memset(e, 0, sizeof *e);
    e->active = 1; e->type = (EnemyU8)type;
    e->state = (EnemyU8)hx(h, 1); e->sub = (EnemyU8)hx(h, 2); e->y = (EnemyU8)hx(h, 3);
    e->timer = (EnemyU8)hx(h, 4); e->x = (EnemyS16)hx(h, 5);
    q->mobile = (EnemyU8)hx(h, 6);
    e->vy = (EnemyS16)(hx(h, 7) | hx(h, 8) << 8); q->vx = (EnemyS16)(hx(h, 9) | hx(h, 10) << 8);
    e->pose = (EnemyU8)hx(h, 11); e->extraPhase = (EnemyU8)hx(h, 13);
    q->c12 = (EnemyU8)hx(h, 0x12); q->c10 = (EnemyU8)hx(h, 0x10); q->c11 = (EnemyU8)hx(h, 0x11);
    if (type == ENEMY_TYPE_BONE) { q->c12 = (EnemyU8)hx(h, 0x0C); q->c10 = 0; }
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
}

static void same(const Enemy *e, const S8Ext *q, const char *h, int type)
{
    assert(e->y == hx(h, 3) && e->sub == hx(h, 2) && e->x == hx(h, 5) && e->timer == hx(h, 4));
    assert(e->vy == (EnemyS16)(hx(h, 7) | hx(h, 8) << 8) && q->vx == (EnemyS16)(hx(h, 9) | hx(h, 10) << 8));
    assert(e->pose == hx(h, 11) && q->mobile == hx(h, 6));
    if (type != ENEMY_TYPE_BONE) assert(e->state == hx(h, 1) && q->c12 == hx(h, 0x12) && q->c10 == hx(h, 0x10));
}

static void replay(const StepCase *c, int n, int type)
{
    int i, lastRoom = -1;
    VKGame g = { 0 };
    EnemyState s;
    EnemyRoomVars v = { 0 };
    memset(&s, 0, sizeof s);
    for (i = 0; i < n; i++) {
        Enemy *e = &s.enemy[0];
        S8Ext *q = &s8x[0];
        if (c[i].room != lastRoom) {
            assert(vk_game_debug_start(&g, 9, c[i].room)); vk_game_room_map(&g); lastRoom = c[i].room;
        }
        v.stage = 9; v.room = (EnemyU8)c[i].room; v.simonX = (EnemyU8)c[i].sx; v.simonY = (EnemyU8)c[i].sy;
        v.map = vk_game_collision(&g);
        load(e, q, c[i].a, type);
        if (type == ENEMY_TYPE_HOPPER) hopperStep(&s, e, &v);
        else boneStep(&s, e);
        assert(e->active);
        same(e, q, c[i].b, type);
    }
}

static EnemyRoomVars roomVars(VKGame *g, int room, int sx, int sy)
{
    EnemyRoomVars v = { 0 };
    assert(vk_game_debug_start(g, 9, room)); vk_game_room_map(g);
    v.stage = 9; v.room = (EnemyU8)room; v.simonX = (EnemyU8)sx; v.simonY = (EnemyU8)sy; v.map = vk_game_collision(g);
    return v;
}

static int count(const EnemyState *s, int type, int x, int y)
{
    int k, n = 0;
    for (k = 0; k < ENEMY_MAX; k++)
        if (s->enemy[k].active && s->enemy[k].type == type && s->enemy[k].x == x && s->enemy[k].y == y) n++;
    return n;
}

int main(void)
{
    VKGame g = { 0 };
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int n;

    replay(kHop9, (int)(sizeof kHop9 / sizeof kHop9[0]), ENEMY_TYPE_HOPPER);
    replay(kBone9, (int)(sizeof kBone9 / sizeof kBone9[0]), ENEMY_TYPE_BONE);

    /* Objects of the fixed list (85A6 group 2, list 86B6): statues 0A in rooms 1, 2 and 4, hoppers 0B in rooms 4, 6 and 8,
     * fleas 0D in rooms 5 and 8, created at the first game frame of the room. Places are the list entries (Y, X nibbles). */
    Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, 1, 16, 144); Enemies_frame(&s, &t, &v);
    assert(count(&s, ENEMY_TYPE_STATUE, 192, 112) == 1);
    Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, 2, 16, 144); Enemies_frame(&s, &t, &v);
    assert(count(&s, ENEMY_TYPE_STATUE, 80, 112) == 1 && count(&s, ENEMY_TYPE_STATUE, 176, 112) == 1);
    Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, 4, 16, 144); Enemies_frame(&s, &t, &v);
    assert(count(&s, ENEMY_TYPE_STATUE, 80, 112) == 1 && count(&s, ENEMY_TYPE_HOPPER, 176, 112) == 1);
    Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, 5, 16, 144); Enemies_frame(&s, &t, &v);
    assert(count(&s, ENEMY_TYPE_FLEA, 80, 112) == 1 && count(&s, ENEMY_TYPE_FLEA, 112, 80) == 1);
    Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, 6, 16, 144); Enemies_frame(&s, &t, &v);
    assert(count(&s, ENEMY_TYPE_HOPPER, 64, 80) == 1);
    Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, 8, 16, 144); Enemies_frame(&s, &t, &v);
    assert(count(&s, ENEMY_TYPE_HOPPER, 48, 176) == 1 && count(&s, ENEMY_TYPE_FLEA, 112, 80) == 1);
    for (n = 0; n < 9; n++) {                          /* rooms 0, 3 and 7 have no list entry, room 7 is the boss room */
        Enemies_init(&s); Enemies_spawnTimerInit(&t); v = roomVars(&g, n, 16, 144); Enemies_frame(&s, &t, &v);
        if (n == 0 || n == 3 || n == 7) assert(count(&s, ENEMY_TYPE_STATUE, 0, 0) == 0 && s.enemy[0].active == 0);
    }

    /* The flea cells of the stage 9 block are the stage 7 ones moved to A4, A8, AC, B0 (80 to 8C at stage 7). */
    {
        EnemySprite sp[ENEMY_SPRITES];
        memset(&s, 0, sizeof s);
        v = roomVars(&g, 5, 16, 144);
        Enemies_frame(&s, &t, &v);
        s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_FLEA; s.enemy[0].pose = 0x67;
        assert(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0xA4);
        s.enemy[0].pose = 0x6B;
        assert(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0xB0);
        v = roomVars(&g, 5, 16, 144); v.stage = 7;
        Enemies_frame(&s, &t, &v);
        s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_FLEA; s.enemy[0].pose = 0x6B;
        assert(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0x8C);
    }
    printf("stage 9 objects (hopper 0B, bone, statue 0A, flea 0D, fixed list): %d + %d trace transitions, spawns, sprites PASS\n",
           (int)(sizeof kHop9 / sizeof kHop9[0]), (int)(sizeof kBone9 / sizeof kBone9[0]));
    return 0;
}
