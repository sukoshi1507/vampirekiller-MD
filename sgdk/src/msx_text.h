#ifndef VK_MSX_TEXT_H
#define VK_MSX_TEXT_H

#include <genesis.h>

/* Text in the MSX font (4ACB/4AE7: 8x8 codes of VRAM page 1, copied opaque by
 * HMMM at any pixel X). Here the cells of a plane get tiles composed for that
 * X: the font colours (PAL1 14 white, the heart 8 and 14; with the 16-bit graphics option the top and bottom rows of the
 * letters are the steel entries 10 and 3, vk_hud16_steel), background black (PAL1 2), transparent outside the
 * text so that the cells around it keep what is under them.
 * Tiles from MSX_TEXT_TILE_INDEX, free VRAM between the candle tiles and the
 * sprite tiles (actors.h). */
#define MSX_TEXT_TILE_INDEX 990
#define MSX_TEXT_TILE_MAX 34

/* Code of the heart in the font ('`' in the texts). */
#define VK_MSX_HEART 0x50
/* The 8 pixels of row py of the font code (0x20..0x5F), as the text draws them: the MSX font, or with the 16-bit graphics
 * option (vk_gfx16) the same shapes in the steel-blue letters of research/gfx16.md, '<' and '>' as chevrons. */
u32 vk_msx_font_row(u8 code, u16 py);

/* Draws text (ASCII, letters, digits, space, '-', '`' heart) at pixel x, cell row y / 8 (y is a
 * multiple of 8), from tile index tile on. Returns the number of cells used. */
u16 vk_msx_text(VDPPlane plane, u16 x, u16 y, const char *text, u16 tile, bool priority);
/* Same, the tiles go through the DMA queue at the next V-Int: buf (at least cells * 8 u32, static) must stay valid until then. */
u16 vk_msx_text_queued(VDPPlane plane, u16 x, u16 y, const char *text, u16 tile, bool priority, u32 *buf);
/* One opaque 8x8 cell of the glyph of c (a text drawn at a multiple of 8 is one such cell per character). */
void vk_msx_glyph_cell(u32 *cell, char c);
/* Writes text (opaque 8x8 cells as above) into a block of cols x rows tiles
 * (cells[(row * cols + col) * 8 + line]), at pixel (x, y) of the block; the
 * pixels outside the block are dropped. '-' is the HUD dash, '`' the heart. */
void vk_msx_text_compose(u32 *cells, u16 cols, u16 rows, s16 x, s16 y, const char *text);
/* Clears the cells a vk_msx_text call of that length at x used. */
void vk_msx_text_clear(VDPPlane plane, u16 x, u16 y, u16 length);

#endif
