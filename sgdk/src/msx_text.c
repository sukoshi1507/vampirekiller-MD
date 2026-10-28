#include "msx_text.h"
#include "actors.h"
#include "generated/object_tiles_md.h"

_Static_assert(MSX_TEXT_TILE_INDEX + MSX_TEXT_TILE_MAX <= PLAYER_TILE_INDEX,
               "text tiles must end before the sprite tiles");

#define INK 14
#define PAPER 2

static u16 cells(u16 x, u16 length)
{
    return (u16)(((x & 7) + 8 * length + 7) / 8);
}

/* Pixel of the font code at column px, row py: 0 for the space (code 0). */
static u16 glyph_pixel(u8 code, u16 px, u16 py)
{
    if (code < VK_MSX_FONT_FIRST || code >= VK_MSX_FONT_FIRST + VK_MSX_FONT_COUNT) return 0;
    return (u16)((vk_msx_font_tiles[(code - VK_MSX_FONT_FIRST) * 8 + py] >> (28 - 4 * px)) & 15);
}

u16 vk_msx_text(VDPPlane plane, u16 x, u16 y, const char *text, u16 tile, bool priority)
{
    const u16 length = (u16)strlen(text);
    const u16 shift = x & 7;
    const u16 count = cells(x, length);
    u32 data[8];
    u16 t, row, p;
    if (count > MSX_TEXT_TILE_MAX) return 0;
    for (t = 0; t < count; t++)
    {
        for (row = 0; row < 8; row++)
        {
            u32 word = 0;
            for (p = 0; p < 8; p++)
            {
                const s16 px = (s16)(t * 8 + p) - (s16)shift;
                u16 v = 0;
                if (px >= 0 && px < 8 * length)
                {
                    const char c = text[px / 8];
                    /* Text codes are ASCII - 0x10; 0 (the space) is a blank cell. */
                    const u8 code = c == ' ' ? 0 : (u8)(c - 0x10);
                    v = glyph_pixel(code, (u16)(px & 7), row) ? INK : PAPER;
                }
                word = (word << 4) | v;
            }
            data[row] = word;
        }
        VDP_loadTileData(data, (u16)(tile + t), 1, CPU);
        VDP_setTileMapXY(plane, TILE_ATTR_FULL(PAL1, priority, FALSE, FALSE, tile + t),
                         (u16)(x / 8 + t), y / 8);
    }
    return count;
}

void vk_msx_text_clear(VDPPlane plane, u16 x, u16 y, u16 length)
{
    const u16 count = cells(x, length);
    u16 t;
    for (t = 0; t < count; t++) VDP_setTileMapXY(plane, 0, (u16)(x / 8 + t), y / 8);
}
