#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "boss.h"
#include "enemies.h"

static void passes(VKBoss *b, int n, int x, int y)
{
    int i, energy = 32;
    for (i = 0; i < n * 2; i++) vk_boss_frame(b, x, y, 0, 0, energy);
}

/* Limits of the boxes 847A / 84A7 traced in openMSX by re-running the ROM routine with forced parameters at the real
 * call sites (research/boss-boxes.md, "Oracle" section): the forced value is the object offset from Simon (dy = Y - C425,
 * dx = X - C427) or, for the whip, from the tip of the lash. A hit window [lo, hi] must hit inside and miss one pixel out. */
#define SX 100
#define SY 160
static int body_contact(int dx, int dy, int st)
{
    VKBoss b; memset(&b, 0, sizeof b);
    b.alive = 1; b.energy = 0x80; b.x = (unsigned short)((SX + dx) << 8); b.y = (unsigned short)((SY + dy) << 8);
    return vk_boss_touches(&b, SX, SY, st);
}
static int ball_contact_k(int kind, int dx, int dy, int st)
{
    VKBoss b; memset(&b, 0, sizeof b);
    b.kind = (VKBossU8)kind; b.energy = 0x80; b.shot[0].active = 1;
    b.shot[0].x = (unsigned short)((SX + dx) << 8); b.shot[0].y = (unsigned short)((SY + dy) << 8);
    return vk_boss_shot_touches(&b, SX, SY, st) == 0;
}
static int ball3(int dx, int dy, int st) { return ball_contact_k(0, dx, dy, st); }
static int ball6(int dx, int dy, int st) { return ball_contact_k(1, dx, dy, st); }
static int whip_body(VKBoss *b, int rel, int dy, int st, int face, int chain)
{
    const int tip = SX + (face ? -1 : 1) * (chain ? 0x18 : 0x14);
    b->x = (unsigned short)((tip + rel) << 8); b->y = (unsigned short)((SY + dy) << 8);
    return vk_boss_whip_touches(b, SX, SY, face, st, chain);
}
static int whip_ball(VKBoss *b, int rel, int dy, int st, int face, int chain)
{
    const int tip = SX + (face ? -1 : 1) * (chain ? 0x18 : 0x14);
    b->shot[0].x = (unsigned short)((tip + rel) << 8); b->shot[0].y = (unsigned short)((SY + dy) << 8);
    return vk_boss_shot_whip_touches(b, SX, SY, face, st, chain) == 0;
}

static void boxes(void)
{
    int v, face, chain;
    VKBoss b;
    /* Body 180C (stages 3 and 6): vertical window by state of C420 (0 standing, 1 jump, 2 crouch, 3 other), horizontal |dX| <= 16. */
    for (v = -35; v <= 35; v++)
    {
        assert(body_contact(0, v, 0) == (v >= -27 && v <= 22));
        assert(body_contact(0, v, 1) == (v >= -27 && v <= 14));
        assert(body_contact(0, v, 2) == (v >= -19 && v <= 22));
        assert(body_contact(0, v, 3) == (v >= -27 && v <= 22));
    }
    for (v = -20; v <= 20; v++) assert(body_contact(v, -4, 0) == (v >= -16 && v <= 16));
    /* Fireballs: stage 3 (6, 3) |dX| <= 7, stage 6 (and 9) (6, 6) |dX| <= 10; vertical 0: -27..4, jump -27..-4, crouch -19..4. */
    for (v = -14; v <= 14; v++)
    {
        assert(ball3(v, -4, 0) == (v >= -7 && v <= 7));
        assert(ball6(v, -4, 0) == (v >= -10 && v <= 10));
    }
    for (v = -35; v <= 12; v++)
    {
        assert(ball6(0, v, 0) == (v >= -27 && v <= 4));
        assert(ball6(0, v, 1) == (v >= -27 && v <= -4));
        assert(ball6(0, v, 2) == (v >= -19 && v <= 4));
        assert(ball3(0, v, 0) == (v >= -27 && v <= 4));
    }
    /* Whip on the body of stage 3: tip within 23 (leather) or 27 (chain) of X, both directions; vertical -21..6 (standing and
     * jumping), -13..14 (crouched). */
    memset(&b, 0, sizeof b);
    b.alive = 1; b.energy = 0x80;
    for (face = 0; face < 2; face++)
        for (chain = 0; chain < 2; chain++)
            for (v = -32; v <= 32; v++)
            {
                const int lim = chain ? 27 : 23;
                assert(whip_body(&b, v, -8, 0, face, chain) == (v >= -lim && v <= lim));
            }
    for (v = -30; v <= 22; v++)
    {
        assert(whip_body(&b, 0, v, 0, 1, 0) == (v >= -21 && v <= 6));
        assert(whip_body(&b, 0, v, 1, 0, 1) == (v >= -21 && v <= 6));
        assert(whip_body(&b, 0, v, 2, 1, 0) == (v >= -13 && v <= 14));
    }
    /* Whip on a fireball of stage 6 (7F63, HL 0606): tip within 17 (leather) or 21 (chain); vertical -21..-12 / crouched -13..-4. */
    memset(&b, 0, sizeof b);
    b.kind = 1; b.energy = 0x80; b.shot[0].active = 1;
    for (face = 0; face < 2; face++)
        for (chain = 0; chain < 2; chain++)
            for (v = -26; v <= 26; v++)
            {
                const int lim = chain ? 21 : 17;
                assert(whip_ball(&b, v, -16, 0, face, chain) == (v >= -lim && v <= lim));
            }
    for (v = -30; v <= 6; v++)
    {
        assert(whip_ball(&b, 0, v, 0, 1, 0) == (v >= -21 && v <= -12));
        assert(whip_ball(&b, 0, v, 2, 1, 1) == (v >= -13 && v <= -4));
    }
    /* Fireball as a target of the thrown weapons (7F79): box (6, 3) at stage 3, (6, 6) otherwise; killed by vk_boss_shot_kill. */
    {
        int x, y, h, l;
        assert(vk_boss_shot_target(&b, 0, &x, &y, &h, &l) && h == 6 && l == 6);
        b.kind = 0;
        assert(vk_boss_shot_target(&b, 0, &x, &y, &h, &l) && h == 6 && l == 3);
        vk_boss_shot_kill(&b, 0);
        assert(!vk_boss_shot_target(&b, 0, &x, &y, &h, &l));
    }
}

