/* Native tests of candles.c: whip touch, drops and pickups of the ROM rules
 * (research/castle-candles.md, section 9). */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "candles.h"

/* Offsets read by tests/md_scenarios/i_courtyard_torch_drop.txt (same layout on m68k). */
_Static_assert(offsetof(VKCandles, lit) == 4 && offsetof(VKCandles, drop) == 6 &&
               sizeof(VKDrop) == 14 && offsetof(VKCandles, hearts) == 118, "VKCandles layout");

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
    lash(&w, &g, 1);
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_WHITE_KEY);
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
    g.player.x = 72;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_GOLD_KEY);
    assert(c.gold_keys == 1 && c.drop[0].state == VK_DROP_NONE);
    run(&c, &g, &w, 200);                          /* the key does not time out */
    g.player.x = 184;
    assert(vk_candles_tick(&c, &g, &w, &hit) & VK_CANDLE_EVENT_GOLD_KEY);
    assert(c.gold_keys == 0 && c.drop[1].kind == VK_OBJ_BIG_HEART);
    g.player.x = 100;
    run(&c, &g, &w, 20);
    assert(c.drop[1].state == VK_DROP_REST);
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

    puts("test_candles: OK");
    return 0;
}
