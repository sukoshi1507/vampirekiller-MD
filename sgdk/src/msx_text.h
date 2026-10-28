#ifndef VK_MSX_TEXT_H
#define VK_MSX_TEXT_H

#include <genesis.h>

/* Text in the MSX font (4ACB/4AE7: 8x8 codes of VRAM page 1, copied opaque by
 * HMMM at any pixel X). Here the cells of a plane get tiles composed for that
 * X: ink white (PAL1 14), background black (PAL1 2), transparent outside the
 * text so that the cells around it keep what is under them.
 * Tiles from MSX_TEXT_TILE_INDEX, free VRAM between the candle tiles and the
 * sprite tiles (actors.h). */
#define MSX_TEXT_TILE_INDEX 990
#define MSX_TEXT_TILE_MAX 34

/* Draws text (ASCII, letters, digits, space) at pixel x, cell row y / 8 (y is a
 * multiple of 8), from tile index tile on. Returns the number of cells used. */
u16 vk_msx_text(VDPPlane plane, u16 x, u16 y, const char *text, u16 tile, bool priority);
/* Clears the cells a vk_msx_text call of that length at x used. */
void vk_msx_text_clear(VDPPlane plane, u16 x, u16 y, u16 length);

#endif
