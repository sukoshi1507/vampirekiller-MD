/* Native tests of candles.c: whip touch, drops and pickups of the ROM rules
 * (research/castle-candles.md, section 9). */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "candles.h"
#include "generated/stage1_collision.h"

/* Offsets read by tests/md_scenarios/i_courtyard_torch_drop.txt and
 * p_gold_key_chest.txt (same layout on m68k). */
_Static_assert(offsetof(VKCandles, lit) == 4 && offsetof(VKCandles, drop) == 6 &&
               sizeof(VKDrop) == 14 && offsetof(VKCandles, hearts) == 118 &&
               offsetof(VKCandles, gold_keys) == 119 && offsetof(VKCandles, hud_key) == 319,
               "VKCandles layout");

/* Courtyard tables of the intro generator, not linked in native tests. */
#define ROW8 1, 1, 1, 1, 1, 1, 1, 1
#define ROW32 ROW8, ROW8, ROW8, ROW8
/* Solid from Y 176 (tile row 20) down: the falling objects land on it (7B98 reads the map; 99E5 removes what falls out of the room). */
#define FLOOR_ROWS [640] = ROW32, ROW32, ROW32, ROW32
const VKByte vk_courtyard_maps[3][768] = {{FLOOR_ROWS}, {FLOOR_ROWS}, {FLOOR_ROWS}};
const VKByte vk_courtyard_neighbors[3][4] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

static void place(VKGame *g, VKArea area, VKByte stage, VKByte room, VKByte x, VKByte y)
{
    vk_game_begin(g);
    g->area = area;
    g->stage = stage;
    g->room = room;
    g->player.x = x;
    g->player.y = y;
    g->player.facing = 0;
}

/* Whip on its test frame (C422 = 3, vk_whip_active) from Simon's place. */
static void lash(VKWhip *w, const VKGame *g, int facing)
{
    vk_whip_reset(w);
    w->timer = VK_WHIP_STRIKE_TIMER;
    w->facing = (VKByte)facing;
    w->x = g->player.x;
    w->y = g->player.y;
    w->crouched = 0;
}

static int run(VKCandles *c, VKGame *g, VKWhip *w, int frames)
{
    int events = 0, hit, i;
    for (i = 0; i < frames; i++) events |= vk_candles_tick(c, g, w, &hit);
    return events;
}

static const int kHearts[][8] = {
#include "death_heart_trace.inc"
};
static const int kFlames26[][8] = {
#include "death_flame26_trace.inc"
};

