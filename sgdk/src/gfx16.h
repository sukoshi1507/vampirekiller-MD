#ifndef VK_GFX16_H
#define VK_GFX16_H

#include <genesis.h>
#include "generated/gfx16_md.h"
#include "generated/enemies16_md.h"
#include "generated/boss3_16_md.h"
#include "generated/items16_md.h"
#include "generated/object_tiles_md.h"
#include "generated/weapons_md.h"
#include "generated/loot_md.h"
#include "generated/death_flame_md.h"
#include "generated/candle_flames_md.h"

/* 16-bit graphics mode (research/gfx16.md, tools/build_gfx16.py): the castle rooms drawn with
 * the 16-bit art, cell for cell on the MSX layout. Nothing of the game logic reads it.
 *
 * vk_gfx16: the option of the title screen (0: the MSX graphics, the default).
 * vk_gfx16_on: the room on screen is drawn with the 16-bit art. PAL0 then holds the palette of
 * that art, and everything else drawn with the colours of the MSX room palette (objects on the
 * plane, sprites) takes PAL2, which holds the MSX room palette (stages 1 to 3: the same colours for the entries
 * that content uses, see vk_gfx16_enemies below): VK_PAL_OBJ. */
extern u8 vk_gfx16;
extern u8 vk_gfx16_on;
#define VK_PAL_OBJ (vk_gfx16_on ? PAL2 : PAL0)

/* The common enemies of block 1 (stages 1 to 3: zombie, panther, bat, fishman) are drawn in the 16-bit art in
 * every room but the boss room: tools/build_enemies16.py, tiles of the same layout as the MSX arrays. Their
 * colours replace the MSX palette in PAL2 (vk_enemy16_palette): the MSX colours that other PAL2 content uses
 * keep their entries, so objects, candles, weapons and the HUD boxes look as in MSX mode. */
u8 vk_gfx16_enemies(u16 stage, u16 room);
/* The bat boss of the stage 3 (room 5, tools/build_boss3_16.py): 16-bit body, fireball and orb (vk_boss16_tiles, vk_orb16_tiles, same layout as the
 * MSX arrays). PAL2 of that room is vk_boss16_palette, whose entries 4 and 6 already hold the colours the orb writes there (006, 00A). */
u8 vk_gfx16_boss3(u16 stage, u16 room);
/* PAL2 of a room: the MSX room palette `msx`, or one of the palettes above in such a room. */
const u16 *vk_gfx16_pal2(u16 stage, u16 room, const u16 *msx);

/* The candles, objects, thrown weapons, loot and flames (tools/build_items16.py): with the option on they are the 16-bit
 * pictures of the same layout as the MSX arrays, drawn with PAL1 (VK_PAL_ITEM), which then holds vk_item16_palette
 * instead of the HUD palette: the entries the HUD reads (1, 2, 8, 14, 15) are the same, the others are the colours of
 * the pictures. Everything else that uses the MSX colours (HUD boxes, text of the merchant, key, door) keeps PAL2. */
#define VK_PAL_ITEM (vk_gfx16 ? PAL1 : VK_PAL_OBJ)
const u32 *vk_item_gfx_weapon_tiles(void);            /* vk_weapon_tiles or the 16-bit frames (not inline: the sprite pass) */
const u32 *vk_item16_object(u16 object);              /* 16-bit picture of the MSX object 1..0x22 (4 tiles) */
/* The tiles to load for the MSX object 1..0x22 (vk_object_tiles), the weapons, the loot and the death flames: inline,
 * these run in the room load and in the sprite pass (one test of vk_gfx16 more than the MSX path). */
#define vk_item_gfx_object_tiles(object) (vk_gfx16 ? vk_item16_object(object) : &vk_object_tiles[((object) - 1) * 32])
#define vk_item_gfx_loot_tiles() (vk_gfx16 ? vk_item16_loot_tiles : vk_loot_tiles)
#define vk_item_gfx_death_flame_tiles() (vk_gfx16 ? vk_item16_death_flame_tiles : vk_death_flame_tiles)
/* The ball of a candle flame (colour 0: white 0E, 1: skin, 2: red 08), 4 tiles: vk_candle_flame_tiles, or a loot ball. */
#define vk_item_gfx_flame_tiles(color) (vk_gfx16 ? &vk_item16_loot_tiles[(3 - (color)) * 32] : &vk_candle_flame_tiles[(color) * 32])

/* The 16-bit data of a castle room, 0 when the option is off or the room has none. */
const VKGfx16Room *vk_gfx16_room(u16 stage, u16 room);
/* The same for a ROM scene drawn as a playfield: the courtyard screens (scenes 3..5) and hall
 * room 0 (scene 6). */
const VKGfx16Room *vk_gfx16_scene(u16 scene);
/* The cells of rows 4..25 of that scene as drawn: 16-bit ones while vk_gfx16_on, else the scene map's. */
u16 vk_scene_cell(u16 scene, u16 col, u16 row);
/* Cell i (row * 32 + col) of the 32 x 22 cells of the room as drawn (tile offset from
 * TILE_USER_INDEX), and the 8 rows of the tile behind one of those cells: the 16-bit ones while
 * vk_gfx16_on, else vk_maps / vk_tiles. */
u16 vk_room_cell(u16 stage, u16 room, u16 i);
const u32 *vk_room_tile(u16 stage, u16 room, u16 tile);

/* The `count` tiles of a 16-bit room at VRAM tile `index` (one transfer per run of tiles that
 * follow each other in vk_gfx16_pool), and its 32 x 22 cells on BG_A rows 4..25 with PAL0 from
 * TILE_USER_INDEX. */
void vk_gfx16_load_tiles(const VKGfx16Room *g, u16 index, TransferMethod tm);
void vk_gfx16_draw_map(const VKGfx16Room *g);

/* Simon's tiles: the arrays decoded from the MSX ROM, or with the option on their 16-bit
 * counterparts of the same layout (generated/simon16_md.h, tools/build_simon16.py). The 16-bit
 * Simon is drawn with PAL3 (vk_simon16_palette in entries 1..13), on every screen of the game. */
const u32 *vk_simon_gfx_player_tiles(void);            /* 12 poses of 12 tiles */
const u32 *vk_simon_gfx_death_tiles(void);             /* 2 x 8 tiles */
const u32 *vk_simon_gfx_stair_tiles(void);             /* 8 poses of 12 tiles */
const u32 *vk_simon_gfx_shield_tiles(u16 pose);        /* 12 tiles */
const u32 *vk_simon_gfx_attack_tiles(void);            /* 3 frames of 32 tiles */
const u32 *vk_simon_gfx_whip_tiles(int kind);          /* kind 1: long whip, 2: thrown weapons; 6 frames of 32 tiles */
const u32 *vk_simon_gfx_crouch_tiles(void);            /* 4 frames of 32 tiles */
const u32 *vk_simon_gfx_crouch_level_tiles(void);      /* 6 frames of 32 tiles */

#endif
