/* Native smoke tests: gcc -std=c99 -Wall -Wextra -Werror ... */
#include <assert.h>
#include <string.h>
#include "generated/stage1_collision.h"
#include "gameplay.h"
#include "room_manager.h"
#include "generated/player_assets.h"

/* Synthetic, empty collision maps isolate scene routing from graphical assets. */
const VKByte vk_courtyard_maps[3][768] = {{0}};
const VKByte vk_courtyard_neighbors[3][4] = {
    {255,255,255,1}, {255,255,0,2}, {255,255,1,255}
};

int main(void)
{
    VKGame g;
    vk_game_begin(&g);
    assert(g.area == VK_AREA_COURTYARD && g.room == 0 && g.collision_verified);
    assert(vk_game_collision(&g) == vk_courtyard_maps[0]);

    /* Force right-side boundary and verify continuity. */
    g.player.x = 248;
    g.player.y = 100;
    g.player.mode = 1;
    g.player.phase = 3;
    g.player.facing = 0;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_ROOM);
    assert(g.room == 1 && g.player.x == 10);
    assert(g.player.mode == 1 && g.player.phase != 0);
    assert(vk_game_collision(&g) == vk_courtyard_maps[1]);

    /* Courtyard room 2: UP at x=192..216 does not change room (openMSX). */
    g.room = 2;
    g.player.x = 200;
    g.player.y = 176;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2);
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2);
    /* Courtyard room 2 -> hall room 0: RIGHT at x=232 (openMSX run 6). */
    g.player.x = 232;
    g.player.y = 176;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_ENTER);
    assert(g.area == VK_AREA_INTERIOR && g.room == 0 && g.collision_verified);
    assert(g.player.x == 232 && g.player.y == 176);
    assert(memcmp(vk_game_collision(&g), vk_player_room, 768) == 0);
    assert(vk_game_debug_room(&g,1));
    assert(g.room == 1 && !g.collision_verified);
    assert(vk_game_collision(&g) == vk_stage1_collision[1]);
    assert(!vk_game_debug_room(&g, 8));
    assert(g.room == 1);
    /* Stage 1 room 1, flight A (tile 12, 7 steps): HAUT at the foot (48,192)
     * climbs to the landing (112,128), the 64 px of research/stairs.md. */
    {
        int frames;
        g.player.x = 48;
        g.player.y = 192;
        g.player.on_stairs = 0;
        g.player.mode = 0;
        assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE);
        assert(g.player.on_stairs && g.player.x == 48 && g.player.y == 192);
        assert(g.player.stair_dir == 1 && g.player.stair_len == 64);
        /* Releasing the key holds Simon on the step. */
        for (frames = 0; frames < 6; frames++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.player.on_stairs && g.player.x == 48 && g.player.y == 192);
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++) {
            vk_game_tick_controls(&g, 0, 0, 1, 0);
            assert(g.player.x - 48 == 192 - g.player.y);
        }
        assert(!g.player.on_stairs && g.player.x == 112 && g.player.y == 128);
        /* Descending with BAS from the same foot returns to (48,192). */
        g.player.x = 48;
        g.player.y = 192;
        assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE && g.player.on_stairs);
        for (frames = 0; frames < 200; frames++) {
            if (g.player.stair_anim == 16) break;
            vk_game_tick_controls(&g, 0, 0, 1, 0);
        }
        assert(g.player.stair_anim == 16);
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(!g.player.on_stairs && g.player.x == 48 && g.player.y == 192);
    }
    /* X=36 is outside the entry window of that foot: no staircase. */
    g.player.x = 36;
    g.player.y = 192;
    g.player.mode = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE && !g.player.on_stairs);
    /* Flight C (tile 13, 8 steps) from its foot (240,96) climbs past the top of the
     * playfield: the hall link 1 -> 5 moves Simon to room 5, on a floor. */
    assert(vk_game_debug_room(&g, 1));
    g.player.x = 240;
    g.player.y = 96;
    g.player.mode = 0;
    {
        VKEvent event = VK_EVENT_NONE;
        int frames;
        for (frames = 0; frames < 300 && event != VK_EVENT_VERTICAL; frames++)
            event = vk_game_tick_controls(&g, 0, 0, 1, 0);
        /* The MSX exit fires at Y < 48 (x 191): Simon lands at (207,224), gliding. */
        assert(event == VK_EVENT_VERTICAL && g.room == 5 && g.player.on_stairs);
        assert(g.player.stair_glide && g.player.x == 207 && g.player.y == 224);
        /* HAUT glides him up the 32 px diagonal to the floor at (175,192). */
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 1, 0);
        assert(!g.player.on_stairs && g.player.x == 175 && g.player.y == 192);
    }
    assert(vk_stage1_links[1].up == 5 && vk_stage1_links[5].down == 1);
    assert(vk_stage1_links[3].up == 7 && vk_stage1_links[7].down == 3);
    /* Bottom edge Y 225 in room 5 sends Simon down to room 1 at its top. */
    g.player.x = 192;
    g.player.y = 225;
    g.player.mode = 4;
    g.player.on_stairs = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 0, 0) == VK_EVENT_VERTICAL);
    assert(g.room == 1 && g.player.y == 48 && g.player.x == 192);
    /* From the room 5 glide foot (207,224), BAS drops through Y 225 into room 1
     * at X 191, the -16 of the MSX exit. */
    assert(vk_game_debug_room(&g, 5));
    g.player.x = 207; g.player.y = 224; g.player.on_stairs = 1;
    g.player.stair_dir = 0; g.player.stair_len = 32; g.player.stair_anim = 0;
    g.player.stair_glide = 1;
    {
        VKEvent event = VK_EVENT_NONE;
        int frames;
        for (frames = 0; frames < 20 && event != VK_EVENT_VERTICAL; frames++)
            event = vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(event == VK_EVENT_VERTICAL && g.room == 1 && g.player.x == 191 && g.player.y == 48);
        /* Room 1 flight C (tile 13) passes (184,40): Simon is put back on the stairs
         * at progress 48, six cells above its foot, instead of falling. */
        assert(g.player.on_stairs && g.player.mode == 0 && g.player.stair_dir == 0);
        assert(g.player.stair_anim == 48 && g.player.stair_len == 72);
        /* Holding BAS descends the flight to its foot and leaves the stairs there.
         * The arrival X is 191, one pixel left of the diagonal, so the foot is (239,96). */
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(!g.player.on_stairs && g.player.x == 239 && g.player.y == 96);
    }
    /* Stage 1 room 7: BAS on the tile 3 platform at (168..180, 96) enters the top of
     * the flight, and holding BAS descends it to the foot (144,128), 32 px down-left
     * (research/down-stairs.md, measured by trace). */
    assert(vk_game_debug_room(&g, 7));
    g.player.x = 176; g.player.y = 96; g.player.mode = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.player.on_stairs && g.player.x == 176 && g.player.y == 96 && !g.player.crouched);
    /* Going down a right-up flight Simon looks down-left, that is left (facing 1). */
    assert(g.player.facing == 1);
    {
        int frames;
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(!g.player.on_stairs && g.player.x == 144 && g.player.y == 128);
    }
    assert(vk_game_debug_room(&g, 0) && g.collision_verified);
    assert(g.player.x == 16 && g.player.y == 192);

    /* Without walking right, x=232 is not a door; the old UP door is gone. */
    vk_game_begin(&g);
    g.room = 2;
    g.player.x = 232;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2);
    g.player.x = 200;
    g.player.x = 200;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2 && g.player.x < 232);

    /* Recoil keeps the walk limits: never past x 248, nothing on a staircase. */
    vk_game_debug_room(&g, 0);
    g.player.x = 240;
    g.player.y = 176;
    vk_game_push(&g, 22);
    assert(g.player.x == 248);
    g.player.on_stairs = 1;
    vk_game_push(&g, -10);
    assert(g.player.x == 248);
    g.player.on_stairs = 0;
    /* Recoil stops at the first wall of a hall room. */
    {
        VKByte room;
        int found = 0;
        for (room = 0; room < 8 && !found; room++)
        {
            int x;
            vk_game_debug_room(&g, room);
            for (x = 30; x < 240 && !found; x++)
            {
                int y;
                for (y = 40; y < 200 && !found; y += 8)
                    if (!vk_wall(vk_game_collision(&g), (VKByte)(x - 12), (VKByte)y, 1, 0, 1, 0) &&
                        !vk_wall(vk_game_collision(&g), (VKByte)(x - 1), (VKByte)y, 1, 0, 1, 0) &&
                        vk_wall(vk_game_collision(&g), (VKByte)x, (VKByte)y, 1, 0, 1, 0))
                    {
                        int k, open = 1;
                        for (k = x - 12; k < x; k++)
                            if (vk_wall(vk_game_collision(&g), (VKByte)k, (VKByte)y, 1, 0, 1, 0)) open = 0;
                        if (!open) continue;
                        g.player.x = (VKByte)(x - 12);
                        g.player.y = (VKByte)y;
                        vk_game_push(&g, 22);
                        assert(g.player.x == x - 1);
                        found = 1;
                    }
            }
        }
        assert(found);
    }

    /* Stage 2 after the door of hall room 7: room 0 at x 16, y 96, on a floor of
     * its own map; the six rooms and the ROM links (U2 D4 R1 for room 0). */
    vk_game_begin(&g);
    vk_game_enter_stage(&g, 2);
    assert(g.area == VK_AREA_INTERIOR && g.stage == 2 && g.room == 0);
    assert(g.player.x == 16 && g.player.y == 96);
    assert(vk_grounded(vk_game_collision(&g), g.player.x, g.player.y, 2, 0));
    assert(vk_stage_room_count(2) == 6);
    assert(vk_stage_links(2)[0].up == 2 && vk_stage_links(2)[0].down == 4 &&
           vk_stage_links(2)[0].left == VK_ROOM_NONE && vk_stage_links(2)[0].right == 1);
    assert(vk_game_debug_room(&g, 5) && g.stage == 2 && !vk_game_debug_room(&g, 6));
    g.player.x = 248;
    g.room = 0;
    assert(vk_room_try_edge(g.stage, &g.room, &g.player, 1) && g.room == 1);
    vk_game_respawn(&g);
    assert(g.stage == 2 && g.room == 0 && g.player.x == 16 && g.player.y == 96);
    return 0;
}