int main(void)
{
    VKCandles c;
    VKGame g;
    VKWhip w;
    int hit, i;

    /* Courtyard room 0, torch A (64,144): Simon at x 44, y 176 facing right hits it
     * (openMSX drops run, image 796): whip X 64, |64 - 72| < 20, 176 - 18 - 144 = 14. */
    vk_candles_begin(&c);
    place(&g, VK_AREA_COURTYARD, 1, 0, 44, 176);
    vk_candles_enter(&c, &g);
    assert(c.count == 2 && vk_candles_lit(&c, 0) && vk_candles_lit(&c, 1));
    lash(&w, &g, 0);
    assert(vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0));
    assert(!vk_candle_whip_touch(vk_candles_def(&c, 1), &w, 0));
    w.timer = VK_WHIP_STRIKE_TIMER + 1;           /* C422 = 2: no test */
    assert(!vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0));
    w.timer = VK_WHIP_STRIKE_TIMER - 1;           /* C422 = 4: no test */
    assert(!vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0));
    lash(&w, &g, 1);                              /* facing left: whip X 24 */
    assert(!vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0));
    /* Stroke in a jump at x 44 (openMSX, scratchpad/whip-jump/run3): the test uses
     * Simon's Y of the pass after the launch. Launched at Y 164 going up, tested at
     * 159 (159 - 18 - 144 < 0): no touch; launched at 159 coming down, tested at
     * 164: the torch goes out. Jump steps on the frames 2 and 4 after the press. */
    {
        static const VKByte up[4] = {164, 164, 159, 159}, down[4] = {159, 159, 164, 164};
        const VKByte *const ys[2] = {up, down};
        int k, f;
        for (k = 0; k < 2; k++)
        {
            VKPlayer p = g.player;
            vk_whip_reset(&w);
            p.y = ys[k][0];
            vk_whip_tick(&w, &p, 1);
            for (f = 1; f < 4; f++)
            {
                p.y = ys[k][f];
                vk_whip_tick(&w, &p, 0);
                assert(vk_whip_active(&w) == (f == 3));
            }
            assert(vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0) == k);
        }
    }

    /* The hit puts it out, a small heart bursts 33 frames (flame 1E, trace) then sways down. */
    lash(&w, &g, 0);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_HIT);
    assert(hit == 0 && !vk_candles_lit(&c, 0) && c.drop[0].state == VK_DROP_BURST);
    assert(c.drop[0].kind == VK_OBJ_SMALL_HEART);
    vk_whip_reset(&w);
    run(&c, &g, &w, 31);
    assert(c.drop[0].state == VK_DROP_BURST);
    run(&c, &g, &w, 1);
    assert(c.drop[0].state == VK_DROP_FALL);
    for (i = 0; i < 400 && c.drop[0].state == VK_DROP_FALL; i++) vk_candles_tick(&c, &g, &w, &hit);
    assert(c.drop[0].state == VK_DROP_REST);
    assert((c.drop[0].x & 7) == 0 && (c.drop[0].y & 7) == 0);
    /* Picked up by walking onto it: hearts + 1. */
    g.player.x = (VKByte)(c.drop[0].x + 8);
    g.player.y = (VKByte)(c.drop[0].y + 2 + 20);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_HEARTS);
    assert(c.hearts == 1 && c.drop[0].state == VK_DROP_NONE);

    /* 7FE2: the heart sways down as an object caught in the air. */
    c.drop[0].state = VK_DROP_BURST;
    c.drop[0].timer = 0;
    c.drop[0].x = 64;
    c.drop[0].y = 144;
    g.player.x = 100;
    run(&c, &g, &w, 33);
    assert(c.drop[0].state == VK_DROP_FALL && c.drop[0].catchable);
    g.player.x = (VKByte)(c.drop[0].x + 8);
    g.player.y = (VKByte)(c.drop[0].y + 2 + 20);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_HEARTS);
    assert(c.hearts == 2 && c.drop[0].state == VK_DROP_NONE);
    c.hearts = 1;

    /* A torch put out stays out when Simon comes back. */
    g.room = 1;
    vk_candles_enter(&c, &g);
    g.room = 0;
    vk_candles_enter(&c, &g);
    assert(!vk_candles_lit(&c, 0) && vk_candles_lit(&c, 1));

    /* Courtyard room 2, torch 0x1A: chain whip (C416 = 1) after the pickup; a
     * second chain whip would come out as a small heart (8A29). */
    vk_candles_begin(&c);
    place(&g, VK_AREA_COURTYARD, 1, 2, 44, 176);
    lash(&w, &g, 0);
    vk_candles_tick(&c, &g, &w, &hit);
    assert(c.drop[0].kind == VK_OBJ_CHAIN_WHIP);
    vk_whip_reset(&w);
    for (i = 0; i < 100 && c.drop[0].state != VK_DROP_REST; i++) vk_candles_tick(&c, &g, &w, &hit);
    assert(c.drop[0].state == VK_DROP_REST);
    g.player.x = (VKByte)(c.drop[0].x + 8);
    g.player.y = (VKByte)(c.drop[0].y + 22);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_WEAPON);
    assert(c.weapon == 1);
    c.lit = 1;
    assert(vk_candles_hit(&c, 0) && c.drop[0].kind == VK_OBJ_SMALL_HEART);

    /* An object left on the floor goes after 8 x 32 frames (trace: -1 every 16 passes). */
    vk_candles_begin(&c);
    place(&g, VK_AREA_COURTYARD, 1, 1, 10, 176);
    vk_candles_enter(&c, &g);
    vk_candles_hit(&c, 0);
    vk_whip_reset(&w);
    for (i = 0; i < 100 && c.drop[0].state != VK_DROP_REST; i++) vk_candles_tick(&c, &g, &w, &hit);
    assert(c.drop[0].kind == VK_OBJ_BIG_HEART && c.drop[0].state == VK_DROP_REST);
    run(&c, &g, &w, 8 * 32 + 32);
    assert(c.drop[0].state == VK_DROP_NONE);

    /* Hall room 2: the hidden block (128,160) holds the white key. Simon on the
     * floor (y 192) at x 166 facing left: whip X 146, |146 - 136| < 20. */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 1, 2, 166, 192);
    vk_whip_reset(&w);
    vk_candles_enter(&c, &g);
    /* 86CA: the block is in the room map, solid (brick tiles 01/02/09/0A/0B). */
    assert(vk_sample_tile(vk_game_collision(&g), 128, 160) == 0x01);
    assert(vk_sample_tile(vk_game_collision(&g), 152, 184) == 0x09);
    assert(vk_tile_solid(vk_sample_tile(vk_game_collision(&g), 136, 176), 1, 0));
    lash(&w, &g, 1);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_WHITE_KEY);
    /* 8899: broken, the ROM tiles are back. */
    assert(vk_sample_tile(vk_game_collision(&g), 128, 160) == vk_stage1_collision[2][(160 - 16) / 8 * 32 + 16]);
    assert(c.drop[2].state == VK_DROP_NONE);
    /* 883E -> 89FD: the stream flag goes to the key; leaving the room without it,
     * the block is back (flag still set, 5B1B). */
    assert(c.out[1][2] == 0);
    place(&g, VK_AREA_INTERIOR, 1, 3, 16, 192);
    vk_candles_enter(&c, &g);
    place(&g, VK_AREA_INTERIOR, 1, 2, 166, 192);
    vk_candles_enter(&c, &g);
    assert(vk_candles_lit(&c, 1) && vk_candles_def(&c, 1)->kind == VK_OBJ_WHITE_KEY);
    assert(vk_sample_tile(vk_game_collision(&g), 128, 160) == 0x01);
    /* 8C0B: once the key is taken, the block does not come back. */
    vk_whip_reset(&w);
    lash(&w, &g, 1);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_WHITE_KEY);
    vk_candles_white_key_taken(&c);
    assert(c.out[1][2] == 2);
    place(&g, VK_AREA_INTERIOR, 1, 3, 16, 192);
    vk_candles_enter(&c, &g);
    place(&g, VK_AREA_INTERIOR, 1, 2, 166, 192);
    vk_candles_enter(&c, &g);
    assert(!vk_candles_lit(&c, 1));

    /* Hall room 5: candle (64,144) holds object 0 (C470: 01 90 40 00 00 00): the
     * red flame of 8C7E for 32 frames (8A99, trace), then nothing (8B1B); candle (144,128)
     * holds 1A, the chain whip. */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 1, 5, 50, 192);
    vk_whip_reset(&w);
    vk_candles_enter(&c, &g);
    assert(c.count == 2 && vk_candles_def(&c, 0)->kind == 0 && vk_candles_def(&c, 1)->kind == 0x1A);
    assert(vk_candles_hit(&c, 0) == VK_CANDLE_EVENT_HIT);
    assert(c.drop[0].state == VK_DROP_BURST && c.drop[0].kind == 0);
    run(&c, &g, &w, 31);
    assert(c.drop[0].state == VK_DROP_BURST);
    run(&c, &g, &w, 1);
    assert(c.drop[0].state == VK_DROP_NONE && c.hearts == 0 && c.weapon == 0);
    assert(!(c.drop[0].content == VK_OBJ_NOTHING));
    vk_candles_hit(&c, 1);
    assert(c.drop[0].state == VK_DROP_BURST && c.drop[0].kind == VK_OBJ_CHAIN_WHIP);

    /* Hall room 2: gold key (64,176) then chest (176,176) with a big heart. */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 1, 2, 160, 192);
    vk_whip_reset(&w);
    vk_candles_enter(&c, &g);
    assert(c.drop[0].state == VK_DROP_REST && c.drop[0].kind == VK_OBJ_GOLD_KEY);
    assert(c.drop[1].state == VK_DROP_REST && c.drop[1].kind == VK_OBJ_CHEST);
    g.player.x = 184;                              /* chest without the key: stays */
    assert(vk_candles_tick(&c, &g, &w, &hit) == VK_CANDLE_EVENT_NONE);
    assert(c.drop[1].state == VK_DROP_REST);
    /* 8F83: no second gold key while one is held (C701 bit 1). */
    c.gold_keys = 1;
    g.player.x = 72;
    assert(vk_candles_tick(&c, &g, &w, &hit) == VK_CANDLE_EVENT_NONE);
    assert(c.drop[0].state == VK_DROP_REST && c.hud_key == 0);
    c.gold_keys = 0;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_GOLD_KEY);
    assert(c.gold_keys == 1 && c.drop[0].state == VK_DROP_NONE);
    assert(c.hud_key == VK_OBJ_GOLD_KEY);          /* 8E50 -> 8EC9: HUD key at (148, 12) */
    run(&c, &g, &w, 200);                          /* the key does not time out */
    g.player.x = 184;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_GOLD_KEY);
    assert(c.gold_keys == 0 && c.drop[1].kind == VK_OBJ_BIG_HEART);
    assert(c.hud_key == 0);                        /* 8C23: last key used, HUD key cleared */
    /* 8C2F/8BCC: the content waits 32 passes (64 frames, traced) 8 px above the chest, then falls. */
    assert(c.drop[1].state == VK_DROP_WAIT && c.drop[1].y == 168);
    g.player.x = 100;
    run(&c, &g, &w, 63);
    assert(c.drop[1].state == VK_DROP_WAIT);
    run(&c, &g, &w, 1);
    assert(c.drop[1].state == VK_DROP_FALL);
    /* Type 02 while it falls (bit 7 clear): Simon under it does not take it. */
    g.player.x = 184;
    for (i = 0; i < 20 && c.drop[1].y < 176; i++)
    {
        vk_candles_tick(&c, &g, &w, &hit);
        assert(c.hearts == 0);
    }
    g.player.x = 100;
    run(&c, &g, &w, 4);
    assert(c.drop[1].state == VK_DROP_FALL);     /* 03 and 04 (8B70): 9 frames after 02, taken on the 9th */
    run(&c, &g, &w, 1);
    assert(c.drop[1].state == VK_DROP_REST && c.drop[1].y == 176);
    g.player.x = 184;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_HEARTS);
    assert(c.hearts == 5);
    /* Taken for good: back in the room, neither key nor chest. */
    g.room = 1;
    vk_candles_enter(&c, &g);
    g.room = 2;
    vk_candles_enter(&c, &g);
    for (i = 0; i < VK_DROP_MAX; i++) assert(c.drop[i].state == VK_DROP_NONE);

    /* New life (state 4, 62E6/70DC): the stage comes back, weapon and gold keys go,
     * the hearts stay. */
    c.weapon = 1;
    c.gold_keys = 1;
    vk_candles_new_life(&c, 1);
    assert(c.weapon == 0 && c.gold_keys == 0 && c.hearts == 5);
    vk_candles_enter(&c, &g);
    assert(c.drop[0].kind == VK_OBJ_GOLD_KEY && c.drop[1].kind == VK_OBJ_CHEST);
    assert(c.lit == (1u << c.count) - 1);

    /* 8DF5-8E03: objects 0B, 0C, 0D set C431 bits 2, 3, 4 (effect 12); 70E8 clears it. */
    c.drop[0].state = VK_DROP_REST;
    c.drop[0].kind = 0x0C;
    c.drop[0].fixed = 0;
    c.drop[0].timer = 0xFF;
    c.drop[0].x = 64;
    c.drop[0].y = 176;
    g.player.x = 72;
    g.player.y = 176 + 2 + 20;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_ITEM);
    assert(c.c431 == 0x08 && c.drop[0].state == VK_DROP_NONE);
    assert(c.picked == 0x0C && vk_candles_sound(c.picked) == 0x12);
    vk_candles_death(&c);
    assert(c.c431 == 0);

    /* 8D30: C419 = the object; 03, 05-0A and 16 have their effect in enemies.c
     * (VK_CANDLE_EVENT_PICK), and so does a whip for C440 (8D70 -> 8D7C). The
     * handler's sound: 1B for 05, 16 for 08, 12 for the others. 0A and 0F set
     * their C701 bit here (VK_CANDLE_EVENT_BOX_ITEM, 0A also arms enemies.c); 10, 11 are taken without effect. */
    {
        static const struct { VKByte kind; int event; VKByte sound; } kPick[] = {
            { 0x03, VK_CANDLE_EVENT_PICK, 0x12 }, { 0x05, VK_CANDLE_EVENT_PICK, 0x1B },
            { 0x06, VK_CANDLE_EVENT_PICK, 0x12 }, { 0x07, VK_CANDLE_EVENT_PICK, 0x12 },
            { 0x08, VK_CANDLE_EVENT_PICK, 0x16 }, { 0x09, VK_CANDLE_EVENT_PICK, 0x12 },
            { 0x16, VK_CANDLE_EVENT_PICK, 0x12 },
            { 0x1C, VK_CANDLE_EVENT_PICK | VK_CANDLE_EVENT_WEAPON, 0x12 },
            { 0x0A, VK_CANDLE_EVENT_BOX_ITEM, 0x12 }, { 0x0F, VK_CANDLE_EVENT_BOX_ITEM, 0x12 },
            { 0x10, 0, 0x12 }, { 0x11, 0, 0x12 } };
        unsigned n;
        for (n = 0; n < sizeof kPick / sizeof kPick[0]; n++)
        {
            int events;
            c.drop[0].state = VK_DROP_REST;
            c.drop[0].kind = kPick[n].kind;
            c.drop[0].fixed = 0;
            c.drop[0].timer = 0xFF;
            c.drop[0].x = 64;
            c.drop[0].y = 176;
            events = vk_candles_tick(&c, &g, &w, &hit);
            assert(c.drop[0].state == VK_DROP_NONE && c.picked == kPick[n].kind);
            assert((events & (VK_CANDLE_EVENT_PICK | VK_CANDLE_EVENT_WEAPON |
                              VK_CANDLE_EVENT_BOX_ITEM)) == kPick[n].event);
            assert(vk_candles_sound(c.picked) == kPick[n].sound);
        }
        c.weapon = 0;
        assert(vk_candles_sound(VK_OBJ_SMALL_HEART) == 0x0F && vk_candles_sound(VK_OBJ_NOTHING) == 0 &&
               vk_candles_sound(VK_OBJ_GOLD_KEY) == 0x14 && vk_candles_sound(VK_OBJ_CHEST) == 0);
    }

    /* HUD item box (8EE6, 8F23, 8F55). After the loop above C701 holds 03 (bit 4),
     * 0A (bit 6) and 0F (bit 7); the last object taken (11) is alone in the box. */
    assert(c.c701 == (VK_ITEM_BIT_03 | VK_ITEM_BIT_0A | VK_ITEM_BIT_0F));
    assert(c.box_pick == 0x11 && c.box_items == 0 && c.box_timer_on);
    {
        int n, ticks = 0;
        VKByte kind;
        /* 8F23: 01 and 17 and up (but 1E) leave the box alone. */
        static const VKByte kQuiet[] = { 0x01, 0x17, 0x1A, 0x1D };
        static const VKByte kShown[] = { 0x02, 0x03, 0x05, 0x16, 0x1E };
        for (n = 0; n < (int)(sizeof kQuiet); n++)
        {
            c.box_pick = 0x11; c.box_timer = 5;
            c.drop[0].state = VK_DROP_REST; c.drop[0].kind = kQuiet[n];
            c.drop[0].fixed = 0; c.drop[0].timer = 0xFF; c.drop[0].x = 64; c.drop[0].y = 176;
            c.gold_keys = 0;
            vk_candles_tick(&c, &g, &w, &hit);
            assert(c.picked == kQuiet[n] && c.box_pick == 0x11);
        }
        c.weapon = 0;
        for (n = 0; n < (int)(sizeof kShown); n++)
        {
            c.drop[0].state = VK_DROP_REST; c.drop[0].kind = kShown[n];
            c.drop[0].fixed = 0; c.drop[0].timer = 0xFF; c.drop[0].x = 64; c.drop[0].y = 176;
            vk_candles_tick(&c, &g, &w, &hit);
            assert(c.box_pick == kShown[n] && c.box_items == 0 && c.box_timer == VK_ITEM_PICK_TICKS);
        }
        /* 1E: C701 bit 3; 04 (8DA1) takes bit 5 from bit 4, 03 the other way. */
        assert(c.c701 == (VK_ITEM_BIT_03 | VK_ITEM_BIT_0A | VK_ITEM_BIT_0F | VK_ITEM_BIT_1E));
        kind = 0x04;
        c.drop[0].state = VK_DROP_REST; c.drop[0].kind = kind;
        c.drop[0].fixed = 0; c.drop[0].timer = 0xFF; c.drop[0].x = 64; c.drop[0].y = 176;
        assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_PICK);
        assert(c.c701 == (VK_ITEM_BIT_04 | VK_ITEM_BIT_0A | VK_ITEM_BIT_0F | VK_ITEM_BIT_1E));
        c.drop[0].state = VK_DROP_REST; c.drop[0].kind = 0x03;
        c.drop[0].fixed = 0; c.drop[0].timer = 0xFF; c.drop[0].x = 64; c.drop[0].y = 176;
        vk_candles_tick(&c, &g, &w, &hit);
        assert(c.c701 == (VK_ITEM_BIT_03 | VK_ITEM_BIT_0A | VK_ITEM_BIT_0F | VK_ITEM_BIT_1E));
        /* 8F55: 32 counted passes of 2 frames (C003 & 3F != 0), then 8EE6 draws
         * C701 bits 7..3. openMSX trace: 33 passes (66 frames) when C003 crosses a
         * multiple of 40h, else 32 (64 frames). */
        while (c.box_pick) { vk_candles_tick(&c, &g, &w, &hit); ticks++; }
        assert(ticks == 2 * VK_ITEM_PICK_TICKS || ticks == 2 * VK_ITEM_PICK_TICKS + 2);
        assert(c.box_items == c.c701 && c.box_timer == 0 && c.box_timer_on);
        {
            /* Same pickup with the skipped pass (C003 = clock / 2 = 40h) inside the 32. */
            const VKByte items = c.c701;
            int skip;
            for (skip = 0; skip < 2; skip++)
            {
                c.clock = (VKByte)(skip ? 0x80 - 8 : 0x80 + 2);
                c.drop[0].state = VK_DROP_REST; c.drop[0].kind = 0x02;
                c.drop[0].fixed = 0; c.drop[0].timer = 0xFF; c.drop[0].x = 64; c.drop[0].y = 176;
                vk_candles_tick(&c, &g, &w, &hit);
                assert(c.box_pick == 0x02 && c.box_items == 0);
                for (ticks = 1; c.box_pick; ticks++)
                {
                    vk_candles_tick(&c, &g, &w, &hit);
                    /* 8F55 only in the passes, 2 frames apart from the pickup. */
                    if (ticks == 1) assert(c.box_timer == VK_ITEM_PICK_TICKS);
                    if (ticks == 2 && !skip) assert(c.box_timer == VK_ITEM_PICK_TICKS - 1);
                }
                assert(ticks - 1 == 2 * VK_ITEM_PICK_TICKS + 2 * skip && c.box_items == items);
            }
            /* C5E5 stays FF: 8EE6 again after 256 counted passes (no change on screen). */
            c.box_items = 0;
            for (ticks = 0; ticks < 2 * 255; ticks++) vk_candles_tick(&c, &g, &w, &hit);
            assert(c.box_items == 0 && c.box_timer_on);
            for (ticks = 0; ticks < 2 * 0x100 && !c.box_items; ticks++) vk_candles_tick(&c, &g, &w, &hit);
            assert(c.box_items == items && c.box_timer == 0);
        }
        /* 819E: the armor used up clears bit 4 and redraws the box. */
        vk_candles_armor_out(&c);
        assert(c.c701 == (VK_ITEM_BIT_0A | VK_ITEM_BIT_0F | VK_ITEM_BIT_1E) && c.box_items == c.c701);
        /* 70DC: the death keeps bit 7 only; the box is drawn again at the next 8EE6. */
        vk_candles_death(&c);
        assert(c.c701 == VK_ITEM_BIT_0F && c.box_items == (VK_ITEM_BIT_0A | VK_ITEM_BIT_0F | VK_ITEM_BIT_1E));
        vk_candles_item_box(&c);
        assert(c.box_items == VK_ITEM_BIT_0F && c.box_pick == 0);
        assert(vk_item_box[0][1] == 0x0F && vk_item_box[3][0] == 0xC8 && vk_item_box[4][1] == 0x1E);
        vk_candles_begin(&c);
        assert(c.c701 == 0 && !c.box_timer_on && !c.box_items);
        g.player.x = 72;
    }

    /* Death (70BF) then HUD redraw (4513): the HUD key stays drawn until the redraw. */
    c.gold_keys = 1;
    vk_candles_hud(&c);
    vk_candles_death(&c);
    assert(c.gold_keys == 0 && c.hud_key == VK_OBJ_GOLD_KEY);
    vk_candles_hud(&c);
    assert(c.hud_key == 0);

    /* Stage 2 room 1: hidden blocks. A gold key block (32,96) puts the key at
     * Y + 16 at once (89E3, type 83: no flame, no fall), for good (+6 FF). */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 2, 1, 200, 64);
    vk_whip_reset(&w);
    vk_candles_enter(&c, &g);
    assert(c.count == 7 && vk_candles_def(&c, 0)->kind == VK_OBJ_GOLD_KEY);
    assert(c.drop[0].state == VK_DROP_REST && c.drop[0].kind == VK_OBJ_CHEST);   /* (16,48) */
    assert(vk_candles_hit(&c, 0) == VK_CANDLE_EVENT_HIT);
    assert(c.drop[1].state == VK_DROP_REST && c.drop[1].kind == VK_OBJ_GOLD_KEY);
    assert(c.drop[1].x == 32 && c.drop[1].y == 112 && c.drop[1].timer == 0xFF);
    /* Block 0x1F (32,160), +9 = 8A: a chest holding 0x0A at Y + 16 (8A13). */
    assert(vk_candles_def(&c, 3)->kind == VK_OBJ_BLOCK_CONTENT && vk_candles_def(&c, 3)->content == 0x8A);
    assert(vk_candles_hit(&c, 3) == VK_CANDLE_EVENT_HIT);
    assert(c.drop[2].state == VK_DROP_REST && c.drop[2].kind == VK_OBJ_CHEST);
    assert(c.drop[2].x == 32 && c.drop[2].y == 176 && c.drop[2].content == 0x0A);
    /* Left unopened: the stream flag stays, the block is back (8814 skips 8874). */
    g.room = 0;
    vk_candles_enter(&c, &g);
    g.room = 1;
    vk_candles_enter(&c, &g);
    assert(vk_candles_lit(&c, 3) && !vk_candles_lit(&c, 0));
    /* Opened with a key: the block does not come back (8C0B). */
    vk_candles_hit(&c, 3);
    c.gold_keys = 1;
    g.player.x = 40;
    g.player.y = 192;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_GOLD_KEY);
    assert(c.gold_keys == 0);
    g.room = 0;
    vk_candles_enter(&c, &g);
    g.room = 1;
    vk_candles_enter(&c, &g);
    assert(!vk_candles_lit(&c, 3));

    /* Stage 2 room 0: a heart block (160,96) gives a big heart at Y + 16 that goes
     * after 8 x 16 frames; the 0x1F block (192,160) with +9 = CE (9179) gives
     * nothing here. */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 2, 0, 16, 96);
    vk_candles_enter(&c, &g);
    vk_candles_hit(&c, 2);
    assert(c.drop[0].state == VK_DROP_REST && c.drop[0].kind == VK_OBJ_BIG_HEART);
    assert(c.drop[0].y == 112 && c.drop[0].timer == 8);
    assert(vk_candles_def(&c, 6)->content == 0xCE);
    vk_candles_hit(&c, 6);
    assert(c.drop[1].state == VK_DROP_NONE);

    /* Stage 9 room 2: candle (48,64) of object 15 (openMSX trace, frames 843-874): the same 16-pass flame as object 0, and when it
     * goes out (8AA7 -> 8AD0) the enemy object is asked for at the candle's place (enemies.c makes it 16 px lower, object 1C). */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 9, 2, 20, 144);
    vk_candles_enter(&c, &g);
    assert(vk_candles_def(&c, 0)->kind == VK_OBJ_NOTHING && vk_candles_def(&c, 0)->x == 48 && vk_candles_def(&c, 0)->y == 64);
    assert(vk_candles_hit(&c, 0) == VK_CANDLE_EVENT_HIT);
    assert(c.drop[0].state == VK_DROP_BURST && c.drop[0].kind == 0);
    assert((run(&c, &g, &w, 31) & VK_CANDLE_EVENT_GHOST) == 0 && c.drop[0].state == VK_DROP_BURST);   /* 32 frames (trace) */
    assert((run(&c, &g, &w, 1) & VK_CANDLE_EVENT_GHOST) && c.drop[0].state == VK_DROP_NONE && c.ghost_x == 48 && c.ghost_y == 64);
    assert((run(&c, &g, &w, 8) & VK_CANDLE_EVENT_GHOST) == 0);   /* once */

    /* Stage 8 room 0: the block (96,112) of object 15 (trace, scratchpad/o1c/b80): the flame stands at Y + 16 = 128 for 32 frames,
     * then the enemy object is asked for there (made at 144). */
    vk_candles_begin(&c);
    place(&g, VK_AREA_INTERIOR, 8, 0, 96, 144);
    vk_candles_enter(&c, &g);
    for (i = 0; i < c.count; i++)
        if (vk_candles_def(&c, i)->x == 96 && vk_candles_def(&c, i)->y == 112) break;
    assert(i < c.count && vk_candles_def(&c, i)->kind == VK_OBJ_NOTHING && vk_candles_def(&c, i)->mode == VK_CANDLE_HIDDEN_BLOCK);
    assert(vk_candles_hit(&c, i) == VK_CANDLE_EVENT_HIT);
    assert(c.drop[0].state == VK_DROP_BURST && c.drop[0].x == 96 && c.drop[0].y == 128 && c.drop[0].kind == 0);
    assert((run(&c, &g, &w, 31) & VK_CANDLE_EVENT_GHOST) == 0);
    assert((run(&c, &g, &w, 1) & VK_CANDLE_EVENT_GHOST) && c.ghost_x == 96 && c.ghost_y == 128 && c.drop[0].state == VK_DROP_NONE);

    /* Flame 26 of the other objects (openMSX traces scratchpad/flame f1-f4, the whip faked at the real candle): it stays 41 frames at the
     * candle, then falls 8 px every other frame until it lands, and the object is made there (5F1F; no C500 object before): it appears
     * 41 + 2 n frames after the flame (+-1: the passes are 2 frames), n = steps of the fall. */
    {
        static const struct { VKByte stage, room, x, y, kind, steps, end_y; } flames[] = {
            { 7, 1, 96, 80, 0x02, 6, 128 }, { 6, 2, 128, 48, 0x02, 6, 96 },
            { 4, 3, 64, 48, 0x02, 8, 112 }, { 2, 2, 160, 112, 0x05, 4, 144 }
        };
        int k, j;
        for (k = 0; k < 4; k++)
        {
            vk_candles_begin(&c);
            place(&g, VK_AREA_INTERIOR, flames[k].stage, flames[k].room, 16, 192);
            vk_candles_enter(&c, &g);
            for (i = 0; i < c.count; i++)
                if (vk_candles_def(&c, i)->x == flames[k].x && vk_candles_def(&c, i)->y == flames[k].y) break;
            assert(i < c.count && vk_candles_def(&c, i)->kind == flames[k].kind);
            assert(vk_candles_hit(&c, i) == VK_CANDLE_EVENT_HIT);
            for (j = 0; j < VK_DROP_MAX && c.drop[j].state != VK_DROP_BURST; j++) {}
            assert(j < VK_DROP_MAX && c.drop[j].y == flames[k].y);
            run(&c, &g, &w, 42 + 2 * flames[k].steps);
            assert(c.drop[j].state == VK_DROP_BURST && c.drop[j].y == flames[k].y + 8 * flames[k].steps);
            run(&c, &g, &w, 1);
            assert(c.drop[j].state == VK_DROP_REST && c.drop[j].y == flames[k].end_y && c.drop[j].x == flames[k].x);
        }
    }

    puts("test_candles: OK");
    /* Contents of the chests traced on openMSX (31 chests of the stages 1-9, research/chests-stage1-9.md). */
    vk_candles_begin(&c);
    assert((vk_candles_give(&c, 0x0F) & VK_CANDLE_EVENT_BOX_ITEM) && c.c70f == 3);
    assert(c.c431 == 0x40 && (c.c701 & VK_ITEM_BIT_0F));              /* 8E33: C431 00 -> 40, C701 -> 80 */
    /* 8E79 (12): C431 bit 1, C700 = 3, C701 bit 2: three keys, icon 12, sound 0F. */
    assert(vk_candles_give(&c, 0x12) == VK_CANDLE_EVENT_ITEM);
    assert(c.gold_keys == 3 && c.triple_key == 1 && c.hud_key == 0x12 && (c.c431 & 0x02));
    assert(vk_candles_sound(0x12) == 0x0F);
    /* 1B: the thrown weapon level (C416 = 2), 1C: 3, 1A: 1 (traced C416 00 -> 02 / 03 / 01). */
    vk_candles_begin(&c);
    vk_candles_give(&c, 0x1B);
    assert(c.weapon == 2);
    vk_candles_give(&c, 0x1C);
    assert(c.weapon == 3);
    vk_candles_give(&c, 0x1A);
    assert(c.weapon == 1);
    assert(vk_candles_give(&c, 0x13) == VK_CANDLE_EVENT_ITEM && vk_candles_sound(0x13) == 0x10);
    assert(vk_candles_sound(0x05) == 0x1B && vk_candles_sound(0x09) == 0x12 && vk_candles_sound(0x0D) == 0x12);
    /* Death drops (research/death-flame.md): the small heart of the end of a death flame, every pass of 39 traced hearts (landing or exit at the
     * pass of the trace, then the object 83 at (X & F8, (Y - 16) & F8)), and the flames 26 left in place of a flame (48 traced). */
    {
        unsigned k;
        for (k = 0; k < sizeof kHearts / sizeof kHearts[0]; k++)
        {
            const int *r = kHearts[k];
            const int n = 2 * r[4];
            vk_candles_begin(&c);
            place(&g, VK_AREA_INTERIOR, (VKByte)r[0], (VKByte)r[1], 0, 0);
            vk_whip_reset(&w);
            vk_candles_enter(&c, &g);
            for (i = 0; i < VK_DROP_MAX; i++) c.drop[i].state = VK_DROP_NONE;   /* the fixed objects of the room */
            vk_candles_death_drop(&c, VK_OBJ_SMALL_HEART, r[2], r[3]);
            assert(c.drop[0].state == VK_DROP_FALL);
            run(&c, &g, &w, n - 1);
            assert(c.drop[0].state == VK_DROP_FALL);
            run(&c, &g, &w, 1);
            if (r[7] == 1)
                assert(c.drop[0].state == VK_DROP_REST && c.drop[0].x == (r[5] & 0xF8) && c.drop[0].y == ((r[6] - 16) & 0xF8));
            else
                assert(c.drop[0].state == VK_DROP_NONE);
        }
        for (k = 0; k < sizeof kFlames26 / sizeof kFlames26[0]; k++)
        {
            const int *r = kFlames26[k];
            const int n = 43 + 2 * r[4];
            vk_candles_begin(&c);
            place(&g, VK_AREA_INTERIOR, (VKByte)r[0], (VKByte)r[1], 0, 0);
            vk_whip_reset(&w);
            vk_candles_enter(&c, &g);
            for (i = 0; i < VK_DROP_MAX; i++) c.drop[i].state = VK_DROP_NONE;   /* the fixed objects of the room */
            vk_candles_death_drop(&c, VK_OBJ_BIG_HEART, r[2], r[3]);
            assert(c.drop[0].state == VK_DROP_BURST);
            run(&c, &g, &w, n - 1);
            assert(c.drop[0].state == VK_DROP_BURST);
            run(&c, &g, &w, 1);
            if (r[7] == 1)
                assert(c.drop[0].state == VK_DROP_REST && c.drop[0].x == r[5] && c.drop[0].y == r[6] && c.drop[0].kind == VK_OBJ_BIG_HEART);
            else
                assert(c.drop[0].state == VK_DROP_NONE);
        }
        /* Out of the screen when the flame ends: nothing. */
        vk_candles_begin(&c);
        place(&g, VK_AREA_INTERIOR, 9, 1, 0, 0);
        vk_candles_enter(&c, &g);
        vk_candles_death_drop(&c, VK_OBJ_SMALL_HEART, 4, 100);
        assert(c.drop[0].state == VK_DROP_NONE);
    }

    /* Stage 10 (5A49 + 5B1B run by build_candles_md.py, research/stage10-level.md): candles and hidden blocks per room, chests and
     * golden keys (objects 83 of the ROM), the two breakable blocks and the items of their slots. */
    {
        static const int candles[9] = { 2, 3, 2, 3, 2, 3, 4, 6, 1 };
        static const int fixed[9] = { 1, 1, 0, 0, 1, 3, 0, 0, 0 };
        int r, sum = 0;
        for (r = 0; r < 9; r++) {
            assert(vk_candle_first[10][r + 1] - vk_candle_first[10][r] == candles[r]);
            assert(vk_fixed_object_first[10][r + 1] - vk_fixed_object_first[10][r] == fixed[r]);
            sum += candles[r];
        }
        assert(sum == 26 && vk_candle_first[10][9] - vk_candle_first[10][0] == 26);
        assert(vk_candle_first[10][0] == 224 && vk_candle_first[10][9] == 250);
        assert(vk_fixed_object_first[10][0] == 64 && vk_fixed_object_first[10][9] == 70);
        assert(vk_wall_first[10][2] - vk_wall_first[10][1] == 1 && vk_wall_first[10][9] - vk_wall_first[10][8] == 1);
        assert(vk_wall_items[10][1][0] == 0x12 && vk_wall_items[10][8][0] == 0x04);
        /* chests (0x19) and their content (+13): rooms 0, 4 and 5 */
        assert(vk_fixed_object_defs[64].kind == 0x19 && vk_fixed_object_defs[64].content == 0x1A && vk_fixed_object_defs[64].x == 224 && vk_fixed_object_defs[64].y == 128);
        assert(vk_fixed_object_defs[66].kind == 0x19 && vk_fixed_object_defs[66].content == 0x11);
        assert(vk_fixed_object_defs[68].content == 0x1C && vk_fixed_object_defs[69].content == 0x13);
        /* no candle of object 15 in the stage 10: the object 1B cannot be made there */
        for (r = vk_candle_first[10][0]; r < vk_candle_first[10][9]; r++) assert(vk_candle_defs[r].kind != 0x15);
        for (r = 0; r < 9; r++) assert(vk_wall_items[10][r][0] != 0x15 && vk_wall_items[10][r][1] != 0x15);
    }

    /* Stage 11 (research/stage11-level.md): 25 candles and hidden blocks (the white key in the block (96, 80) of the room 1), one golden key (17) in the room 1,
     * the items 0E and 16 of the wall slots of the rooms 1 and 3, four candles of object 15 (rooms 0 and 5: the object 1B). */
    {
        static const int candles[6] = { 3, 7, 3, 7, 3, 2 };
        int r, n15 = 0;
        for (r = 0; r < 6; r++) assert(vk_candle_first[11][r + 1] - vk_candle_first[11][r] == candles[r]);
        assert(vk_candle_first[11][0] == 250 && vk_candle_first[11][6] == 275);
        assert(vk_fixed_object_first[11][0] == 70 && vk_fixed_object_first[11][1] == 70 && vk_fixed_object_first[11][2] == 71 && vk_fixed_object_first[11][6] == 71);
        assert(vk_fixed_object_defs[70].kind == 0x17 && vk_fixed_object_defs[70].x == 128 && vk_fixed_object_defs[70].y == 64);
        assert(vk_candle_defs[vk_candle_first[11][1]].kind == 0x18 && vk_candle_defs[vk_candle_first[11][1]].mode == 3 &&
               vk_candle_defs[vk_candle_first[11][1]].x == 96 && vk_candle_defs[vk_candle_first[11][1]].y == 80);
        assert(vk_wall_items[11][1][0] == 0x0E && vk_wall_items[11][3][0] == 0x16);
        for (r = vk_candle_first[11][0]; r < vk_candle_first[11][6]; r++) n15 += vk_candle_defs[r].kind == 0x15;
        assert(n15 == 4);
        /* the out and taken flags of the stage 11 live in the two rows added at the end of VKCandles */
        {
            VKCandles c2;
            vk_candles_begin(&c2);
            VK_CANDLE_OUT(&c2, 11, 1) = 5; VK_CANDLE_TAKEN(&c2, 11, 1) = 1;
            assert(c2.out11[1] == 5 && c2.taken11[1] == 1 && c2.out[10][1] == 0);
            vk_candles_reset_stage(&c2, 11);
            assert(c2.out11[1] == 0 && c2.taken11[1] == 0);
        }
    }

    return 0;
}
