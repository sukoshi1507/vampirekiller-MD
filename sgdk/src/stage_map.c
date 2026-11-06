#include "stage_map.h"
#include "generated/stage_map_md.h"

#define COLOUR_PANEL 0x33            /* 9597: two pixels of colour 3 */
#define COLOUR_WALL 0x0E             /* 97D3 */
#define THUMB_W 32                   /* 9807: BC = 2016h */
#define THUMB_H 22
#define MAP_FIRST_ROW 2              /* 9793: DE = E840, the rows under the HUD */
#define BLOCK_CELLS 4                /* 9666 */

static void put(VKByte *tiles, int x, int y, VKByte colour)
{
    VKByte *b;
    x -= VK_MAP_X;
    y -= VK_MAP_Y;
    if (x < 0 || y < 0 || x >= VK_MAP_COLS * 8 || y >= VK_MAP_ROWS * 8) return;
    b = tiles + ((y >> 3) * VK_MAP_COLS + (x >> 3)) * 32 + (y & 7) * 4 + ((x & 7) >> 1);
    *b = (x & 1) ? (VKByte)((*b & 0xF0) | colour) : (VKByte)((*b & 0x0F) | (colour << 4));
}

/* 97AE: the colour of a map cell; D002 chooses what classes 3 and 4 show. */
static VKByte cell_colour(VKByte tile, VKByte d002)
{
    VKByte kind;
    if (tile >= sizeof vk_map_class) return 0;
    kind = vk_map_class[tile];
    if (kind == 1 || kind == 2) return COLOUR_WALL;
    if (kind == 3 && (d002 == 0 || d002 == 5)) return COLOUR_WALL;
    if (kind == 4 && d002 == 5) return COLOUR_WALL;
    return 0;
}

/* 967A: the place of a room on the map. */
static const VKByte *place_of(VKByte stage, VKByte room)
{
    return vk_map_place[vk_map_rooms[stage][room]];
}

/* 9609, 9793, 9807: one room, a cell is a pixel. */
static void draw_room(VKByte *tiles, const VKCandles *c, VKByte stage, VKByte room)
{
    const VKByte *map = vk_stage_room_map(stage, room);
    const VKByte *place = place_of(stage, room);
    const VKByte d002 = vk_map_d002[stage];
    int row, col;
    VKByte i;
    if (!map) return;
    for (row = 0; row < THUMB_H; row++)
        for (col = 0; col < THUMB_W; col++)
            put(tiles, place[1] + col, place[0] + row,
                cell_colour(map[(row + MAP_FIRST_ROW) * THUMB_W + col], d002));
    if (room >= VK_CANDLE_ROOMS) return;
    /* 9623, 964C: the hidden blocks still there are walls too (value 1). */
    for (i = 0; i < vk_candle_first[stage][room + 1] - vk_candle_first[stage][room]; i++)
    {
        const VKCandleDef *def = &vk_candle_defs[vk_candle_first[stage][room] + i];
        if (def->mode != VK_CANDLE_HIDDEN_BLOCK || (c->out[stage][room] & (1u << i))) continue;
        for (row = 0; row < BLOCK_CELLS; row++)
            for (col = 0; col < BLOCK_CELLS; col++)
            {
                const int r = (def->y - 0x10) / 8 + row - MAP_FIRST_ROW;
                const int k = def->x / 8 + col;
                if (r >= 0 && r < THUMB_H && k < THUMB_W)
                    put(tiles, place[1] + k, place[0] + r, COLOUR_WALL);
            }
    }
}

/* 498A with A = 0: opaque, 4 bits a pixel. */
static void copy(VKByte *tiles, const VKByte *data, int x, int y, int w, int h)
{
    int row, col;
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++)
        {
            const VKByte b = data[row * (w / 2) + col / 2];
            put(tiles, x + col, y + row, (col & 1) ? (VKByte)(b & 15) : (VKByte)(b >> 4));
        }
}

void vk_stage_map_render(VKByte *tiles, const VKCandles *c, VKByte stage, VKByte room,
                         VKByte px, VKByte py)
{
    const VKByte *place = place_of(stage, room);
    const VKByte *exit = vk_map_exit[stage];
    const int y_offset = py - 0x40;      /* 9824: signed, 9844 takes 20h off L */
    VKByte *t = tiles;
    int i, col, line, k;
    for (i = 0; i < VK_MAP_ROWS; i++)
        for (col = 0; col < VK_MAP_COLS; col++)
            for (line = 0; line < 8; line++)
                for (k = 0; k < 4; k++)
                    *t++ = i * 8 + line < VK_MAP_H ? COLOUR_PANEL : 0;
    for (i = 0; i < vk_map_room_count[stage]; i++) draw_room(tiles, c, stage, (VKByte)i);
    /* 9815: Simon, 9853: the exit of the stage. */
    copy(tiles, vk_map_dot, place[1] + px / 8, place[0] + (y_offset >> 3), 4, 4);
    copy(tiles, vk_map_arrow[exit[2]], exit[1], exit[0], 8, 7);
}
