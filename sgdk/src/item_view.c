#include "item_view.h"
#include "room_view.h"
#include "actors.h"
#include "hud.h"
#include "generated/door_key_md.h"
#include "generated/enemy_sprites_md.h"
#include "generated/object_tiles_md.h"
#include "generated/rooms.h"
#include "gfx16.h"
#include "generated/hud16_md.h"
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
static void draw_block_pal(u16 pal, u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    u16 r, c;
    for (r = 0; r < h; r++)
    {
        u16 cells[DOOR_COLS > VK_HUD_BOX_W ? DOOR_COLS : VK_HUD_BOX_W];
        for (c = 0; c < w; c++) cells[c] = TILE_ATTR_FULL(pal, FALSE, FALSE, FALSE, tile + r * w + c);
        VDP_setTileMapDataRow(BG_A, cells, (u16)(y + r), x, w, DMA_QUEUE_COPY);
    }
}

static void draw_block(u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    draw_block_pal(VK_PAL_OBJ, tile, x, y, w, h);
}

/* Palette of the key cells: PAL0 when compose_key put the key over the 16-bit cells. */
static u16 key_pal;

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
    /* The opening redraws it every other turn for 80 frames: by the CPU (display on) it cost ~45 lines of the turn. */
    VDP_loadTileData(door_tiles, DOOR_TILE_INDEX, DOOR_COLS * DOOR_CELLS, vk_tiles_by_dma ? DMA : DMA_QUEUE);
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
    /* 16-bit graphics: the key over the 16-bit cells, its MSX colours taken to the closest
     * entries of the room's 16-bit palette (msx[]), drawn with PAL0 like those cells. */
    const VKGfx16Room *g16 = vk_gfx16_on ? vk_gfx16_room(game->stage, game->room) : 0;
    const u16 *map = room_map(game);
    const u32 *object = &vk_object_tiles[(VK_OBJ_WHITE_KEY - 1) * 32];
    u16 cell, row, k;
    key_pal = g16 ? PAL0 : VK_PAL_OBJ;
    for (cell = 0; cell < 4; cell++)
    {
        const u16 col = (u16)(x / 8 + (cell & 1));
        const u16 line = (u16)(y / 8 + (cell >> 1));
        const u16 i = (u16)((line - 4) * 32 + col);
        const u32 *under = g16 ? vk_room_tile(game->stage, game->room, vk_room_cell(game->stage, game->room, i))
                               : &vk_tiles[vk_stages[game->stage - 1].tileset][map[i] * 8];
        const u32 *over = &object[((cell & 1) * 2 + (cell >> 1)) * 8];
        for (row = 0; row < 8; row++)
        {
            u32 word = under[row];
            for (k = 0; k < 8; k++)
            {
                const u32 mask = (u32)0xF << (k * 4);
                const u32 c = (over[row] >> (k * 4)) & 0xF;
                if (c) word = (word & ~mask) | ((u32)(g16 ? g16->msx[c] : c) << (k * 4));
            }
            key_tiles[cell * 8 + row] = word;
        }
    }
}

/* The room map tiles back under a taken stage 2 key (the opened block, 885E). */
static void clear_key(const VKGame *game, VKByte x, VKByte y)
{
    /* The cells as drawn (16-bit graphics: gfx16.h); the key itself, composed over them by compose_key. */
    u16 cell;
    for (cell = 0; cell < 4; cell++)
    {
        const u16 col = (u16)(x / 8 + (cell & 1));
        const u16 line = (u16)(y / 8 + (cell >> 1));
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                         TILE_USER_INDEX + vk_room_cell(game->stage, game->room, (u16)((line - 4) * 32 + col))), col, line);
    }
}

static void draw_hud_box(const VKItems *items)
{
    /* 16-bit graphics: PAL1 (the box only uses its HUD entries 2, 8, 14), bevelled frame (load_hud_box). */
    draw_block_pal(VK_PAL_ITEM, (items->keys & VK_KEY_WHITE) ? HUD_KEY_TILE_INDEX : HUD_BOX_TILE_INDEX,
                   VK_HUD_BOX_CELL_X, VK_HUD_BOX_CELL_Y, VK_HUD_BOX_W, VK_HUD_BOX_H);
}

/* Loads the cells of a HUD box (x 147..180): with the 16-bit graphics its right column (x 180: cell 4 of each row, pixel 4)
 * and its bottom line (y 28: row 2, line 4) take the bevel entry (hud16_md.h) where white. */
