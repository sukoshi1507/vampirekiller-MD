#include "gfx16.h"
#include "generated/rooms.h"
#include "generated/player_assets.h"
#include "generated/simon_attack_md.h"
#include "generated/simon_crouch_md.h"
#include "generated/simon_crouch_levels_md.h"
#include "generated/whip_level1_md.h"
#include "generated/whip_level2_md.h"
#include "generated/simon16_md.h"

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

#define PICK(msx, art) ((const u32 *)(vk_gfx16 ? (const void *)(art) : (const void *)(msx)))
const u32 *vk_simon_gfx_player_tiles(void) { return PICK(vk_player_tiles, vk_simon16_player); }
const u32 *vk_simon_gfx_death_tiles(void) { return PICK(vk_player_death_tiles, vk_simon16_death); }
const u32 *vk_simon_gfx_stair_tiles(void) { return PICK(vk_player_stair_tiles, vk_simon16_stair); }
const u32 *vk_simon_gfx_shield_tiles(u16 pose) { return PICK(vk_player_shield_tiles[pose], vk_simon16_shield[pose]); }
const u32 *vk_simon_gfx_attack_tiles(void) { return PICK(vk_simon_attack_tiles, vk_simon16_attack); }
const u32 *vk_simon_gfx_whip_tiles(int kind)
{
    return kind == 1 ? PICK(vk_whip_level1_tiles, vk_simon16_whip1) : PICK(vk_whip_level2_tiles, vk_simon16_whip2);
}
const u32 *vk_simon_gfx_crouch_tiles(void) { return PICK(vk_simon_crouch_tiles, vk_simon16_crouch); }
const u32 *vk_simon_gfx_crouch_level_tiles(void) { return PICK(vk_simon_crouch_level_tiles, vk_simon16_crouch_levels); }
