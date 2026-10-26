/* Native smoke tests: gcc -std=c99 -Wall -Wextra -Werror ... */
#include <assert.h>
#include <string.h>
#include "generated/stage1_collision.h"
#include "gameplay.h"
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

    g.room = 2;
    g.player.x = 248;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_NONE);
    assert(g.room == 2);

    g.player.x = 210;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_ENTER);
    assert(g.area == VK_AREA_INTERIOR && g.collision_verified);
    assert(memcmp(vk_game_collision(&g), vk_player_room, 768) == 0);
    assert(vk_game_debug_room(&g,1));
    assert(g.room == 1 && !g.collision_verified);
    assert(vk_game_collision(&g) == vk_stage1_collision[1]);
    assert(!vk_game_debug_room(&g, 8));
    assert(g.room == 1);
    /* Demonstration stair must move diagonally, not merely teleport a sprite. */
    g.player.x = 128;
    g.player.y = 176;
    g.player.on_stairs = 1;
    g.player.mode = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE);
    assert(g.player.x == 129 && g.player.y == 175 && g.player.on_stairs);
    assert(g.player.facing == 0 && g.player.pose <= 2);
    assert(vk_game_tick_controls(&g, 0, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.player.x == 128 && g.player.y == 176 && g.player.on_stairs);
    assert(g.player.facing == 1 && g.player.pose >= 3 && g.player.pose <= 5);
    assert(vk_game_tick_controls(&g, 0, 0, 0, 0) == VK_EVENT_NONE);
    assert(g.player.on_stairs && g.player.facing == 1);
    assert(vk_game_debug_room(&g, 0) && g.collision_verified);
    assert(g.player.x == 16 && g.player.y == 192);

    vk_game_begin(&g);
    g.room = 2;
    g.player.x = 180;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 1) != VK_EVENT_ENTER);
    return 0;
}
