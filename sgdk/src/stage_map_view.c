#include "stage_map_view.h"
#include "frontend.h"
#include "candle_view.h"
#include "stage_map.h"

#define FIRST_ROW 4                  /* 9585: black from Y = 32, the HUD stays */
#define ROWS 24
#define COLS 32
#define PANEL_ROW (VK_MAP_Y / 8)
#define PANEL_COL (VK_MAP_X / 8)
/* The map takes the tiles of the room: they are saved, and it never needs more
 * (12 rooms of 16 cells at most, research/stage-map.md). The cells of the HUD also
 * use some of these tiles, which stay as they are. */
#define MAP_TILE_INDEX 16
#define MAP_TILE_MAX 208
#define TILE_WORDS 16

static u32 tiles[VK_MAP_BYTES / 4];
static u16 cells[ROWS][COLS];
static u16 saved_cells[ROWS][COLS];
static u16 saved_tiles[MAP_TILE_MAX * TILE_WORDS];
static u8 slot[MAP_TILE_MAX];        /* tile of each distinct tile of the map */
static u16 slots;
static bool map_open;

/* Interrupts off (the V-Int also drives the VDP): the tiles the HUD does not use. */
static void find_slots(void)
{
    static u8 used[MAP_TILE_MAX];
    u16 r, c;
    for (r = 0; r < MAP_TILE_MAX; r++) used[r] = 0;
    for (r = 0; r < FIRST_ROW; r++)
    {
        *(vu32 *)VDP_CTRL_PORT = VDP_READ_VRAM_ADDR((u32)VDP_getPlaneAddress(BG_A, 0, r));
        for (c = 0; c < COLS; c++)
        {
            const u16 tile = *(vu16 *)VDP_DATA_PORT & TILE_INDEX_MASK;
            if (tile >= MAP_TILE_INDEX && tile < MAP_TILE_INDEX + MAP_TILE_MAX)
                used[tile - MAP_TILE_INDEX] = 1;
        }
    }
    slots = 0;
    for (r = 0; r < MAP_TILE_MAX; r++)
        if (!used[r]) slot[slots++] = (u8)r;
}

/* Cells of equal tiles share one, in place; blank cells use tile 0. */
static u16 make_tiles(void)
{
    u16 count = 0;
    u16 row, col, k, i;
    for (row = 0; row < ROWS; row++)
        for (col = 0; col < COLS; col++) cells[row][col] = 0;
    for (row = 0; row < VK_MAP_ROWS; row++)
        for (col = 0; col < VK_MAP_COLS; col++)
        {
            const u32 *t = &tiles[(row * VK_MAP_COLS + col) * 8];
            u32 any = 0;
            for (i = 0; i < 8; i++) any |= t[i];
            if (!any) continue;
            for (k = 0; k < count; k++)
            {
                for (i = 0; i < 8 && tiles[k * 8 + i] == t[i]; i++) {}
                if (i == 8) break;
            }
            if (k == count)
            {
                if (count == slots) k = (u16)(count - 1);   /* never seen */
                else
                {
                    for (i = 0; i < 8; i++) tiles[count * 8 + i] = t[i];
                    count++;
                }
            }
            cells[PANEL_ROW - FIRST_ROW + row][PANEL_COL + col] =
                TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, MAP_TILE_INDEX + slot[k]);
        }
    return count;
}

void vk_stage_map_open(void)
{
    u16 r, c;
    u16 count;
    vk_stage_map_render((VKByte *)tiles, &vk_candles, vk_candles_stage_of(&vk_game), vk_game.room,
                        vk_player.x, vk_player.y);
    SYS_disableInts();
    find_slots();
    SYS_enableInts();
    count = make_tiles();
    SYS_disableInts();
    for (r = 0; r < ROWS; r++)
    {
        *(vu32 *)VDP_CTRL_PORT = VDP_READ_VRAM_ADDR((u32)VDP_getPlaneAddress(BG_A, 0, (u16)(FIRST_ROW + r)));
        for (c = 0; c < COLS; c++) saved_cells[r][c] = *(vu16 *)VDP_DATA_PORT;
    }
    *(vu32 *)VDP_CTRL_PORT = VDP_READ_VRAM_ADDR((u32)(MAP_TILE_INDEX * 32));
    for (r = 0; r < MAP_TILE_MAX * TILE_WORDS; r++) saved_tiles[r] = *(vu16 *)VDP_DATA_PORT;
    for (r = 0; r < count; r++) VDP_loadTileData(&tiles[r * 8], MAP_TILE_INDEX + slot[r], 1, CPU);
    for (r = 0; r < ROWS; r++)
        VDP_setTileMapDataRow(BG_A, cells[r], (u16)(FIRST_ROW + r), 0, COLS, CPU);
    SYS_enableInts();
    map_open = TRUE;
}

void vk_stage_map_close(void)
{
    u16 r;
    SYS_disableInts();
    VDP_loadTileData((const u32 *)saved_tiles, MAP_TILE_INDEX, MAP_TILE_MAX, CPU);
    for (r = 0; r < ROWS; r++)
        VDP_setTileMapDataRow(BG_A, saved_cells[r], (u16)(FIRST_ROW + r), 0, COLS, CPU);
    SYS_enableInts();
    map_open = FALSE;
}

bool vk_stage_map_is_open(void)
{
    return map_open;
}
