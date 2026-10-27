#include "item_view.h"
#include "actors.h"
#include "hud.h"
#include "generated/door_key_md.h"
#include "generated/enemy_sprites_md.h"

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

static void draw_block(u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    u16 i;
    for (i = 0; i < w * h; i++)
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, tile + i),
                         x + i % w, y + i / w);
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

static void draw_hud_box(const VKItems *items)
{
    draw_block((items->keys & VK_KEY_WHITE) ? HUD_KEY_TILE_INDEX : HUD_BOX_TILE_INDEX,
               VK_HUD_BOX_CELL_X, VK_HUD_BOX_CELL_Y, VK_HUD_BOX_W, VK_HUD_BOX_H);
}

void vk_item_view_room(const VKItems *items, const VKGame *game)
{
    VDP_loadTileData((const u32 *)vk_key_tiles, KEY_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *)vk_key_clear_tiles, KEY_CLEAR_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *)vk_hud_box_tiles, HUD_BOX_TILE_INDEX, VK_HUD_BOX_W * VK_HUD_BOX_H, CPU);
    VDP_loadTileData((const u32 *)vk_hud_key_tiles, HUD_KEY_TILE_INDEX, VK_HUD_BOX_W * VK_HUD_BOX_H, CPU);
    draw_hud_box(items);
    if (vk_items_key_shown(items, game))
        draw_block(KEY_TILE_INDEX, VK_KEY_CELL_X, VK_KEY_CELL_Y, 2, 2);
    if (game->area == VK_AREA_INTERIOR && game->stage == 1 && game->room == 7)
    {
        draw_door(vk_items_door_lift(items));
        draw_block(DOOR_TILE_INDEX, VK_DOOR_CELL_X, VK_DOOR_CELL_Y, DOOR_COLS, DOOR_CELLS);
    }
}

void vk_item_view_event(VKItemEvent event, const VKItems *items, const VKGame *game)
{
    (void)game;
    if (event == VK_ITEM_KEY_TAKEN)
    {
        draw_block(KEY_CLEAR_TILE_INDEX, VK_KEY_CELL_X, VK_KEY_CELL_Y, 2, 2);
        draw_hud_box(items);
    }
    else if (event == VK_ITEM_DOOR_OPENED) draw_hud_box(items);
    else if (event == VK_ITEM_DOOR_STEP) draw_door(vk_items_door_lift(items));
}
