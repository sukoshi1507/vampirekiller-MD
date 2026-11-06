#ifndef VK_WALL_VIEW_H
#define VK_WALL_VIEW_H

#include <genesis.h>
#include "wall.h"

/* Wall objects of wall.c as 32 x 24 PAL0 sprites (the 925E copy of a 32 x 32 frame of
 * VRAM page 1, whose first 8 lines are empty), and the merchant window (9397) in the
 * cells of plane A. */

extern VKWall vk_walls;

/* After a castle room is shown: the room tiles were loaded again over the frame
 * tiles, which are loaded again when next drawn. */
void vk_wall_view_room(void);
/* Sprites of the shown records from sprite index first, chained; returns how many
 * (the caller links the last one). */
u16 vk_wall_view_sprites(u16 first);
/* 9397: the window over the room, the cells under it saved (4946). */
void vk_shop_view_open(void);
/* 9507/950D: the saved cells back. */
void vk_shop_view_close(void);

#endif
