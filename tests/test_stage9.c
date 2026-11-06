/*
 * Boss of stage 9 (research/stage9-boss.md): two walkers, object 14h (B87C, B893, B9CF), and their fireball (D700 type 7,
 * 9E90), plus what follows the defeat (the script B6AB, shared with stages 3 and 6). boss.c is included to reach the
 * per-object routines.
 *
 * kWalk and kShot (stage9_boss_trace.inc) are real transitions of the openMSX traces R (Simon still) and B (Simon moved
 * every 120 frames) of the boss room: the 32-byte block of one sample and the next different sample, with Simon's place of
 * that image. One pass of the port from the first block must give the second. B9B6 draws the next wait from the R register
 * of the Z80: the replay accepts any of its 8 values (the table has 8 entries) and checks every byte of the object.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boss.c"
#include "enemies.h"

typedef struct { int room, sx, sy; const char *a, *b; } StepCase;
#include "stage9_boss_trace.inc"

static int hx(const char *s, int i) { char b[3] = { s[2 * i], s[2 * i + 1], 0 }; return (int)strtol(b, 0, 16); }
static int w16(const char *s, int i) { return hx(s, i) | hx(s, i + 1) << 8; }

static void loadw(VKBossWalker *w, const char *h)
{
    memset(w, 0, sizeof *w);
    w->alive = 1;
    w->state = (VKBossU8)hx(h, 1); w->y = (unsigned short)w16(h, 2); w->x = (unsigned short)w16(h, 4);
    w->moving = (VKBossU8)hx(h, 6); w->vy = (short)w16(h, 7); w->vx = (short)w16(h, 9); w->pose = (VKBossU8)hx(h, 11);
    w->wait = (VKBossU8)hx(h, 0x10); w->timer = (VKBossU8)hx(h, 0x11); w->facing = (VKBossU8)hx(h, 0x12);
    w->anim = (VKBossU8)hx(h, 0x13); w->idx = (VKBossU8)hx(h, 0x14); w->dir = (VKBossU8)hx(h, 0x15); w->turns = (VKBossU8)hx(h, 0x17);
}

static int samew(const VKBossWalker *w, const char *h)
{
    return w->state == hx(h, 1) && w->y == w16(h, 2) && w->x == w16(h, 4) && w->moving == hx(h, 6) && w->vy == (short)w16(h, 7) &&
           w->vx == (short)w16(h, 9) && w->pose == hx(h, 11) && w->wait == hx(h, 0x10) && w->timer == hx(h, 0x11) &&
           w->facing == hx(h, 0x12) && w->anim == hx(h, 0x13) && w->idx == hx(h, 0x14) && w->dir == hx(h, 0x15) && w->turns == hx(h, 0x17);
}

static int replayWalk(const StepCase *c)
{
    int r;
    for (r = 0; r < 8; r++) {
        VKBoss b;
        VKBossWalker w;
        memset(&b, 0, sizeof b);
        Enemies_s7R = r;
        loadw(&w, c->a);
        walker_pass(&b, &w, c->sx, c->sy, 0);
        if (w.moving) { w.y = (unsigned short)(w.y + (unsigned short)w.vy); w.x = (unsigned short)(w.x + (unsigned short)w.vx); }
        if (samew(&w, c->b)) return 1;
    }
    return 0;
}

static void replayShot(const StepCase *c)
{
    VKBoss b;
    VKBossShot *s = &b.shot[0];
    memset(&b, 0, sizeof b);
    b.kind = 2;
    s->active = 1; s->y = (unsigned short)w16(c->a, 2); s->x = (unsigned short)w16(c->a, 4);
    s->vy = (short)w16(c->a, 7); s->vx = (short)w16(c->a, 9); s->anim = (VKBossU8)hx(c->a, 0x0C);
    b.shot_tgt[0] = (unsigned short)w16(c->a, 0x10); b.shot_slope[0] = (unsigned short)w16(c->a, 0x12);
    shots_pass(&b);
    if (!s->active) return;                                          /* left the screen (99E5) */
    assert(s->y == w16(c->b, 2) && s->x == w16(c->b, 4) && s->vy == (short)w16(c->b, 7) && s->vx == (short)w16(c->b, 9));
    assert(s->anim == hx(c->b, 0x0C) && b.shot_tgt[0] == w16(c->b, 0x10));
}

