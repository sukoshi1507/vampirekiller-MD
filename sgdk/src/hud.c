#include "hud.h"
#include "health.h"
#include "generated/intro_assets.h"

#define HUD_COL 7
#define HUD_COLS 9
#define HUD_ROW 1
#define HUD_ROWS 3
#define HUD_FILL 1                  /* PLAYER bar colour index in the ROM tiles */
/* Scene 6 tiles of each HUD row: left end, middle (columns 8..14), right end. */
static const u16 bar_source[HUD_ROWS][3] = {
    { 27, 28, 29 }, { 41, 42, 43 }, { 51, 52, 53 }
};
static u32 bar_tiles[HUD_ROWS * HUD_COLS * 8];
static int drawn_pixels = -1;

/* Copies the ROM tiles and clears each PLAYER fill pixel at or past 'filled'.
 * Bar pixel 0 is pixel 4 of column 7 (pixels 0..3 hold the margin and border). */
static void build_bar(int filled)
{
    const u32 *rom = vk_scenes[6].tiles;
    u16 row, col, line;
    for (row = 0; row < HUD_ROWS; row++)
        for (col = 0; col < HUD_COLS; col++)
        {
            const u16 source = bar_source[row][col == 0 ? 0 : (col == HUD_COLS - 1 ? 2 : 1)];
            u32 *out = &bar_tiles[(row * HUD_COLS + col) * 8];
            for (line = 0; line < 8; line++)
            {
                u32 pixels = rom[source * 8 + line];
                u16 p;
                for (p = 0; p < 8; p++)
                {
                    const u16 shift = 28 - 4 * p;
                    const int bar_x = col * 8 + p - 4;
                    if (((pixels >> shift) & 0xF) == HUD_FILL && bar_x >= filled)
                        pixels &= ~(0xFUL << shift);
                }
                out[line] = pixels;
            }
        }
}

void vk_hud_show(bool labels)
{
    u16 row, col;
    for (row = 0; row < HUD_ROWS; row++)
        for (col = 0; col < HUD_COLS; col++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE,
                HUD_TILE_INDEX + row * HUD_COLS + col), HUD_COL + col, HUD_ROW + row);
    if (labels)
    {
        VDP_drawText("PLAYER", 1, 1);
        VDP_drawText("ENEMY", 1, 3);
    }
    drawn_pixels = -1;
}

void vk_hud_update(int energy)
{
    const int filled = vk_health_bar_pixels(energy);
    if (filled == drawn_pixels) return;
    drawn_pixels = filled;
    build_bar(filled);
    VDP_loadTileData(bar_tiles, HUD_TILE_INDEX, HUD_ROWS * HUD_COLS, CPU);
}
