/* Native tests of items.c: white key of hall room 2, door of hall room 7
 * (research/door-room7.md, section 16, runs K1, K3 and K4). */
#include <assert.h>
#include <stdio.h>
#include "items.h"

/* Courtyard tables of the intro generator, not linked in native tests. */
const VKByte vk_courtyard_maps[3][768] = {{0}};
const VKByte vk_courtyard_neighbors[3][4] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

static void hall(VKGame *g, VKByte room, VKByte x, VKByte y)
{
    vk_game_begin(g);
    g->area = VK_AREA_INTERIOR;
    g->room = room;
    g->player.x = x;
    g->player.y = y;
    g->player.facing = 0;
}

int main(void)
{
    VKItems it;
    VKGame g;
    int t;

    /* K1: the key lies in room 2 and goes when Simon walks onto it (x 148, y 192). */
    vk_items_begin(&it);
    hall(&g, 2, 166, 192);
    assert(!vk_items_key_shown(&it, &g));                 /* hidden until the whip hits */
    it.white_key_revealed = 1;
    assert(vk_items_key_shown(&it, &g));
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE);
    g.player.x = 149;
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE);
    g.player.x = 148;
    assert(vk_items_tick(&it, &g) == VK_ITEM_KEY_TAKEN);
    assert((it.keys & VK_KEY_WHITE) && !vk_items_key_shown(&it, &g));
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE);          /* taken once */
    /* Revealed, then the room is left: the C500 key is gone, the block is back. */
    vk_items_begin(&it);
    it.white_key_revealed = 1;
    vk_items_enter_room(&it);
    assert(!vk_items_key_shown(&it, &g));

    /* K4: without the key the door is a wall at x 228 on the ledge (y 96). */
    vk_items_begin(&it);
    hall(&g, 7, 232, 96);
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && g.player.x == 228);
    assert(it.door == VK_DOOR_CLOSED && !vk_items_controls_locked(&it));
    /* Below the ledge nothing blocks. */
    hall(&g, 7, 240, 192);
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && g.player.x == 240);

    /* With the key, facing left: no opening. */
    it.keys = VK_KEY_WHITE;
    hall(&g, 7, 228, 96);
    g.player.facing = 1;
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && it.door == VK_DOOR_CLOSED);

    /* K3: facing right at (228, 96) the door opens and the key goes (7743). */
    g.player.facing = 0;
    assert(vk_items_tick(&it, &g) == VK_ITEM_DOOR_OPENED);
    assert(!(it.keys & VK_KEY_WHITE) && vk_items_controls_locked(&it));
    assert(vk_items_door_lift(&it) == 0);
    /* C5AF: 0 at frame 5800, 1 at 5801-5802, then one pixel every two frames. */
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && vk_items_door_lift(&it) == 0);
    assert(vk_items_tick(&it, &g) == VK_ITEM_DOOR_STEP && vk_items_door_lift(&it) == 1);
    for (t = 2; t < 86; t++) (void)vk_items_tick(&it, &g);
    assert(vk_items_door_lift(&it) == VK_DOOR_LIFT_MAX && g.player.x == 228);
    /* Simon walks alone: x 229 at t 91 (frame 5890), one pixel every four frames. */
    for (; t < 91; t++) (void)vk_items_tick(&it, &g);
    assert(g.player.x == 229 && it.door == VK_DOOR_WALKING);
    for (; t < 95; t++) (void)vk_items_tick(&it, &g);
    assert(g.player.x == 230);
    for (; t < 167; t++) assert(vk_items_tick(&it, &g) != VK_ITEM_STAGE_EXIT);
    assert(g.player.x == 248);
    /* Frame 5967: D000 1 -> 2. */
    assert(vk_items_tick(&it, &g) == VK_ITEM_STAGE_EXIT && it.door == VK_DOOR_DONE);
    assert(!vk_items_controls_locked(&it));

    /* Stage 2 room 2 holds no white key and room 7 no door. */
    vk_items_begin(&it);
    hall(&g, 2, 148, 192);
    g.stage = 2;
    assert(!vk_items_key_shown(&it, &g) && vk_items_tick(&it, &g) == VK_ITEM_NONE && !it.keys);

    printf("test_items: OK\n");
    /* New life: a white key not used is lost and back in its hidden block. */
    vk_items_begin(&it);
    it.white_key_revealed = 1;
    it.white_key_taken = 1;
    it.keys = VK_KEY_WHITE;
    vk_items_new_life(&it);
    assert(it.keys == 0 && !it.white_key_taken && !it.white_key_revealed);
    return 0;
}
