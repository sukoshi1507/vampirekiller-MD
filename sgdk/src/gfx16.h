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
/* The 32 x 22 cells of the room as drawn (tile offsets from TILE_USER_INDEX), and the 8 rows of
 * the tile behind one of those cells: the 16-bit ones while vk_gfx16_on, else vk_maps / vk_tiles. */
const u16 *vk_room_cells(u16 stage, u16 room);
const u32 *vk_room_tile(u16 stage, u16 room, u16 tile);

#endif
