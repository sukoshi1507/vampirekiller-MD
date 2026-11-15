#ifndef VK_GFX16_H
#define VK_GFX16_H

#include <genesis.h>
#include "generated/gfx16_md.h"
#include "generated/enemies16_md.h"

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
/* PAL2 of a room: the MSX room palette `msx`, or the palette above in such a room. */
const u16 *vk_gfx16_pal2(u16 stage, u16 room, const u16 *msx);

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
