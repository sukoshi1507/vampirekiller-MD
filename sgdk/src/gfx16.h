#ifndef VK_GFX16_H
#define VK_GFX16_H

#include <genesis.h>
#include "generated/gfx16_md.h"

/* 16-bit graphics mode (research/gfx16.md, tools/build_gfx16.py): the castle rooms drawn with
 * the 16-bit art, cell for cell on the MSX layout. Nothing of the game logic reads it.
 *
 * vk_gfx16: the option of the title screen (0: the MSX graphics, the default).
 * vk_gfx16_on: the room on screen is drawn with the 16-bit art. PAL0 then holds the palette of
 * that art, and everything else drawn with the colours of the MSX room palette (objects on the
 * plane, sprites) takes PAL2, which always holds the MSX room palette: VK_PAL_OBJ. */
extern u8 vk_gfx16;
extern u8 vk_gfx16_on;
#define VK_PAL_OBJ (vk_gfx16_on ? PAL2 : PAL0)

/* The 16-bit data of a castle room, 0 when the option is off or the room has none. */
const VKGfx16Room *vk_gfx16_room(u16 stage, u16 room);
/* The same for a ROM scene drawn as a playfield: the courtyard screens (scenes 3..5) and hall
 * room 0 (scene 6). */
const VKGfx16Room *vk_gfx16_scene(u16 scene);
/* The cells of rows 4..25 of that scene as drawn: 16-bit ones while vk_gfx16_on, else the scene map's. */
u16 vk_scene_cell(u16 scene, u16 col, u16 row);
/* The 32 x 22 cells of the room as drawn (tile offsets from TILE_USER_INDEX), and the 8 rows of
 * the tile behind one of those cells: the 16-bit ones while vk_gfx16_on, else vk_maps / vk_tiles. */
const u16 *vk_room_cells(u16 stage, u16 room);
const u32 *vk_room_tile(u16 stage, u16 room, u16 tile);

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
