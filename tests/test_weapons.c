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
    return vk_shot_touches(&((const VKWeapons *)ctx)->shot[slot], x, y, h, l);
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
        w.shot[1] = (VKShot){ 2, 2, 0, 5, 176, 89, 0x20, 0, 0 };
        assert(!Enemies_shotHit(&es, touch, &w, &slot, &score));  /* |89 - 100| = 11, not < 5 + 6 */
        w.shot[1].x = 90;
        assert(Enemies_shotHit(&es, touch, &w, &slot, &score) && slot == 1 && score == 1);
        assert(!es.enemy[0].active && es.enemy[1].active);
        vk_weapons_enemy_hit(&w, slot);
        assert(!w.shot[1].type);
    }
    puts("weapons: PASS");
    return 0;
}