static u32 box16[VK_HUD_BOX_W * VK_HUD_BOX_H * 8];
__attribute__((noinline)) static void load_hud_box(const unsigned long *tiles, u16 index)
{
    const u32 *src = (const u32 *)tiles;
    if (vk_gfx16)
    {
        u16 i;
        for (i = 0; i < VK_HUD_BOX_W * VK_HUD_BOX_H * 8; i++)
        {
            u32 w = (u32)tiles[i];
            if (i / 8 % VK_HUD_BOX_W == VK_HUD_BOX_W - 1 && ((w >> 12) & 15) == 14) w = (w & ~0xF000UL) | ((u32)VK_HUD16_BEVEL << 12);
            if (i >= ((VK_HUD_BOX_H - 1) * VK_HUD_BOX_W) * 8 && (i & 7) == 4)
            {
                const u32 m = (w >> 1) & (w >> 2) & (w >> 3) & ~w & 0x11111111UL;   /* the white (E) nibbles */
                w = (w & ~(m | (m << 1) | (m << 2) | (m << 3))) | (m * VK_HUD16_BEVEL);
            }
            box16[i] = w;
        }
        src = box16;
    }
    VDP_loadTileData(src, index, VK_HUD_BOX_W * VK_HUD_BOX_H, VK_TM);
}

/* The hidden key revealed by the whip: its tiles were loaded with the room, only its block is drawn (no tile
 * reload, which would overrun the frame of the break). */
void vk_item_view_key(const VKItems *items, const VKGame *game)
{
    VKByte key_x = 0, key_y = 0;
    vk_items_key_place(game, &key_x, &key_y);
    if (vk_items_key_shown(items, game))
        draw_block_pal(key_pal, KEY_TILE_INDEX, key_x / 8, key_y / 8, 2, 2);
}

void vk_item_view_hud_box(const VKItems *items)
{
    load_hud_box(vk_hud_box_tiles, HUD_BOX_TILE_INDEX);
    load_hud_box(vk_hud_key_tiles, HUD_KEY_TILE_INDEX);
    draw_hud_box(items);
}

void vk_item_view_room(const VKItems *items, const VKGame *game)
{
    VKByte key_x = 0, key_y = 0;
    const int key_room = vk_items_key_place(game, &key_x, &key_y);
    key_pal = VK_PAL_OBJ;
    if (key_room && (game->stage != 1 || vk_gfx16_on))
    {
        compose_key(game, key_x, key_y);
        VDP_loadTileData(key_tiles, KEY_TILE_INDEX, 4, VK_TM);
    }
    else VDP_loadTileData((const u32 *)vk_key_tiles, KEY_TILE_INDEX, 4, VK_TM);
    VDP_loadTileData((const u32 *)vk_key_clear_tiles, KEY_CLEAR_TILE_INDEX, 4, VK_TM);
    load_hud_box(vk_hud_box_tiles, HUD_BOX_TILE_INDEX);
    load_hud_box(vk_hud_key_tiles, HUD_KEY_TILE_INDEX);
    draw_hud_box(items);
    if (vk_items_key_shown(items, game))
        draw_block_pal(key_pal, KEY_TILE_INDEX, key_x / 8, key_y / 8, 2, 2);
    if (vk_items_door_room(game))
    {
        VKByte door_x, door_y;
        vk_items_door_place(game, &door_x, &door_y);
        draw_door(vk_items_door_lift(items));
        draw_block(DOOR_TILE_INDEX, (door_x - 4) / 8,
                   door_y / 8, DOOR_COLS, DOOR_CELLS);
    }
}

void vk_item_view_event(VKItemEvent event, const VKItems *items, const VKGame *game)
{
    if (event == VK_ITEM_KEY_TAKEN)
    {
        VKByte key_x, key_y;
        if (vk_items_key_place(game, &key_x, &key_y) && (game->stage != 1 || vk_gfx16_on)) clear_key(game, key_x, key_y);
        else draw_block(KEY_CLEAR_TILE_INDEX, VK_KEY_CELL_X, VK_KEY_CELL_Y, 2, 2);
        draw_hud_box(items);
    }
    else if (event == VK_ITEM_DOOR_OPENED) draw_hud_box(items);
    else if (event == VK_ITEM_DOOR_STEP) draw_door(vk_items_door_lift(items));
}
