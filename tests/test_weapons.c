/* Native tests of weapons.c: the thrown weapons of the MSX2 (research/secondary-
 * weapons.md). The paths are those of the openMSX trace scratchpad/sw/A (hall room 1,
 * tools/run_weapon_trace.py, tools/analyze_weapon_trace.py). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "weapons.h"
#include "enemies.h"

static VKByte map[768];

/* One pass (two frames of vk_weapons_frame). */
static void pass(VKWeapons *w, const VKWeaponSimon *s, VKByte weapon, VKWeaponEvents *ev)
{
    while (!vk_weapons_frame(w, s, weapon, map, 1, ev)) {}
}

static void throw_one(VKWeapons *w, const VKWeaponSimon *s, VKByte weapon, VKWeaponEvents *ev)
{
    assert(vk_weapons_reserve(w, weapon));
    vk_weapons_launch(w, weapon);
    pass(w, s, weapon, ev);          /* 7286 then 72B2 in the same pass */
}

static int has_sound(const VKWeaponEvents *ev, VKByte id)
{
    int i;
    for (i = 0; i < ev->sounds; i++) if (ev->sound[i] == id) return 1;
    return 0;
}

static int touch(const void *ctx, int slot, int x, int y, int h, int l)
{
    return vk_shot_touches_slot((const VKWeapons *)ctx, slot, x, y, h, l);
}