static void passes(VKBoss *b, int n, int x, int y)
{
    int i;
    for (i = 0; i < n * 2; i++) vk_boss_frame(b, x, y, 0, 0, 32);
}

int main(void)
{
    VKBoss b;
    VKBossSprite sp[VK_BOSS_MAX_SPRITES];
    int i, n, ok = 0, events, frames, x, y, h, l;

    /* The door of room 8 leads to the boss room 7 by its left link, X 246, Y of the door room (77D1); not stage 5 or 10. */
    assert(vk_boss_door(9, 8) && !vk_boss_door(9, 7) && vk_boss_room(9, 7) && !vk_boss_room(9, 8) && !vk_boss_door(8, 8) && !vk_boss_door(10, 8));
    assert(vk_boss_room_of(9) == 7 && vk_boss_room_of(3) == 5 && vk_boss_room_of(6) == 5);
    assert(vk_boss_entry_x(9) == 246 && vk_boss_entry_y(9) < 0);

    Enemies_s7R = -1;
    /* B9B6 and BF32 draw from the common generator (research/hasard-R.md): 8 table entries, equally often over a period of 65536 draws;
     * a forced R gives the table entry R & 7 (bit 7 ignored). */
    {
        VKBoss rb;
        int count[8] = { 0 }, k, j;
        static const VKBossU8 tab[8] = { 0x40, 0x08, 0x30, 0x10, 0x20, 0x18, 0x38, 0x28 };
        memset(&rb, 0, sizeof rb);
        for (k = 0; k < 65536; k++) { VKBossU8 v = rand_wait(&rb); for (j = 0; j < 8; j++) if (tab[j] == v) count[j]++; }
        for (j = 0; j < 8; j++) assert(count[j] == 8192);
        for (k = 0; k < 256; k++) { Enemies_s7R = k; assert(rand_wait(&rb) == tab[k & 7]); }
        Enemies_s7R = -1;
    }

    /* Real transitions. */
    for (i = 0; i < (int)(sizeof kWalk / sizeof kWalk[0]); i++) { assert(replayWalk(&kWalk[i])); ok++; }
    for (i = 0; i < (int)(sizeof kShot / sizeof kShot[0]); i++) replayShot(&kShot[i]);

    /* Creation (B857, B87C) : (30h, C5h) and (D0h, C5h), pose 36, +10 = 20h, one pass already done (trace R 4508 : 1Eh two passes later). */
    vk_boss_begin_stage(&b, 9);
    assert(b.kind == 2 && b.energy == 0x80);
    passes(&b, 1, 16, 128);
    assert(b.alive && b.walker[0].x == 0x3000 && b.walker[1].x == 0xD000 && b.walker[0].y == 0xC500 && b.walker[1].y == 0xC500);
    assert(b.walker[0].wait == 0x1F && b.walker[1].wait == 0x1F && b.walker[0].pose == 0x36 && !b.walker[0].moving);
    passes(&b, 1, 16, 128);
    assert(b.walker[0].wait == 0x1E);

    /* The fireball (B985, A0ED, 9F6D, A04D) : walker 0 in (9Ah, C5h) at the end of state 3, Simon (16, 128), D012 0 : first
     * sample of trace R f4877 after the move : Y ACB8h, X 9802h, vy FFB8h, vx FE02h; the walker goes back to state 2, moving. */
    b.walker[0].x = 0x9A00; b.walker[0].state = 3; b.walker[0].moving = 0; b.walker[0].timer = 1; b.walker[0].facing = 1;
    passes(&b, 1, 16, 128);
    assert(b.shot[0].active && b.shot[0].y == 0xACB8 && b.shot[0].x == 0x9802 && b.shot[0].vy == (short)0xFFB8 && b.shot[0].vx == (short)0xFE02);
    assert(b.walker[0].state == 2 && b.walker[0].moving);

    /* Damage : 4 with the whip (C416 0 or 2), 6 otherwise (7E59, 7E60 [14h - 11h]) ; 2 walkers, one bar. */
    assert(!vk_boss_hit(&b, 0) && b.energy == 0x7C);
    assert(!vk_boss_hit(&b, 1) && b.energy == 0x76);
    assert(!vk_boss_hit(&b, 2) && b.energy == 0x72);

    /* Boxes of class 6 (82CB[14h]) : body 8420 (28h, 5), whip 8428, thrown weapons 8430; fireball class 1 : (6, 6). */
    b.walker[0].x = 100 << 8; b.walker[0].y = 0xC5 << 8; b.walker[1].x = 220 << 8;
    assert(vk_boss_touch_x(&b, 100, 192, 0) == 100 && vk_boss_touches(&b, 105, 192, 0) && !vk_boss_touches(&b, 111, 192, 0));
    assert(vk_boss_touch_x(&b, 220, 192, 0) == 220 && vk_boss_touch_x(&b, 160, 192, 0) < 0);
    assert(vk_boss_whip_touches(&b, 80, 192, 0, 0, 0) && !vk_boss_whip_touches(&b, 60, 192, 0, 0, 0));   /* the whip reaches X + 20 */
    assert(vk_boss_target(&b, 0, &x, &y, &h, &l) && x == 100 && y == 0xC5 && h == 0x28 && l == 5);
    vk_boss_set_armed(&b, 0);
    assert(!vk_boss_target(&b, 0, &x, &y, &h, &l) && vk_boss_target(&b, 1, &x, &y, &h, &l) && x == 220);
    vk_boss_rearm(&b);
    assert(vk_boss_target(&b, 0, &x, &y, &h, &l) && !vk_boss_target(&b, 2, &x, &y, &h, &l));
    b.shot[1].active = 1; b.shot[1].x = 50 << 8; b.shot[1].y = 190 << 8;
    assert(vk_boss_shot_touches(&b, 50 + 10, 190 + 6, 0) == 1 && vk_boss_shot_touches(&b, 50 + 12, 190 + 6, 0) < 0);
    b.shot[1].active = 0;

    /* Sprites : 3 cells per walker, (X - 8, Y - 46 + 16 part), pose 36 -> tiles 36, 40, 44 ; the blinking cells on the even +11. */
    b.walker[0].pose = 0x36; b.walker[0].state = 2; b.walker[1].alive = 0; b.shot[0].active = 0; b.shot[1].active = 0;
    n = vk_boss_sprites(&b, sp);
    assert(n == 3 && sp[0].x == 92 && sp[0].y == 0xC5 - 46 && sp[1].y == 0xC5 - 30 && sp[2].y == 0xC5 - 14 && sp[0].tile == 36 && sp[2].tile == 44 && sp[0].size == 2);
    b.walker[0].state = 3; b.walker[0].timer = 4;
    n = vk_boss_sprites(&b, sp);
    assert(n == 3 && sp[0].tile == VK_BOSS9_FLASH_TILE && sp[1].tile == VK_BOSS9_FLASH_TILE + 4);
    b.walker[0].timer = 3;
    assert(vk_boss_sprites(&b, sp) == 3 && sp[0].tile == 36);

    /* Defeat : the two walkers and the fireballs go, the script B6AB (orb at 78h, 40h, landing C0h : trace C2) is the common one. */
    b.walker[1].alive = 1; b.shot[0].active = 1; b.energy = 4;
    assert(vk_boss_hit(&b, 0) && !b.alive && !b.walker[0].alive && !b.walker[1].alive && !b.shot[0].active && b.flash_wait == 0x14);
    frames = 0;
    do { events = vk_boss_frame(&b, 16, 128, 0, 0, 32); frames++; } while (!(events & VK_BOSS_EV_ORB_TILES));
    assert(frames >= 120 && frames <= 135 && b.orb && b.orb_y == 0x40);
    while (!b.orb_landed) vk_boss_frame(&b, 100, 192, 0, 0, 32);
    assert(b.orb_y == 0xC0);
    assert(vk_boss_orb_touch(&b, 120, 192, 0));
    while (b.script != 6) vk_boss_frame(&b, 120, 192, 0, 0, 32);
    frames = 0;
    do { events = vk_boss_frame(&b, 120, 192, 0, 0, 32); frames++; } while (!(events & VK_BOSS_EV_EXIT) && frames < 400);
    assert(events & VK_BOSS_EV_EXIT);
    printf("stage 9 boss: %d walker transitions, %d fireball transitions, creation, aim, damage, boxes, sprites, defeat PASS\n",
           ok, (int)(sizeof kShot / sizeof kShot[0]));
    return 0;
}
