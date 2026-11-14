/* Native tests of items.c: white key of hall room 2, door of hall room 7
 * (research/door-room7.md, section 16, runs K1, K3 and K4), key of stage 2
 * room 5 and door of stage 2 room 1 (section 18). */
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
    it.attacking = 1;                                     /* 7FE2: no pickup while C422 != 0 */
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && !it.white_key_taken);
    it.attacking = 0;
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

    /* 8580 runs only on the first frame of a pass (openMSX: 209 calls, none on a second frame): a touch on the second
     * frame leaves the door closed, the next first frame opens it. */
    g.player.facing = 0;
    it.second_frame = 1;
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && it.door == VK_DOOR_CLOSED && (it.keys & VK_KEY_WHITE));
    it.second_frame = 0;

    /* K3: facing right at (228, 96) the door opens and the key goes (7743). */
    it.c003 = 117;                                   /* C003 of the opening pass in the openMSX trace of the pose phase */
    assert(vk_items_tick(&it, &g) == VK_ITEM_DOOR_OPENED);
    assert(!(it.keys & VK_KEY_WHITE) && vk_items_controls_locked(&it));
    assert(vk_items_door_lift(&it) == 0);
    /* C5AF: 0 at frame 5800, 1 at 5801-5802, then one pixel every two frames. */
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && vk_items_door_lift(&it) == 0);
    assert(vk_items_tick(&it, &g) == VK_ITEM_DOOR_STEP && vk_items_door_lift(&it) == 1);
    for (t = 2; t < 86; t++) (void)vk_items_tick(&it, &g);
    assert(vk_items_door_lift(&it) == VK_DOOR_LIFT_MAX && g.player.x == 228);
    /* Simon walks alone: x 229 at f 90 (778A runs on the first frames, from f 88), one pixel every four frames. */
    for (; t < 90; t++) (void)vk_items_tick(&it, &g);
    assert(g.player.x == 229 && it.door == VK_DOOR_WALKING);
    /* 778A -> 6CA4: pose 2, 1, 0, 1 from bits 3-2 of C003 (117 + f / 2): 2 up to f 93, 1 from f 94, 0 from f 102 */
    assert(g.player.pose == 2);
    for (; t < 94; t++) (void)vk_items_tick(&it, &g);
    assert(g.player.x == 230 && g.player.pose == 1);
    for (; t < 94 + 8; t++) (void)vk_items_tick(&it, &g);
    assert(g.player.pose == 0);
    for (; t < 166; t++) assert(vk_items_tick(&it, &g) != VK_ITEM_STAGE_EXIT);
    assert(g.player.x == 248);
    /* X 248 at f 166, 7802 in the same frame, D000 1 -> 2 two frames later. */
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE);
    assert(vk_items_tick(&it, &g) == VK_ITEM_STAGE_EXIT && it.door == VK_DOOR_DONE);
    assert(!vk_items_controls_locked(&it));

    /* Stage 2 room 2 holds no white key and room 7 no door. */
    vk_items_begin(&it);
    hall(&g, 2, 148, 192);
    g.stage = 2;
    assert(!vk_items_key_shown(&it, &g) && vk_items_tick(&it, &g) == VK_ITEM_NONE && !it.keys);

    /* Stage 2 (run K4 of the stage 2, door-room7.md section 18): block 0x18 of
     * room 5 gives the key at (192,112) (89FD), taken at Simon x 188, y 144. */
    vk_items_begin(&it);
    vk_items_enter_stage(&it);
    hall(&g, 5, 176, 144);
    g.stage = 2;
    {
        VKByte kx, ky;
        assert(vk_items_key_place(&g, &kx, &ky) && kx == 192 && ky == 112);
    }
    assert(!vk_items_key_shown(&it, &g));
    it.white_key_revealed = 1;
    assert(vk_items_key_shown(&it, &g) && vk_items_tick(&it, &g) == VK_ITEM_NONE);
    g.player.x = 188;
    assert(vk_items_tick(&it, &g) == VK_ITEM_KEY_TAKEN && (it.keys & VK_KEY_WHITE));
    /* Hall room 2 of the same stage index holds nothing; stage 2 room 2 neither. */
    g.room = 2;
    assert(!vk_items_door_room(&g));
    /* Room 1: the bar is a wall at x 228 on the ledge, then opens facing right;
     * D000 2 -> 3 at t 168 (frame 7076, opening at 6908). */
    hall(&g, 1, 232, 96);
    g.stage = 2;
    assert(vk_items_door_room(&g));
    g.player.facing = 1;
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE && g.player.x == 228);
    g.player.facing = 0;
    assert(vk_items_tick(&it, &g) == VK_ITEM_DOOR_OPENED && !(it.keys & VK_KEY_WHITE));
    for (t = 1; t < 168; t++) assert(vk_items_tick(&it, &g) != VK_ITEM_STAGE_EXIT);
    assert(g.player.x == 248 && vk_items_tick(&it, &g) == VK_ITEM_STAGE_EXIT);
    /* Hall room 7 door state does not carry over: a new stage closes its door. */
    vk_items_enter_stage(&it);
    assert(it.door == VK_DOOR_CLOSED && !it.white_key_taken);

    /* Left door, stage 4 room 3 (12, 48), openMSX forced room (Simon from X 34, facing left, C701 bit 0):
     * opening at X 26 in walk pose C42E = 11, kept 88 frames; then the left walk compositions (C42E 10..12,
     * 765F), X 25 at f 88 (row 89) and one pixel every four frames, X 7 at f 160, D000 4 -> 5 at f 162. */
    vk_items_begin(&it);
    it.keys = VK_KEY_WHITE;
    it.c003 = 117;
    hall(&g, 3, 26, 96);
    g.stage = 4;
    g.player.facing = 1;
    g.player.pose = 4;                               /* left walk pose of the opening pass */
    assert(vk_items_tick(&it, &g) == VK_ITEM_DOOR_OPENED);
    for (t = 1; t < 88; t++)
    {
        g.player.pose = 3;                           /* the locked controls give the idle pose each frame */
        (void)vk_items_tick(&it, &g);
        assert(g.player.x == 26 && g.player.pose == 4);
    }
    /* f 88: first 778A, on a first frame of the pass: the half pixel already takes one, pose of C003 + 44 (117 + 44 = 161: 2, left 5) */
    for (t = 88; t <= 160; t++)
    {
        static const VKByte walk[4] = {2, 1, 0, 1};
        assert(vk_items_tick(&it, &g) == VK_ITEM_NONE);
        assert(g.player.x == 25 - (t - 88) / 4);
        assert(g.player.pose == walk[((117 + t / 2) >> 2) & 3] + 3);   /* never the right poses: no moonwalk */
    }
    assert(g.player.x == 7 && it.door == VK_DOOR_WALKING);
    assert(vk_items_tick(&it, &g) == VK_ITEM_NONE);   /* 77BA at f 160 (X 7), C000 = 10 at f 161 */
    assert(vk_items_tick(&it, &g) == VK_ITEM_STAGE_EXIT);   /* D000 + 1 at f 162 */

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
