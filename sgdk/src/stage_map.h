#ifndef VK_STAGE_MAP_H
#define VK_STAGE_MAP_H

#include "candles.h"

/* The stage map of object 0F (research/stage-map.md; ROM code read in bank 2,
 * 95D0-98AB). No SGDK dependency: tested natively. */

#define VK_MAP_X 0x18                /* 9591: the panel, x 24..231, y 48..173, colour 3 */
#define VK_MAP_Y 0x30
#define VK_MAP_W 0xD0
#define VK_MAP_H 0x7E
#define VK_MAP_COLS (VK_MAP_W / 8)   /* the whole cells around the panel */
#define VK_MAP_ROWS 16
#define VK_MAP_BYTES (VK_MAP_COLS * VK_MAP_ROWS * 32)

/* Draws the map into tiles (VK_MAP_BYTES, cells in order, 4 bits a pixel): stage is
 * vk_candles_stage_of, room and px, py those of Simon (D001, C427, C425). */
void vk_stage_map_panel(VKByte *tiles);
void vk_stage_map_render(VKByte *tiles, const VKCandles *c, VKByte stage, VKByte room,
                         VKByte px, VKByte py);

#endif
