#include "msx_text.h"
#include "actors.h"
#include "generated/object_tiles_md.h"

_Static_assert(MSX_TEXT_TILE_INDEX + MSX_TEXT_TILE_MAX <= PLAYER_TILE_INDEX,
               "text tiles must end before the sprite tiles");

#define PAPER 2

static u16 cells(u16 x, u16 length)
{
    return (u16)(((x & 7) + 8 * length + 7) / 8);
}

/* Text codes are ASCII - 0x10; 0 (the space) is a blank cell. The dash of the
 * HUD (4C00 "SCORE-") is code 0x30, the heart code 0x50 (written '`' here). */
static u8 text_code(char c)
{
    if (c == ' ') return 0;
    if (c == '-') return 0x30;
    return (u8)(c - 0x10);
}

/* Row py of the glyph of c as an opaque 8-pixel word: font colours, PAPER where
 * the font pixel is 0 (the cell is copied whole, as HMMM does). */
static u32 glyph_row(char c, u16 py)
{
    const u8 code = text_code(c);
    u32 g = 0, zero;
    if (code >= VK_MSX_FONT_FIRST && code < VK_MSX_FONT_FIRST + VK_MSX_FONT_COUNT)
        g = vk_msx_font_tiles[(code - VK_MSX_FONT_FIRST) * 8 + py];
    zero = ~(g | (g >> 1) | (g >> 2) | (g >> 3)) & 0x11111111UL;
    return g | (zero << 1);   /* PAPER (2) in each empty nibble */
}

void vk_msx_text_compose(u32 *cells, u16 cols, u16 rows, s16 x, s16 y, const char *text)
{
    s16 k, py;
    for (k = 0; text[k]; k++)
    {
        const s16 cx = x + 8 * k;
        const s16 col = cx >> 3;
        const u16 shift = (u16)(cx & 7);
        for (py = 0; py < 8; py++)
        {
            const s16 sy = y + py;
            u32 g, *line;
            if (sy < 0 || sy >= 8 * (s16)rows) continue;
            g = glyph_row(text[k], (u16)py);
            line = &cells[((sy >> 3) * cols) * 8 + (sy & 7)];
            if (col >= 0 && col < (s16)cols)
                line[col * 8] |= shift ? g >> (4 * shift) : g;
            if (shift && col + 1 >= 0 && col + 1 < (s16)cols)
                line[(col + 1) * 8] |= g << (4 * (8 - shift));
        }
    }
}

u16 vk_msx_text(VDPPlane plane, u16 x, u16 y, const char *text, u16 tile, bool priority)
{
    const u16 count = cells(x, (u16)strlen(text));
    u32 data[MSX_TEXT_TILE_MAX * 8];
    u16 i;
    if (count > MSX_TEXT_TILE_MAX) return 0;
    for (i = 0; i < count * 8; i++) data[i] = 0;
    vk_msx_text_compose(data, count, 1, (s16)(x & 7), 0, text);
    VDP_loadTileData(data, tile, count, CPU);
    VDP_fillTileMapRectInc(plane, TILE_ATTR_FULL(PAL1, priority, FALSE, FALSE, tile),
                           x / 8, y / 8, count, 1);
    return count;
}

void vk_msx_text_clear(VDPPlane plane, u16 x, u16 y, u16 length)
{
    const u16 count = cells(x, length);
    u16 t;
    for (t = 0; t < count; t++) VDP_setTileMapXY(plane, 0, (u16)(x / 8 + t), y / 8);
}
