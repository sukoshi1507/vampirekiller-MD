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
               offsetof(VKCandles, gold_keys) == 119 && offsetof(VKCandles, hud_key) == 169,
               "VKCandles layout");

/* Courtyard tables of the intro generator, not linked in native tests. */
const VKByte vk_courtyard_maps[3][768] = {{0}};
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

/* Whip in its full lash (phase 2, C422 = 3) from Simon's place. */
static void lash(VKWhip *w, const VKGame *g, int facing)
{
    vk_whip_reset(w);
    w->timer = 5;
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
    w.timer = 9;                                  /* phase 1: no touch */
    assert(!vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0));
    lash(&w, &g, 1);                              /* facing left: whip X 24 */
    assert(!vk_candle_whip_touch(vk_candles_def(&c, 0), &w, 0));

    /* The hit puts it out, a small heart bursts 16 frames then sways down. */
    lash(&w, &g, 0);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_HIT);
    assert(hit == 0 && !vk_candles_lit(&c, 0) && c.drop[0].state == VK_DROP_BURST);
    assert(c.drop[0].kind == VK_OBJ_SMALL_HEART);
    vk_whip_reset(&w);
    run(&c, &g, &w, 15);
    assert(c.drop[0].state == VK_DROP_FALL);
    for (i = 0; i < 400 && c.drop[0].state == VK_DROP_FALL; i++) vk_candles_tick(&c, &g, &w, &hit);
    assert(c.drop[0].state == VK_DROP_REST);
    assert((c.drop[0].x & 7) == 0 && (c.drop[0].y & 7) == 0);
    /* Picked up by walking onto it: hearts + 1. */
    g.player.x = (VKByte)(c.drop[0].x + 8);
    g.player.y = (VKByte)(c.drop[0].y + 2 + 20);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_HEARTS);
    assert(c.hearts == 1 && c.drop[0].state == VK_DROP_NONE);

    /* 7FE2: a candle object falls as type 82 (bit 7 set) and is caught in the air. */
    c.drop[0].state = VK_DROP_BURST;
    c.drop[0].timer = 0;
    c.drop[0].x = 64;
    c.drop[0].y = 144;
    g.player.x = 100;
    run(&c, &g, &w, 16);
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

    /* An object left on the floor goes after 8 x 16 frames. */
    vk_candles_begin(&c);
    place(&g, VK_AREA_COURTYARD, 1, 1, 10, 176);
    vk_candles_enter(&c, &g);
    vk_candles_hit(&c, 0);
    vk_whip_reset(&w);
    for (i = 0; i < 100 && c.drop[0].state != VK_DROP_REST; i++) vk_candles_tick(&c, &g, &w, &hit);
    assert(c.drop[0].kind == VK_OBJ_BIG_HEART && c.drop[0].state == VK_DROP_REST);
    run(&c, &g, &w, 8 * 16 + 16);
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
    /* 8C2F/8BCC: the content waits 32 frames 8 px above the chest, then falls. */
    assert(c.drop[1].state == VK_DROP_WAIT && c.drop[1].y == 168);
    g.player.x = 100;
    run(&c, &g, &w, 31);
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

    /* 8D30: C419 = the object; 03, 05-09 and 16 have their effect in enemies.c
     * (VK_CANDLE_EVENT_PICK), and so does a whip for C440 (8D70 -> 8D7C). The
     * handler's sound: 1B for 05, 16 for 08, 12 for the others. 0A and 0F only set
     * their C701 bit here (VK_CANDLE_EVENT_BOX_ITEM); 10, 11 are taken without effect. */
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
        /* 8F55: 32 counted passes (C003 & 3F != 0), then 8EE6 draws C701 bits 7..3. */
        while (c.box_pick) { vk_candles_tick(&c, &g, &w, &hit); ticks++; }
        assert(ticks >= VK_ITEM_PICK_TICKS && ticks <= VK_ITEM_PICK_TICKS + 1);
        assert(c.box_items == c.c701 && c.box_timer == 0 && c.box_timer_on);
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

    puts("test_candles: OK");
    return 0;
}
