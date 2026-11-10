/*
 * Stage 8 objects (research/stage8-enemies.md): the hopper 0B (B37E), the leaper 0C (A67D) and the bone of the
 * hopper (D700 type 4, 9F07). enemies.c is included to reach the per-object state (s8x) that Enemy has no room for.
 *
 * kHop, kLeap and kBone (stage8_trace.inc) are real transitions of the openMSX traces: the object block of one
 * sample, then the next different sample, with Simon's place of that image. One step of the port from the first
 * block must give the second one (Y, X with fractions, vy, vx, state, pose, mobile flag and counters).
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enemies.c"
#include "gameplay.h"
#include "room_manager.h"

typedef struct { int room, sx, sy; const char *a, *b; } StepCase;
#include "stage8_trace.inc"

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
    q->c18 = (EnemyU8)hx(h, 0x18); q->c1e = (EnemyU8)hx(h, 0x1E);
    if (type == ENEMY_TYPE_BONE) { q->c12 = (EnemyU8)hx(h, 0x0C); q->c10 = 0; }
    e->dx = (EnemyS8)(q->vx < 0 ? -1 : 1);
}

static void same(const Enemy *e, const S8Ext *q, const char *h, int type)
{
    assert(e->y == hx(h, 3) && e->sub == hx(h, 2) && e->x == hx(h, 5) && e->timer == hx(h, 4));
    assert(e->vy == (EnemyS16)(hx(h, 7) | hx(h, 8) << 8) && q->vx == (EnemyS16)(hx(h, 9) | hx(h, 10) << 8));
    assert(e->pose == hx(h, 11) && q->mobile == hx(h, 6));
    if (type != ENEMY_TYPE_BONE) assert(e->state == hx(h, 1) && q->c12 == hx(h, 0x12) && q->c10 == hx(h, 0x10));
    if (type == ENEMY_TYPE_LEAPER) assert(q->c18 == hx(h, 0x18) && q->c1e == hx(h, 0x1E) && q->c11 == hx(h, 0x11));
}

static void replay(const StepCase *c, int n, int type)
{
    int i, lastRoom = -1;
    VKGame g = { 0 };
    EnemyState s;
    EnemyRoomVars v = { 0 };
    memset(&s, 0, sizeof s);
    for (i = 0; i < n; i++) {
        Enemy *e = Enemies_slot(&s,0);
        S8Ext *q = &s8x[0];
        if (c[i].room != lastRoom) {
            assert(vk_game_debug_start(&g, 8, c[i].room)); vk_game_room_map(&g); lastRoom = c[i].room;
        }
        v.stage = 8; v.room = (EnemyU8)c[i].room; v.simonX = (EnemyU8)c[i].sx; v.simonY = (EnemyU8)c[i].sy;
        v.map = vk_game_collision(&g);
        load(e, q, c[i].a, type);
        if (type == ENEMY_TYPE_HOPPER) hopperStep(&s, e, &v);
        else if (type == ENEMY_TYPE_LEAPER) leaperStep(&s, e, &v);
        else boneStep(&s, e);
        assert(e->active);
        same(e, q, c[i].b, type);
    }
}

static EnemyRoomVars roomVars(VKGame *g, int room, int sx, int sy)
{
    EnemyRoomVars v = { 0 };
    assert(vk_game_debug_start(g, 8, room)); vk_game_room_map(g);
    v.stage = 8; v.room = (EnemyU8)room; v.simonX = (EnemyU8)sx; v.simonY = (EnemyU8)sy; v.map = vk_game_collision(g);
    return v;
}

static int touchSlot0(const void *ctx, int slot, int x, int y, int h, int l)
{ (void)ctx; (void)x; (void)y; (void)h; (void)l; return slot == 0; }

int main(void)
{
    VKGame g = { 0 };
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    Enemy *e;
    int n, k, hits, score, slot;

    /* Real transitions: one pass of the port reproduces the next sample of the trace. */
    replay(kHop, (int)(sizeof kHop / sizeof kHop[0]), ENEMY_TYPE_HOPPER);
    replay(kLeap, (int)(sizeof kLeap / sizeof kLeap[0]), ENEMY_TYPE_LEAPER);
    replay(kBone, (int)(sizeof kBone / sizeof kBone[0]), ENEMY_TYPE_BONE);

    /* Room 2: the hopper 0B of the list (208, 80), 4 hit points, vx toward Simon (16, 144), then X - 2.25 per pass
     * (trace R: D0.00, CD.C0, CB.80, C9.40). */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    v = roomVars(&g, 2, 16, 144);
    Enemies_frame(&s, &t, &v);
    e = Enemies_slot(&s,0);
    assert(e->active && e->type == ENEMY_TYPE_HOPPER && e->x == 208 && e->y == 80 && e->extraPhase == 4);
    assert(ext(&s,Enemies_slot(&s,0))->vx == -0x240 && ext(&s,Enemies_slot(&s,0))->mobile == 1);
    Enemies_frame(&s, &t, &v);   /* B344 falls into B364: the creation already counts one in +12 (ROM, stage 7 Z80 vectors) */
    assert(e->x == 0xCD && e->timer == 0xC0 && ext(&s,Enemies_slot(&s,0))->c12 == 2 && e->pose == 0x48);
    Enemies_frame(&s, &t, &v); Enemies_frame(&s, &t, &v);
    assert(e->x == 0xCB && e->timer == 0x80 && ext(&s,Enemies_slot(&s,0))->c12 == 3);

    /* Room 5: the leaper 0C of the list (80, 80), pose 89, 18h passes of waiting, then the first leap (A7BD):
     * Simon (244, 144) is lower and to the right: vx +200h, vy +220h (table A81B, +18 & 3 = 0). */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    v = roomVars(&g, 5, 244, 144);
    Enemies_frame(&s, &t, &v);
    e = Enemies_slot(&s,0);
    assert(e->active && e->type == ENEMY_TYPE_LEAPER && e->x == 80 && e->y == 80 && e->pose == 0x89 && e->extraPhase == 1);
    assert(ext(&s,Enemies_slot(&s,0))->c10 == 0x18 && !ext(&s,Enemies_slot(&s,0))->mobile);
    for (n = 0; n < 46; n++) Enemies_frame(&s, &t, &v);
    assert(e->state == 0 && ext(&s,Enemies_slot(&s,0))->c10 == 1 && !ext(&s,Enemies_slot(&s,0))->mobile);       /* 23 passes done */
    Enemies_frame(&s, &t, &v);                                         /* frame 47: not a pass */
    Enemies_frame(&s, &t, &v);                                         /* frame 48: pass 24, the leap and its first move */
    assert(e->state == 1 && ext(&s,Enemies_slot(&s,0))->mobile && ext(&s,Enemies_slot(&s,0))->vx == 0x200 && e->vy == 0x220);
    assert(e->x == 82 && e->timer == 0 && e->y == 82 && e->sub == 0x20);

    /* Fixed list of rooms 6 and 7: statue + leaper, hopper. */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    v = roomVars(&g, 6, 16, 144);
    Enemies_frame(&s, &t, &v);
    n = 0; k = 0;
    for (slot = 0; slot < ENEMY_ALL; slot++) {
        if (Enemies_slot(&s,slot)->active && Enemies_slot(&s,slot)->type == ENEMY_TYPE_STATUE) { n++; assert(Enemies_slot(&s,slot)->x == 144 && Enemies_slot(&s,slot)->y == 144); }
        if (Enemies_slot(&s,slot)->active && Enemies_slot(&s,slot)->type == ENEMY_TYPE_LEAPER) { k++; assert(Enemies_slot(&s,slot)->x == 80 && Enemies_slot(&s,slot)->y == 192); }
    }
    assert(n == 1 && k == 1);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    v = roomVars(&g, 7, 16, 144);
    Enemies_frame(&s, &t, &v);
    assert(Enemies_slot(&s,0)->active && Enemies_slot(&s,0)->type == ENEMY_TYPE_HOPPER && Enemies_slot(&s,0)->x == 128 && Enemies_slot(&s,0)->y == 192);

    /* The throw (B4A9, 9F61): 10h passes of windup, then a bone in (X, Y - 10h), vx +-180h toward Simon, vy -500h or
     * -800h (R register, random), moved in the same pass: trace R f1970 gives X 66.80, Y 78.50, vy F850. */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    v = roomVars(&g, 2, 16, 144);
    Enemies_frame(&s, &t, &v);
    e = Enemies_slot(&s,0);
    e->x = 0x68; e->y = 0x90; e->timer = 0; e->sub = 0; e->state = 2; ext(&s,Enemies_slot(&s,0))->c10 = 1; ext(&s,Enemies_slot(&s,0))->mobile = 0;
    Enemies_frame(&s, &t, &v);
    k = -1;
    for (slot = 0; slot < ENEMY_ALL; slot++) if (Enemies_slot(&s,slot)->active && Enemies_slot(&s,slot)->type == ENEMY_TYPE_BONE) k = slot;
    assert(k >= 0 && e->state == 0 && ext(&s,Enemies_slot(&s,0))->mobile);
    assert(ext(&s,Enemies_slot(&s,k))->vx == -0x180 && Enemies_slot(&s,k)->x == 0x66 && Enemies_slot(&s,k)->timer == 0x80 && Enemies_slot(&s,k)->pose == 0x4B);
    assert(Enemies_slot(&s,k)->vy == -0x7B0 || Enemies_slot(&s,k)->vy == -0x4B0);
    assert(Enemies_slot(&s,k)->y == (Enemies_slot(&s,k)->vy == -0x7B0 ? 0x78 : 0x7B));

    /* Contacts: hopper and leaper -2 (81CE base 1, doubled), bone -1 (7F3D, 462B with B = 1), no armor. */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    v = roomVars(&g, 2, 100, 144);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_HOPPER; Enemies_slot(&s,0)->x = 104; Enemies_slot(&s,0)->y = 144; Enemies_slot(&s,0)->extraPhase = 4;
    Enemies_slot(&s,0)->age = 1;   /* next frame is not a pass */
    ext(&s,Enemies_slot(&s,0))->mobile = 0; ext(&s,Enemies_slot(&s,0))->vx = 0;
    assert(Enemies_touches(Enemies_slot(&s,0), &v));
    assert(Enemies_frame(&s, &t, &v) == 1 && s.energy == 30);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_LEAPER; Enemies_slot(&s,0)->x = 104; Enemies_slot(&s,0)->y = 144; Enemies_slot(&s,0)->extraPhase = 1;
    Enemies_slot(&s,0)->age = 1; ext(&s,Enemies_slot(&s,0))->mobile = 0; ext(&s,Enemies_slot(&s,0))->c10 = 9; ext(&s,Enemies_slot(&s,0))->vx = 0;
    assert(Enemies_frame(&s, &t, &v) == 1 && s.energy == 30);
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_BONE; Enemies_slot(&s,0)->x = 104; Enemies_slot(&s,0)->y = 144; Enemies_slot(&s,0)->age = 1;
    ext(&s,Enemies_slot(&s,0))->mobile = 0; ext(&s,Enemies_slot(&s,0))->vx = 0;
    assert(Enemies_frame(&s, &t, &v) == 1 && s.energy == 31 && !Enemies_slot(&s,0)->active);   /* the bone is destroyed by the contact */

    /* Whip (7DAD, 7DE6): the hopper has 4 hit points (1 per hit, 2 with the chain), 200 points; the leaper 1, 100 points;
     * the bone dies at the first hit, 100 points. Simon (100, 144) turned right: the whip reaches X 120. */
    Enemies_init(&s); Enemies_spawnTimerInit(&t);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_HOPPER; Enemies_slot(&s,0)->x = 120; Enemies_slot(&s,0)->y = 144; Enemies_slot(&s,0)->extraPhase = 4;
    for (k = 0; k < 3; k++) {
        assert(Enemies_whipHitStage4(&s, 100, 144, 0, 0, 0, &hits) == 0 && hits == 1 && Enemies_slot(&s,0)->active);
        Enemies_rearm(&s);
    }
    assert(Enemies_whipHitStage4(&s, 100, 144, 0, 0, 0, &hits) == 1 && !Enemy_alive(s.enemy[0]) && Enemies_killScore == 2);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_HOPPER; Enemies_slot(&s,0)->x = 120; Enemies_slot(&s,0)->y = 144; Enemies_slot(&s,0)->extraPhase = 4;   /* the flame of the kill took the block */
    assert(Enemies_whipHitStage4(&s, 100, 144, 0, 0, 1, &hits) == 0 && Enemies_slot(&s,0)->active);
    Enemies_rearm(&s);
    assert(Enemies_whipHitStage4(&s, 100, 144, 0, 0, 1, &hits) == 1 && Enemies_killScore == 2);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_LEAPER; Enemies_slot(&s,0)->extraPhase = 1; Enemies_slot(&s,0)->x = 120; Enemies_slot(&s,0)->y = 138;
    assert(Enemies_whipHitStage4(&s, 100, 144, 0, 0, 0, &hits) == 1 && Enemies_killScore == 1);
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_BONE; Enemies_slot(&s,0)->extraPhase = 0; Enemies_slot(&s,0)->x = 120; Enemies_slot(&s,0)->y = 134;
    assert(Enemies_whipHitStage4(&s, 100, 144, 0, 0, 0, &hits) == 1 && Enemies_killScore == 1);

    /* Thrown weapons (82A4, 8066): the axe (4) kills the hopper at once, the dagger (1) does not. */
    Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_HOPPER; Enemies_slot(&s,0)->extraPhase = 4; Enemies_slot(&s,0)->x = 120; Enemies_slot(&s,0)->y = 144;
    Enemies_shotPower[0] = 1;
    assert(Enemies_shotHit(&s, touchSlot0, 0, &slot, &score) == 1 && Enemies_slot(&s,0)->active && (Enemies_slot(&s,0)->extraPhase & 0x0F) == 3);
    Enemies_slot(&s,0)->extraPhase = 4;
    Enemies_shotPower[0] = 4;
    assert(Enemies_shotHit(&s, touchSlot0, 0, &slot, &score) == 1 && !Enemy_alive(s.enemy[0]) && score == 2);

    /* Sprites (trace R and L): hopper facing left 47 -> cells 64 and 68, facing right 4A -> 7C and 80, leaper 8B -> 94,
     * bone 4B in X - 4, 4C in X - 8. */
    {
        EnemySprite sp[ENEMY_SPRITES];
        memset(&s, 0, sizeof s);
        Enemies_slot(&s,0)->active = 1; Enemies_slot(&s,0)->type = ENEMY_TYPE_HOPPER; Enemies_slot(&s,0)->x = 100; Enemies_slot(&s,0)->y = 144; Enemies_slot(&s,0)->pose = 0x47;
        assert(Enemies_sprites(&s, 0, sp) == 2 && sp[0].pattern == 0x64 && sp[1].pattern == 0x68 && sp[0].x == 92 && sp[0].y == 113 && sp[1].y == 129);
        Enemies_slot(&s,0)->pose = 0x4A;
        assert(Enemies_sprites(&s, 0, sp) == 2 && sp[0].pattern == 0x7C && sp[1].pattern == 0x80);
        Enemies_slot(&s,0)->type = ENEMY_TYPE_LEAPER; Enemies_slot(&s,0)->pose = 0x8B;
        assert(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0x94 && sp[0].x == 92 && sp[0].y == 130);
        Enemies_slot(&s,0)->type = ENEMY_TYPE_BONE; Enemies_slot(&s,0)->pose = 0x4B;
        assert(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0x98 && sp[0].x == 96);
        Enemies_slot(&s,0)->pose = 0x4C;
        assert(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0x9C && sp[0].x == 92);
    }
    printf("stage 8 objects (hopper 0B, leaper 0C, bone): %d + %d + %d trace transitions, spawn, throw, contact, whip, shots, sprites PASS\n",
           (int)(sizeof kHop / sizeof kHop[0]), (int)(sizeof kLeap / sizeof kLeap[0]), (int)(sizeof kBone / sizeof kBone[0]));
    return 0;
}