int main(void)
{
    VKWeapons w;
    VKWeaponEvents ev;
    VKWeaponSimon simon = { 60, 192, 0, 0 };
    int i, n;
    for (i = 22 * 32; i < 768; i++) map[i] = 1;     /* floor from Y 192 (row 22) */

    /* Dagger (C416 = 2, 7436): two slots (71BE), a third press is ignored. Speed 5,
     * Y = C425 - 16, pattern 20 to the right, effect 04, out at X FB..04 (74CE). Trace:
     * X 65 at the launch pass, 70, ..., 250, then gone (38 passes, 76 frames). */
    vk_weapons_reset(&w);
    throw_one(&w, &simon, 2, &ev);
    throw_one(&w, &simon, 2, &ev);
    assert(w.shot[0].type == 2 && w.shot[1].type == 2 && !vk_weapons_reserve(&w, 2));
    vk_weapons_reset(&w);
    throw_one(&w, &simon, 2, &ev);
    assert(w.shot[0].type == 2 && w.shot[0].x == 65 && w.shot[0].y == 176 && w.shot[0].pattern == 0x20);
    assert(has_sound(&ev, 0x04) && vk_weapons_frame_index(&w.shot[0]) == 1);
    for (n = 1; w.shot[0].type; n++) { assert(w.shot[0].x == 65 + 5 * (n - 1)); pass(&w, &simon, 2, &ev); }
    assert(n == 39 && !ev.lost && w.shot[0].y == VK_SHOT_FREE_Y);
    /* To the left from X 56: 51, 46, ..., 6, then gone. */
    simon.x = 56; simon.facing = 1;
    throw_one(&w, &simon, 2, &ev);
    assert(w.shot[0].x == 51 && w.shot[0].pattern == 0x18 && vk_weapons_frame_index(&w.shot[0]) == 0);
    for (n = 0; w.shot[0].type; n++) pass(&w, &simon, 2, &ev);
    assert(n == 10);
    /* Crouched: Y = C425 - 10. */
    simon.state = 2;
    throw_one(&w, &simon, 2, &ev);
    assert(w.shot[0].y == 182);
    vk_weapons_reset(&w);
    simon.state = 0;

    /* Axe (C416 = 3, 7473): one slot, speed 3, 24 passes out, then 23 passes of state 3
     * (dx - 1 every other pass), back at dx -8, caught by Simon (8240). Trace from X 58:
     * 61, 64, ..., 130, 133 (state 3), 135, 137, 138, 139, 139, 139, 138, ..., 75, 67,
     * caught in state 3 after 47 passes (94 frames). Effect 06 when C003 & 7 = 0. */
    {
        static const VKByte back[] = { 135, 137, 138, 139, 139, 139, 138, 137, 135, 133, 130,
                                       127, 123, 119, 114, 109, 103, 97, 90, 83, 75, 67 };
        int sounds = 0;
        simon.x = 58; simon.facing = 0;
        vk_weapons_reset(&w);
        throw_one(&w, &simon, 3, &ev);
        assert(w.shot[0].x == 61 && w.shot[0].state == 2 && !vk_weapons_reserve(&w, 3));
        for (n = 1; n < 24; n++) { pass(&w, &simon, 3, &ev); sounds += has_sound(&ev, 0x06); }
        assert(w.shot[0].x == 130 && w.shot[0].state == 2);
        pass(&w, &simon, 3, &ev);
        assert(w.shot[0].x == 133 && w.shot[0].state == 3);
        for (i = 0; i < (int)sizeof(back); i++) { pass(&w, &simon, 3, &ev); assert(w.shot[0].x == back[i]); }
        assert(w.shot[0].state == 3);                      /* caught in state 3 */
        pass(&w, &simon, 3, &ev);
        assert(!w.shot[0].type && !ev.lost && sounds == 3);
    }
    /* Axe to the left from X 56: 53, ..., 8, then the edge (7359): state 4, dx +5, caught. */
    simon.x = 56; simon.facing = 1;
    throw_one(&w, &simon, 3, &ev);
    assert(w.shot[0].x == 53);
    for (n = 0; w.shot[0].state == 2; n++) pass(&w, &simon, 3, &ev);
    assert(n == 16 && w.shot[0].state == 4 && w.shot[0].x == 13);
    for (n = 0; w.shot[0].type; n++) pass(&w, &simon, 3, &ev);
    assert(n == 8 && !ev.lost);

    /* Cross (C416 = 4, 72DE): speed 5, caught 63 passes (126 frames) after the launch from X 16. */
    simon.x = 16; simon.facing = 0;
    vk_weapons_reset(&w);
    throw_one(&w, &simon, 4, &ev);
    assert(w.shot[0].x == 21);
    for (n = 1; w.shot[0].type; n++) pass(&w, &simon, 4, &ev);
    assert(n == 64 && !ev.lost);
    /* Lost when it leaves the screen (74CE with C416 3 or 4): Simon away from it. */
    simon.x = 16;
    throw_one(&w, &simon, 4, &ev);
    simon.x = 200;                   /* no catch: back to the left edge */
    for (n = 0; w.shot[0].type && n < 200; n++) pass(&w, &simon, 4, &ev);
    assert(!w.shot[0].type && ev.lost);

    /* Holy water (object 1E, 714D): 5 hearts, C460 only, two starts from Simon (73AE),
     * the arc of 707D, flames on the floor (effect 18) for 24 passes. Trace (Simon X 14
     * in a jump, Y 160 then 161): X 16 Y 144, 16/145, 18/139, 20/133, 22/129, ...
     * floor at 74/193, flames until 24 passes later. */
    {
        VKByte hearts = 9;
        static const VKByte ys[] = { 139, 133, 129, 125, 121, 119, 117, 115, 113, 113, 113,
                                     113, 113, 115, 117, 119, 121, 125, 129, 133, 139, 145, 151,
                                     157, 163, 169, 175, 181, 187 };
        vk_weapons_reset(&w);
        simon.x = 14; simon.y = 160; simon.state = 1;
        assert(vk_weapons_water(&w, &hearts, 0) && hearts == 4 && w.shot[1].type == 5);
        assert(!vk_weapons_water(&w, &hearts, 0));               /* 4 hearts, slot busy */
        pass(&w, &simon, 4, &ev);
        assert(w.shot[1].x == 16 && w.shot[1].y == 144 && vk_weapons_frame_index(&w.shot[1]) == 10);
        simon.y = 161;
        pass(&w, &simon, 4, &ev);
        assert(w.shot[1].x == 16 && w.shot[1].y == 145 && w.shot[1].state == 2);
        for (i = 0; i < (int)sizeof(ys); i++)
        {
            pass(&w, &simon, 4, &ev);
            assert(w.shot[1].y == ys[i] && w.shot[1].x == 18 + 2 * i);
        }
        pass(&w, &simon, 4, &ev);
        assert(w.shot[1].state == 3 && w.shot[1].y == 193 && w.shot[1].x == 74 && has_sound(&ev, 0x18));
        for (n = 0; w.shot[1].type; n++) pass(&w, &simon, 4, &ev);
        assert(n == 24);
        /* To the left from X 14: out at the left edge, and C416 4 is lost (74CE). */
        hearts = 5;
        assert(vk_weapons_water(&w, &hearts, 1) && hearts == 0);
        for (n = 0; w.shot[1].type; n++) pass(&w, &simon, 4, &ev);
        assert(ev.lost && n == 6);
    }

    /* Collisions (8500): dagger box H + 6, others H + 12; candle (8460). */
    vk_weapons_reset(&w);
    w.shot[0] = (VKShot){ 2, 5, 0, 2, 145, 60, 0x38, 0, 0 };
    assert(vk_shot_touches_candle(&w, 48, 144, 0) == 0);          /* trace: 3525 */
    assert(vk_shot_touches_candle(&w, 112, 144, 0) < 0);
    w.shot[0].type = 2; w.shot[0].y = 176;
    assert(vk_shot_touches_candle(&w, 48, 144, 0) < 0);           /* standing dagger under the candle */
    vk_weapons_candle_hit(&w, 0, 3);
    assert(!w.shot[0].type && w.block == 3);                      /* dagger gone, C433 = 3 */
    /* An axe in flight drops as object 1C after a block (74A5) and C416 goes. */
    simon.x = 58; simon.y = 192; simon.state = 0; simon.facing = 0;
    vk_weapons_pick(&w);                                          /* C433 = 0 (7FFD) */
    throw_one(&w, &simon, 3, &ev);
    w.block = 3;
    pass(&w, &simon, 3, &ev);
    assert(ev.drop && ev.lost && ev.drop_x == 61 && ev.drop_y == 160 && !w.shot[0].type && !w.block);

    /* Enemies (801E): the first enemy touched, one per pass; type 1 (romBox) at its
     * object X, Y (SAT + 8, + 31): box H 18h, L 5. */
    {
        EnemyState es;
        int slot = -1, score = 0;
        memset(&es, 0, sizeof(es));
        es.enemy[0].active = 1; es.enemy[0].type = 0; es.enemy[0].romBox = 1;
        es.enemy[0].x = 92; es.enemy[0].y = 161;                 /* object X 100, Y 192 */
        es.enemy[1] = es.enemy[0];
        vk_weapons_reset(&w);
        w.shot[1] = (VKShot){ 2, 2, 0, 5, 176, 83, 0x20, 0, 0 };
        assert(!Enemies_shotHit(&es, touch, &w, &slot, &score));  /* |83 - 100| = 17, not < 5 + 12 (C460: the second call of 8500 keeps L + 6) */
        w.shot[1].x = 90;
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && slot == 1 && score == 1);
        assert(es.enemy[0].type == ENEMY_TYPE_DEATH && es.enemy[1].active && es.enemy[1].type == 0);   /* slot 0 holds the death flame */
        vk_weapons_enemy_hit(&w, slot);
        assert(!w.shot[1].type);
    }
    /* openMSX trace (research/weapon-targets.md, stage 8 room 2 and 6, stage 9 room 5): damage, invulnerability,
     * score and shot fate against the objects of stages 7-9. Object X, Y as the MSX (IX+5, IX+3). */
    {
        EnemyState es;
        int slot = -1, score = 0;
        const int types[4] = { ENEMY_TYPE_HOPPER, ENEMY_TYPE_STATUE, ENEMY_TYPE_LEAPER, ENEMY_TYPE_FLEA };
        const int hp[4] = { 4, 8, 1, 1 };
        const int pts[4] = { 2, 3, 1, 2 };            /* 200, 300, 100, 200 (81CE) */
        int k;
        memset(&es, 0, sizeof(es));
        Enemies_init(&es);
        /* 0B (4 hp): dagger 1 (the dagger goes), axe 4 (dead, 200), cross 2 then, re-armed (7356), 2 again. */
        es.enemy[0].active = 1; es.enemy[0].type = ENEMY_TYPE_HOPPER; es.enemy[0].extraPhase = 4;
        es.enemy[0].x = 100; es.enemy[0].y = 144;
        vk_weapons_reset(&w);
        w.shot[0] = (VKShot){ 2, VK_SHOT_DAGGER, 0, 5, 128, 100, 0x20, 0, 0 };
        Enemies_shotPower[0] = 1;
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && slot == 0 && score == 0 && (es.enemy[0].extraPhase & 0x0F) == 3);
        vk_weapons_enemy_hit(&w, slot);
        assert(!w.shot[0].type);                      /* trace 915: hp 4 -> 3, dagger freed */
        w.shot[0] = (VKShot){ 2, VK_SHOT_CROSS, 0, 5, 128, 100, 0x18, 0, 0 };
        Enemies_shotPower[0] = 2;
        Enemies_rearm(&es);
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && (es.enemy[0].extraPhase & 0x0F) == 1);
        vk_weapons_enemy_hit(&w, slot);
        assert(w.shot[0].type == VK_SHOT_CROSS);      /* the cross goes on */
        assert(!Enemies_shotHit(&es, touch, &w, &slot, &score));   /* +E bit 0 clear until 999F (trace 1709-1727) */
        Enemies_rearm(&es);
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && !Enemy_alive(es.enemy[0]) && score == 2);
        /* One shot of each weapon on each object, from full hit points. */
        for (k = 0; k < 4; k++)
        {
            int power;
            for (power = 1; power <= 4; power++)
            {
                if (power == 3) continue;
                memset(&es, 0, sizeof(es));
                es.enemy[0].active = 1; es.enemy[0].type = (EnemyU8)types[k]; es.enemy[0].extraPhase = (EnemyU8)hp[k];
                es.enemy[0].x = 100; es.enemy[0].y = 144;
                w.shot[0] = (VKShot){ 2, VK_SHOT_AXE, 0, 3, 128, 100, 0x18, 0, 0 };
                Enemies_shotPower[0] = (EnemyU8)power;
                assert(Enemies_shotHit(&es, touch, &w, &slot, &score));
                if (power >= hp[k]) assert(!Enemy_alive(es.enemy[0]) && score == pts[k]);   /* axe on 0B/0C/0D, 2 axes on the statue */
                else assert(es.enemy[0].active && score == 0 && (es.enemy[0].extraPhase & 0x0F) == hp[k] - power);
            }
        }
        /* Statue (8 hp): axe 8 -> 4 -> 0, 300 points (trace 3013/3051 ... 3948: sc 09 -> 12). */
        memset(&es, 0, sizeof(es));
        es.enemy[0].active = 1; es.enemy[0].type = ENEMY_TYPE_STATUE; es.enemy[0].extraPhase = 8;
        es.enemy[0].x = 100; es.enemy[0].y = 144;
        w.shot[0] = (VKShot){ 2, VK_SHOT_AXE, 0, 3, 128, 100, 0x18, 0, 0 };
        Enemies_shotPower[0] = 4;
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && (es.enemy[0].extraPhase & 0x0F) == 4);
        Enemies_rearm(&es);
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && !Enemy_alive(es.enemy[0]) && score == 3);
        /* The statue ball and the bone (D700) die at once for 100 points (7F79, 822A), the dagger goes (trace 895, 920). */
        memset(&es, 0, sizeof(es));
        es.enemy[0].active = 1; es.enemy[0].type = ENEMY_TYPE_FIREBALL; es.enemy[0].x = 100; es.enemy[0].y = 128;
        w.shot[0] = (VKShot){ 2, VK_SHOT_DAGGER, 0, 5, 128, 100, 0x20, 0, 0 };
        Enemies_shotPower[0] = 1;
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && score == 1 && !es.enemy[0].active);
        es.enemy[0].active = 1; es.enemy[0].type = ENEMY_TYPE_BONE;
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && score == 1 && !es.enemy[0].active);
        /* Box edges (8500) against the statue (class 1, H 18h, L 5; trace ladders ladA, ladC, ladD with the shot at Y 128):
         * axe and cross (H + 12): E 117 touches, 116 does not; dagger (H + 6): E 124 touches, 120 does not. */
        w.shot[0] = (VKShot){ 2, VK_SHOT_AXE, 0, 3, 128, 144, 0x18, 0, 0 };
        assert(vk_shot_touches(&w.shot[0], 144, 117, 0x18, 5) && !vk_shot_touches(&w.shot[0], 144, 116, 0x18, 5));
        w.shot[0].type = VK_SHOT_CROSS;
        assert(vk_shot_touches(&w.shot[0], 144, 117, 0x18, 5) && !vk_shot_touches(&w.shot[0], 144, 116, 0x18, 5));
        w.shot[0].type = VK_SHOT_DAGGER;
        assert(vk_shot_touches(&w.shot[0], 144, 124, 0x18, 5) && !vk_shot_touches(&w.shot[0], 144, 120, 0x18, 5));
    }
    /* Boss boxes 84F0, re-run in openMSX with forced parameters (research/boss-boxes.md): offsets from the object (X, Y);
     * body 180C |dX| <= 17, ball 0606 |dX| <= 11, ball 0603 |dX| <= 8; vertical k = shot Y - object Y: dagger -24..5 /
     * others -24..11 (body), -6..5 / -6..11 (ball), dagger -6..5 on the ball 0603 as well. */
    {
        static const struct { int h, l, dx, vlo; } box[3] = { { 0x18, 0x0C, 17, -24 }, { 6, 6, 11, -6 }, { 6, 3, 8, -6 } };
        int b, t, v;
        for (b = 0; b < 3; b++)
            for (t = VK_SHOT_DAGGER; t <= VK_SHOT_WATER; t++)
            {
                VKShot s;
                memset(&s, 0, sizeof s);
                s.type = (VKByte)t;
                for (v = -40; v <= 40; v++)
                {
                    s.x = (VKByte)(100 + v); s.y = (VKByte)(120 - box[b].h + 2);
                    assert(vk_shot_touches(&s, 100, 120, box[b].h, box[b].l) == (v >= -box[b].dx && v <= box[b].dx));
                    s.x = 100; s.y = (VKByte)(120 + v);
                    assert(vk_shot_touches(&s, 100, 120, box[b].h, box[b].l) ==
                           (v >= box[b].vlo && v <= (t == VK_SHOT_DAGGER ? 5 : 11)));
                }
            }
    }
    /* Boxes of the objects of stages 7-9 (openMSX trace by re-execution of the ROM routines 8500, 84A7 and 847A with the inputs
     * swept around the real DE of the object: research/enemy-boxes.md). Object at (100, 144); each row is one class of 82CB / 830D:
     * type, H, L, then contact |dX| <= cx and Simon - E windows (standing, jump, crouched), whip |tip - X| <= wx, Simon - E windows
     * (standing, crouched). Edge inside / one past it for every row. */
    {
        static const struct {
            EnemyU8 type; int h, l;
            int cx, stand_lo, jump_lo, crouch_lo;     /* contact: upper edges are 27 (standing, jump) and 19 (crouched) */
            int wx, wstand_lo, wcrouch_lo;            /* whip: upper edges are 21 (standing) and 13 (crouched) */
        } cls[] = {
            { ENEMY_TYPE_STATUE,   0x18, 5, 9, -22, -14, -22, 16, -6, -14 },   /* 0A, 0B: class 1 (8378) */
            { ENEMY_TYPE_8,        0x10, 8, 12, -14, -6, -14, 19, 2, -6 },     /* 08, 0C, 0D: class 2 (8390) */
            { ENEMY_TYPE_FIREBALL, 0x06, 3, 7, -4, 4, -4, 14, 12, 4 },         /* the ball of the statue: 8318 */
            { ENEMY_TYPE_BONE,     0x0C, 6, 10, -10, -2, -10, 17, 6, -2 },     /* the bone of 0B: 8358 */
        };
        const int ex = 100, ey = 144;
        int c, d;
        for (c = 0; c < 4; c++)
        {
            EnemyState es;
            EnemyRoomVars v;
            const int h = cls[c].h, l = cls[c].l;
            memset(&es, 0, sizeof(es));
            memset(&v, 0, sizeof(v));
            es.enemy[0].active = 1; es.enemy[0].type = cls[c].type; es.enemy[0].x = ex; es.enemy[0].y = ey;
            es.enemy[0].extraPhase = 8;
            /* Contact 847A. */
            for (d = 0; d < 3; d++)
            {
                const int lo = d == 0 ? cls[c].stand_lo : d == 1 ? cls[c].jump_lo : cls[c].crouch_lo;
                const int hi = d == 2 ? 19 : 27;
                v.simonState = d == 0 ? ENEMY_SIMON_GROUND : d == 1 ? ENEMY_SIMON_JUMP : ENEMY_SIMON_CROUCH;
                v.simonX = (EnemyU8)(ex - cls[c].cx); v.simonY = (EnemyU8)(ey + lo);
                assert(Enemies_touches(&es.enemy[0], &v));
                v.simonX = (EnemyU8)(ex + cls[c].cx); v.simonY = (EnemyU8)(ey + hi);
                assert(Enemies_touches(&es.enemy[0], &v));
                v.simonX = (EnemyU8)(ex - cls[c].cx - 1); v.simonY = (EnemyU8)(ey + lo);
                assert(!Enemies_touches(&es.enemy[0], &v));
                v.simonX = (EnemyU8)(ex + cls[c].cx + 1);
                assert(!Enemies_touches(&es.enemy[0], &v));
                v.simonX = (EnemyU8)ex; v.simonY = (EnemyU8)(ey + lo - 1);
                assert(!Enemies_touches(&es.enemy[0], &v));
                v.simonY = (EnemyU8)(ey + hi + 1);
                assert(!Enemies_touches(&es.enemy[0], &v));
            }
            /* Whip 84A7, leather (reach 20, L + 12) and chain (reach 24, L + 16), facing right then left, standing then crouched. */
            for (d = 0; d < 4; d++)
            {
                const int chain = d & 1, crouched = d >> 1;
                const int reach = chain ? 24 : 20, wx = cls[c].wx + (chain ? 4 : 0);
                const int lo = crouched ? cls[c].wcrouch_lo : cls[c].wstand_lo, hi = crouched ? 13 : 21;
                int left, hits;
                for (left = 0; left < 2; left++)
                {
                    const int sign = left ? -1 : 1;
                    #define WHIP(sx, sy) (hits = 0, Enemies_whipHitStage4(&es, (sx), (sy), left, crouched, chain, &hits), es.enemy[0].active = 1, \
                                          es.enemy[0].type = cls[c].type, es.enemy[0].x = ex, es.enemy[0].y = ey, es.enemy[0].extraPhase = 8, hits)
                    assert(WHIP(ex - sign * reach - wx, ey + lo) == 1);
                    assert(WHIP(ex - sign * reach + wx, ey + hi) == 1);
                    assert(WHIP(ex - sign * reach - wx - 1, ey + lo) == 0);
                    assert(WHIP(ex - sign * reach + wx + 1, ey + hi) == 0);
                    assert(WHIP(ex - sign * reach, ey + lo - 1) == 0);
                    assert(WHIP(ex - sign * reach, ey + hi + 1) == 0);
                    #undef WHIP
                }
            }
            /* Thrown weapons 8500: slot 0 |dX| < L + 6, dagger window [-H, 5], axe and cross [-H, 11]. */
            {
                int t;
                for (t = 0; t < 2; t++)
                {
                    const VKByte type = t ? VK_SHOT_AXE : VK_SHOT_DAGGER;
                    const int hi = t ? 11 : 5;
                    vk_weapons_reset(&w);
                    w.shot[0] = (VKShot){ 2, type, 0, 3, (VKByte)(ey - h), (VKByte)(ex - l - 5), 0x18, 0, 0 };
                    assert(vk_shot_touches_slot(&w, 0, ex, ey, h, l));
                    w.shot[0].x = (VKByte)(ex + l + 5); w.shot[0].y = (VKByte)(ey + hi);
                    assert(vk_shot_touches_slot(&w, 0, ex, ey, h, l));
                    w.shot[0].x = (VKByte)(ex - l - 6);
                    assert(!vk_shot_touches_slot(&w, 0, ex, ey, h, l));
                    w.shot[0].x = (VKByte)(ex + l + 6);
                    assert(!vk_shot_touches_slot(&w, 0, ex, ey, h, l));
                    w.shot[0].x = (VKByte)ex; w.shot[0].y = (VKByte)(ey - h - 1);
                    assert(!vk_shot_touches_slot(&w, 0, ex, ey, h, l));
                    w.shot[0].y = (VKByte)(ey + hi + 1);
                    assert(!vk_shot_touches_slot(&w, 0, ex, ey, h, l));
                    /* Slot 1 alone (slot 0 free: X 0, Y E0): 8500 leaves L + 6 in HL for the second call: |dX| < L + 12 (the statue:
                     * a dagger in C460 touches at dX -16 and +16 where C450 stops at +-10; trace 1280/1290 and the sweeps). */
                    vk_weapons_reset(&w);
                    w.shot[1] = (VKShot){ 2, type, 0, 3, (VKByte)(ey + hi), (VKByte)(ex - l - 11), 0x18, 0, 0 };
                    assert(vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    w.shot[1].x = (VKByte)(ex + l + 11); w.shot[1].y = (VKByte)(ey - h);
                    assert(vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    w.shot[1].x = (VKByte)(ex - l - 12);
                    assert(!vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    w.shot[1].x = (VKByte)(ex + l + 12);
                    assert(!vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    /* Behind a dagger (H + 6) or an axe (H + 12) of C450 that passes the X test and misses in Y, the window of
                     * C460 starts that much higher (H changed by the first call: the 30 and 36 seen in the trace). */
                    w.shot[0] = (VKShot){ 2, VK_SHOT_DAGGER, 0, 3, (VKByte)(ey - h - 20), (VKByte)ex, 0x18, 0, 0 };
                    w.shot[1] = (VKShot){ 2, type, 0, 3, (VKByte)(ey - h - 6), (VKByte)ex, 0x18, 0, 0 };
                    assert(vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    w.shot[1].y = (VKByte)(ey - h - 7);
                    assert(!vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    w.shot[0].type = VK_SHOT_AXE;
                    w.shot[1].y = (VKByte)(ey - h - 12);
                    assert(vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    w.shot[1].y = (VKByte)(ey - h - 13);
                    assert(!vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                    /* Slot 0 outside the X window leaves H alone. */
                    w.shot[0].x = (VKByte)(ex + l + 6);
                    w.shot[1].y = (VKByte)(ey - h - 1);
                    assert(!vk_shot_touches_slot(&w, 1, ex, ey, h, l));
                }
            }
        }
        /* Top edge of the statue: the dagger at Y 128 touches at E 152 and not at 153, whatever is in the way (real chain, openMSX: E 146,
         * 147, 148 and 152 all touch with the leaper removed; the old 147 / 148 limit came from another object stopping the dagger). */
        w.shot[0] = (VKShot){ 2, VK_SHOT_DAGGER, 0, 5, 128, 144, 0x20, 0, 0 };
        assert(vk_shot_touches(&w.shot[0], 144, 152, 0x18, 5) && !vk_shot_touches(&w.shot[0], 144, 153, 0x18, 5));
        assert(vk_shot_touches(&w.shot[0], 144, 147, 0x18, 5) && vk_shot_touches(&w.shot[0], 144, 148, 0x18, 5));
    }
    /* Object 08 isolated (openMSX, stage 9 room 1, pinned at (130, 105)): 1 hit point, dies to one dagger, axe or cross for 200 points. */
    {
        EnemyState es;
        int slot = -1, score = 0, k;
        for (k = 0; k < 3; k++)
        {
            const VKByte type = k == 0 ? VK_SHOT_DAGGER : k == 1 ? VK_SHOT_AXE : VK_SHOT_CROSS;
            memset(&es, 0, sizeof(es));
            Enemies_init(&es);
            es.enemy[0].active = 1; es.enemy[0].type = ENEMY_TYPE_8; es.enemy[0].extraPhase = 1;
            es.enemy[0].x = 130; es.enemy[0].y = 105;
            vk_weapons_reset(&w);
            w.shot[0] = (VKShot){ 2, type, 0, 3, 96, 120, 0x18, 0, 0 };     /* trace: Y 96, X 120 / 117 */
            Enemies_shotPower[0] = k == 0 ? 1 : k == 1 ? 4 : 2;
            assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && slot == 0 && score == 2 && !Enemy_alive(es.enemy[0]));
        }
    }
    puts("weapons: PASS");
    return 0;
}
