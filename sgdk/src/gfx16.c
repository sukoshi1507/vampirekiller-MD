#include "gfx16.h"
#include "generated/rooms.h"
#include "generated/intro_assets.h"
#include "generated/player_assets.h"
#include "generated/simon_attack_md.h"
#include "generated/simon_crouch_md.h"
#include "generated/simon_crouch_levels_md.h"
#include "generated/whip_level1_md.h"
#include "generated/whip_level2_md.h"
#include "generated/simon16_md.h"
#include "boss.h"

u8 vk_gfx16;
u8 vk_gfx16_on;

const VKGfx16Room *vk_gfx16_room(u16 stage, u16 room)
{
    if (!vk_gfx16 || stage < 1 || stage > 18) return 0;
    return vk_gfx16_rooms[vk_stages[stage - 1].first + room];
}

u8 vk_gfx16_enemies(u16 stage, u16 room)
{
    return (u8)(vk_gfx16_on && stage >= 1 && stage <= 3 && !vk_boss_room(stage, room));
}

u8 vk_gfx16_boss3(u16 stage, u16 room)
{
    return (u8)(vk_gfx16_on && stage == 3 && vk_boss_room(3, room));
}

const u16 *vk_gfx16_pal2(u16 stage, u16 room, const u16 *msx)
{
    if (vk_gfx16_boss3(stage, room)) return vk_boss16_palette;
    return vk_gfx16_enemies(stage, room) ? vk_enemy16_palette : msx;
}

const VKGfx16Room *vk_gfx16_scene(u16 scene)
{
    if (!vk_gfx16) return 0;
    if (scene >= 3 && scene <= 5) return vk_gfx16_scenes[scene - 3];
    return scene == 6 ? vk_gfx16_room(1, 0) : 0;
}

u16 vk_scene_cell(u16 scene, u16 col, u16 row)
{
    const VKGfx16Room *g = vk_gfx16_on ? vk_gfx16_scene(scene) : 0;
    return (g && row >= 4 && row < 26) ? g->map[(row - 4) * 32 + col] : vk_scenes[scene].map[row * 32 + col];
}

u16 vk_room_cell(u16 stage, u16 room, u16 i)
{
    const VKGfx16Room *g = vk_gfx16_on ? vk_gfx16_room(stage, room) : 0;
    return g ? g->map[i] : vk_maps[vk_stages[stage - 1].first + room][i];
}

const u32 *vk_room_tile(u16 stage, u16 room, u16 tile)
{
    const VKGfx16Room *g = vk_gfx16_on ? vk_gfx16_room(stage, room) : 0;
    return g ? &vk_gfx16_pool[g->slots[tile] * 8] : &vk_tiles[vk_stages[stage - 1].tileset][tile * 8];
}

void vk_gfx16_load_tiles(const VKGfx16Room *g, u16 index, TransferMethod tm)
{
    /* The slots point anywhere in the pool (about 60 runs per room): one VRAM address, then
     * the tiles written by the CPU, no transfer set up per run (draw_room is timed to the image). */
    vu32 *data = (vu32 *)VDP_DATA_PORT;
    const u16 *slot = g->slots;
    u16 n = g->count;
    (void)tm;
    VDP_setAutoInc(2);
    *(vu32 *)VDP_CTRL_PORT = VDP_WRITE_VRAM_ADDR((u32)index * 32);
    while (n--)
    {
        const u32 *src = &vk_gfx16_pool[*slot++ * 8];
        *data = *src++; *data = *src++; *data = *src++; *data = *src++;
        *data = *src++; *data = *src++; *data = *src++; *data = *src;
    }
}

void vk_gfx16_draw_map(const VKGfx16Room *g)
{
    vu16 *data = (vu16 *)VDP_DATA_PORT;
    const u8 *cell = g->map;
    const u16 base = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX);
    u16 row, col;
    VDP_setAutoInc(2);
    for (row = 4; row < 26; row++)
    {
        *(vu32 *)VDP_CTRL_PORT = VDP_WRITE_VRAM_ADDR((u32)VDP_getPlaneAddress(BG_A, 0, row));
        for (col = 0; col < 32; col += 4)
        {
            *data = (u16)(base + *cell++); *data = (u16)(base + *cell++);
            *data = (u16)(base + *cell++); *data = (u16)(base + *cell++);
        }
    }
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

/* No 16-bit picture (0xFF): the object is empty on the MSX too (0x1F, 0x20) or never shown (0x15), its MSX cell is used. */
const u32 *vk_item16_object(u16 object)
{
    const u8 cell = vk_item16_object_cell[object - 1];
    return cell == 0xFF ? &vk_object_tiles[(object - 1) * 32] : &vk_item16_object_tiles[cell * 32];
}
const u32 *vk_item_gfx_weapon_tiles(void) { return vk_gfx16 ? vk_item16_weapon_tiles : vk_weapon_tiles; }
