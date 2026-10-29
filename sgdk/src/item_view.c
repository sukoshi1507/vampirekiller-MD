#include "item_view.h"
#include "actors.h"
#include "hud.h"
#include "generated/door_key_md.h"
#include "generated/enemy_sprites_md.h"
#include "generated/object_tiles_md.h"
#include "generated/rooms.h"
#include "candles.h"

/* Free VRAM after the enemy tiles (actors.h) and before the HUD bar (hud.h). */
#define KEY_TILE_INDEX 700
#define KEY_CLEAR_TILE_INDEX (KEY_TILE_INDEX + 4)
#define HUD_BOX_TILE_INDEX (KEY_CLEAR_TILE_INDEX + 4)
#define HUD_KEY_TILE_INDEX (HUD_BOX_TILE_INDEX + VK_HUD_BOX_W * VK_HUD_BOX_H)
#define DOOR_TILE_INDEX (HUD_KEY_TILE_INDEX + VK_HUD_BOX_W * VK_HUD_BOX_H)
#define DOOR_COLS 2
#define DOOR_CELLS (VK_DOOR_ROWS / 8)

_Static_assert(KEY_TILE_INDEX >= ENEMY4_TILE_INDEX + VK_ENEMY4_TILE_COUNT,
               "key tiles must follow the enemy tiles");
_Static_assert(DOOR_TILE_INDEX + DOOR_COLS * DOOR_CELLS <= ENEMY5_TILE_INDEX,
               "key and door tiles must end before the type 5 tiles");

static u32 door_tiles[DOOR_COLS * DOOR_CELLS * 8];

/* Queued for the VBlank (p + 1 on the MSX, research/frame-timing.md), after the
 * queued block and candle cells of candle_view.c, which the key may cover. */
static void draw_block(u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    u16 r, c;
    for (r = 0; r < h; r++)
    {
        u16 cells[DOOR_COLS > VK_HUD_BOX_W ? DOOR_COLS : VK_HUD_BOX_W];
        for (c = 0; c < w; c++) cells[c] = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, tile + r * w + c);
        VDP_setTileMapDataRow(BG_A, cells, (u16)(y + r), x, w, DMA_QUEUE_COPY);
    }
}

/* The bar rows shifted up by 'lift' pixels, black below (K3: the opening behind
 * the bar is index 0). Tiles are column-major per 8x8 cell: left then right. */
static void draw_door(int lift)
{
    u16 row, col;
    for (row = 0; row < VK_DOOR_ROWS; row++)
    {
        const u16 source = (u16)(row + lift);
        for (col = 0; col < DOOR_COLS; col++)
            door_tiles[((row / 8) * DOOR_COLS + col) * 8 + row % 8] =
                source < VK_DOOR_ROWS ? vk_door_bar_rows[source * 2 + col] : 0;
    }
    VDP_loadTileData(door_tiles, DOOR_TILE_INDEX, DOOR_COLS * DOOR_CELLS, CPU);
}

/* Stage 2 key (room 5): 8C7E copies sheet object 0x18 over the bitmap with
 * transparency, so its cells are the room map tiles under it with the object's
 * non-zero pixels on top (sheet tiles are column-major: left column first). The
 * hall key keeps the cells read in the K3 snapshots (vk_key_tiles). */
static u32 key_tiles[4 * 8];

static const u16 *room_map(const VKGame *game)
{
    return vk_maps[vk_stages[game->stage - 1].first + game->room];
}

static void compose_key(const VKGame *game, VKByte x, VKByte y)
{
    const u16 *map = room_map(game);
    const u32 *object = &vk_object_tiles[(VK_OBJ_WHITE_KEY - 1) * 32];
    u16 cell, row, k;
    for (cell = 0; cell < 4; cell++)
    {
        const u16 col = (u16)(x / 8 + (cell & 1));
        const u16 line = (u16)(y / 8 + (cell >> 1));
        const u32 *under = &vk_tiles[vk_stages[game->stage - 1].tileset][map[(line - 4) * 32 + col] * 8];
        const u32 *over = &object[((cell & 1) * 2 + (cell >> 1)) * 8];
        for (row = 0; row < 8; row++)
        {
            u32 word = under[row];
            for (k = 0; k < 8; k++)
            {
                const u32 mask = (u32)0xF << (k * 4);
                if (over[row] & mask) word = (word & ~mask) | (over[row] & mask);
            }
            key_tiles[cell * 8 + row] = word;
        }
    }
}

/* The room map tiles back under a taken stage 2 key (the opened block, 885E). */
static void clear_key(const VKGame *game, VKByte x, VKByte y)
{
    const u16 *map = room_map(game);
    u16 cell;
    for (cell = 0; cell < 4; cell++)
    {
        const u16 col = (u16)(x / 8 + (cell & 1));
        const u16 line = (u16)(y / 8 + (cell >> 1));
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                         TILE_USER_INDEX + map[(line - 4) * 32 + col]), col, line);
    }
}

static void draw_hud_box(const VKItems *items)
{
    draw_block((items->keys & VK_KEY_WHITE) ? HUD_KEY_TILE_INDEX : HUD_BOX_TILE_INDEX,
               VK_HUD_BOX_CELL_X, VK_HUD_BOX_CELL_Y, VK_HUD_BOX_W, VK_HUD_BOX_H);
}

void vk_item_view_room(const VKItems *items, const VKGame *game)
{
    VKByte key_x = 0, key_y = 0;
    const int key_room = vk_items_key_place(game, &key_x, &key_y);
    if (key_room && game->stage != 1)
    {
        compose_key(game, key_x, key_y);
        VDP_loadTileData(key_tiles, KEY_TILE_INDEX, 4, CPU);
    }
    else VDP_loadTileData((const u32 *)vk_key_tiles, KEY_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *)vk_key_clear_tiles, KEY_CLEAR_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *)vk_hud_box_tiles, HUD_BOX_TILE_INDEX, VK_HUD_BOX_W * VK_HUD_BOX_H, CPU);
    VDP_loadTileData((const u32 *)vk_hud_key_tiles, HUD_KEY_TILE_INDEX, VK_HUD_BOX_W * VK_HUD_BOX_H, CPU);
    draw_hud_box(items);
    if (vk_items_key_shown(items, game))
        draw_block(KEY_TILE_INDEX, key_x / 8, key_y / 8, 2, 2);
    if (vk_items_door_room(game))
    {
        draw_door(vk_items_door_lift(items));
        draw_block(DOOR_TILE_INDEX, VK_DOOR_CELL_X, VK_DOOR_CELL_Y, DOOR_COLS, DOOR_CELLS);
    }
}

void vk_item_view_event(VKItemEvent event, const VKItems *items, const VKGame *game)
{
    if (event == VK_ITEM_KEY_TAKEN)
    {
        VKByte key_x, key_y;
        if (vk_items_key_place(game, &key_x, &key_y) && game->stage != 1) clear_key(game, key_x, key_y);
        else draw_block(KEY_CLEAR_TILE_INDEX, VK_KEY_CELL_X, VK_KEY_CELL_Y, 2, 2);
        draw_hud_box(items);
    }
    else if (event == VK_ITEM_DOOR_OPENED) draw_hud_box(items);
    else if (event == VK_ITEM_DOOR_STEP) draw_door(vk_items_door_lift(items));
}
