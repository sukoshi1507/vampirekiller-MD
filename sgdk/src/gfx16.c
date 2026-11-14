#include "gfx16.h"
#include "generated/rooms.h"

u8 vk_gfx16;
u8 vk_gfx16_on;

const VKGfx16Room *vk_gfx16_room(u16 stage, u16 room)
{
    if (!vk_gfx16 || stage < 1 || stage > 18) return 0;
    return vk_gfx16_rooms[vk_stages[stage - 1].first + room];
}

const u16 *vk_room_cells(u16 stage, u16 room)
{
    const VKGfx16Room *g = vk_gfx16_on ? vk_gfx16_room(stage, room) : 0;
    return g ? g->map : vk_maps[vk_stages[stage - 1].first + room];
}

const u32 *vk_room_tile(u16 stage, u16 room, u16 tile)
{
    const VKGfx16Room *g = vk_gfx16_on ? vk_gfx16_room(stage, room) : 0;
    return g ? &g->tiles[tile * 8] : &vk_tiles[vk_stages[stage - 1].tileset][tile * 8];
}