int main(void)
{
    VKBoss b;
    VKBossSprite s[VK_BOSS_MAX_SPRITES];
    int i, events, frames;
    assert(vk_boss_door(3, 4) && vk_boss_room(3, 5) && !vk_boss_door(5, 4) && vk_boss_door(6, 4) && vk_boss_room(6, 5));
    assert(vk_boss_entry_x(3) == 10 && vk_boss_entry_y(3) == 176 && vk_boss_entry_x(6) == 246 && vk_boss_entry_y(6) < 0);
    /* BF32: wait = 40h - (R >> 2); R is forced here (Enemies_s7R), the free generator is tested below. */
    Enemies_s7R = 0x7C;
    vk_boss_begin(&b);
    /* Trace O: Y 40h, X 70h, 60 passes at rest, then the aim of Simon at (10, 192). */
    passes(&b, 1, 10, 192);
    assert(b.alive && (b.y >> 8) == 0x40 && (b.x >> 8) == 0x70 && !b.moving);
    passes(&b, 61, 10, 192);
    assert(b.state == 3 && b.tgt_y == 0x90 && b.tgt_x == 0xC2);
    assert(b.vy == 0x2D4 && b.vx == 0x2C4);
    /* Trace: 12 03 A8 45 88 75, two moves of (2D4h, 2C4h) from (4000h, 7000h). */
    passes(&b, 1, 10, 192);
    assert(b.y == 0x45A8 && b.x == 0x7588);
    /* The aim stops at the target, the wait fires one shot on the way down. */
    while (b.state == 3) passes(&b, 1, 10, 192);
    assert(b.state == 4 && !b.vy && !b.vx && (b.y >> 8) >= 0x88);
    assert(b.wait == 0x40 - 0x1F);                    /* R = 7Ch: 21h passes */
    while (b.wait != 0x18) passes(&b, 1, 10, 192);
    passes(&b, 1, 10, 192);
    assert(b.shot[0].active);
    assert(vk_boss_sprites(&b, s) >= 4);
    /* Damage: 8 with the whip, 12 with the chain, from 80h. */
    assert(!vk_boss_hit(&b, 0) && b.energy == 0x78);
    assert(!vk_boss_hit(&b, 1) && b.energy == 0x6C);
    /* Contact 83EA and the whip box 83F2. */
    b.y = 0x80 << 8;
    b.x = 0x80 << 8;
    assert(vk_boss_touches(&b, 0x80, 0x80 + 16, 0));
    assert(!vk_boss_touches(&b, 0x80 + 17, 0x80 + 16, 0));
    assert(vk_boss_whip_touches(&b, 0x80 - 20, 0x80 + 12, 0, 0, 0));
    assert(!vk_boss_whip_touches(&b, 0x80 - 45, 0x80 + 12, 0, 0, 0));
    /* Defeat and the script: 60 passes, the orb, 90 passes, the fall to C0h, 180 passes. */
    b.energy = 8;
    assert(vk_boss_hit(&b, 0) && !b.alive && b.flash_wait == 0x14 && !b.shot[0].active);
    assert(b.flame[0] == 16 && !b.flame[1]);      /* 7806 -> 9A3A: 1E flame, 16 passes */
    frames = 0;
    do { events = vk_boss_frame(&b, 10, 192, 0, 0, 32); frames++; } while (!(events & VK_BOSS_EV_ORB_TILES));
    assert(frames >= 120 && frames <= 130);       /* trace: 1366 -> 1492 */
    assert(!b.flame[0]);
    assert(b.orb && b.orb_y == 0x40);
    while (!b.orb_landed) vk_boss_frame(&b, 10, 192, 0, 0, 32);
    assert(b.orb_y == 0xC0);
    assert(!vk_boss_orb_touch(&b, 10, 192, 0));
    assert(vk_boss_orb_touch(&b, 120, 192, 0));
    /* Taken: 150 passes, then the energy comes back, then 60 passes and the exit. */
    while (b.script != 5) vk_boss_frame(&b, 120, 192, 0, 0, 10);
    for (i = 0; i < 4; i++) events |= vk_boss_frame(&b, 120, 192, 0, 0, 10);
    assert(events & VK_BOSS_EV_HEAL);
    frames = 0;
    events = 0;
    while (!events) { events = vk_boss_frame(&b, 120, 192, 0, 0, 32) & VK_BOSS_EV_EXIT; frames++; }
    assert(frames >= 118 && frames <= 126);       /* trace: 2106 -> 2227 */
    /* Too late: script 6 without healing, trace D: 2285 -> 2573 (90h passes). */
    vk_boss_begin(&b);
    passes(&b, 2, 10, 192);
    b.energy = 8;
    vk_boss_hit(&b, 0);
    while (!b.orb_landed) vk_boss_frame(&b, 10, 192, 0, 0, 20);
    while (b.script != 6) vk_boss_frame(&b, 10, 192, 0, 0, 20);
    assert(b.script_wait == 0x90);
    /* Stage 6, object 13h, trace A (Simon still at (246, 160), no hit): CE01 waits 78h passes, the object is made
     * at (90h, 90h) and runs in the same pass (frame 1230 = object pass 1). */
    vk_boss_begin_stage(&b, 6);
    passes(&b, 121, 246, 160);
    assert(!b.alive && b.room_step == 2);
    events = vk_boss_frame(&b, 246, 160, 0, 0, 32);
    events |= vk_boss_frame(&b, 246, 160, 0, 0, 32);
    assert(events & VK_BOSS_EV_ERASE);
    assert(b.alive && b.y == 0x9030 && b.x == 0x90F8 && b.vy == 0x30 && b.p15 == 0x3B && b.pose == 0x2B);
    passes(&b, 1, 246, 160);
    assert(b.y == 0x9080 && b.x == 0x91F0 && b.vy == 0x50);        /* frame 1232 */
    passes(&b, 58, 246, 160);                                       /* pass 60: the first fireball, frame 1348 */
    assert(b.shot[0].active && b.shot[0].y == 0x82C8 && b.shot[0].x == 0xCCE8 && b.shot[0].vy == 200 && b.shot[0].vx == 1000);
    assert(b.p15 == 0x3C);
    /* Fireball of class 1 (830D[5]): box (6, 6), contact when |dX| < 11 (trace 847A: HL = 0606; hits at |dX| 8 and 9, none at 11). */
    b.shot[0].x = 235 << 8; b.shot[0].y = 150 << 8;
    assert(vk_boss_shot_touches(&b, 246, 160, 0) < 0);
    b.shot[0].x = 236 << 8;
    assert(vk_boss_shot_touches(&b, 246, 160, 0) == 0);
    b.shot[0].x = 0xCCE8; b.shot[0].y = 0x82C8;
    passes(&b, 40, 246, 160);                                       /* pass 100: within 8 of Simon, frame 1428 */
    assert(b.state == 1 && b.y == 0xB1C0 && b.x == 0xEEF0 && b.vy == -0x30 && b.vx == -0xF8 && b.p12 == 0x3F && b.p13 == 0x60);
    passes(&b, 96, 246, 160);                                       /* pass 196: 60h passes later it stops, frame 1620 */
    assert(!b.moving && b.y == 0x9030 && b.x == 0x92E8 && b.p12 == 0x3E && b.p13 == 1);
    passes(&b, 62, 246, 160);                                       /* pass 258: it goes again, frame 1744 */
    assert(b.state == 0 && b.moving && b.y == 0x8FE0 && b.x == 0x93E0 && b.vy == -0x50 && b.vx == 0xF8 && b.p12 == 0x2F && b.p13 == 0x4F);
    /* Defeat: same script, the orb falls to B0h (trace D: Y B0h at the landing, frame 2424). */
    b.energy = 8;
    assert(vk_boss_hit(&b, 0) && !b.alive);
    frames = 0;
    do { events = vk_boss_frame(&b, 108, 176, 0, 0, 32); frames++; } while (!(events & VK_BOSS_EV_ORB_TILES));
    assert(frames >= 120 && frames <= 130);                         /* trace D: 1897 -> 2021 */
    frames = 0;
    while (!b.orb_landed) { vk_boss_frame(&b, 108, 176, 0, 0, 32); frames++; }
    assert(frames >= 400 && frames <= 406);                         /* trace D: 2021 -> 2424 */
    assert(b.orb_y == 0xB0 && !b.shot[0].active);
    assert(vk_boss_orb_touch(&b, 108, 176, 0));
    boxes();
    puts("boss: PASS");
    return 0;
}
