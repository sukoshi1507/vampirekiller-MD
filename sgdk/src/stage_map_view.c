#include "stage_map_view.h"
#include "frontend.h"
#include "candle_view.h"
#include "stage_map.h"
#include "item_effects.h"
#include "gfx16.h"
#include "generated/screens16_md.h"

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
static VKItemMapState map_state;

/* 16-bit graphics (research/gfx16.md): the panel is a brick wall (texture on the pixels of colour 3), a
 * gilt frame around it, and PAL0 holds the map palette while the map is open. style16: the map on screen
 * is drawn that way; frame_slot[i]: the tile slot of frame tile i. */
static bool style16;
static u16 saved_palette[16];
static u8 frame_slot[VK_MAP16_FRAME_TILES];
static bool frame_ok;

/* The nibbles of w equal to v, as a 0xF mask. */
static u32 nibble_mask(u32 w, u32 v)
{
    u32 x = w ^ (v * 0x11111111UL);
    x = (x | (x >> 1) | (x >> 2) | (x >> 3)) & 0x11111111UL;
    return (x ^ 0x11111111UL) * 15;
}

static void style_tiles(void)
{
    u16 row, col, line;
    for (row = 0; row < VK_MAP_ROWS; row++)
        for (col = 0; col < VK_MAP_COLS; col++)
            for (line = 0; line < 8; line++)
            {
                u32 *w = &tiles[(row * VK_MAP_COLS + col) * 8 + line];
                if (row * 8 + line < VK_MAP_H)
                {
                    const u32 m = nibble_mask(*w, 3);
                    *w = (*w & ~m) | (vk_map16_brick[col & 1][line] & m);
                }
                else
                {
                    const u32 m = nibble_mask(*w, 0);       /* the two last lines: the frame */
                    *w |= (line == 6 ? 0x99999999UL : 0xAAAAAAAAUL) & m;
                }
            }
}

/* The ring of frame cells (rows 5 and 22, columns 2 and 29, order of vk_map16_frame: TL T TR L R BL B BR LL RL). */
static void frame_cells(u16 count)
{
    u16 i;
    frame_ok = style16 && count + VK_MAP16_FRAME_TILES <= slots;
    if (!frame_ok) return;
    for (i = 0; i < VK_MAP16_FRAME_TILES; i++) frame_slot[i] = slot[count + i];
#define FRAME(i) TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, MAP_TILE_INDEX + frame_slot[i])
    cells[1][2] = FRAME(0);
    cells[1][29] = FRAME(2);
    cells[18][2] = FRAME(5);
    cells[18][29] = FRAME(7);
    for (i = 3; i < 29; i++) { cells[1][i] = FRAME(1); cells[18][i] = FRAME(6); }
    for (i = 2; i < 17; i++) { cells[i][2] = FRAME(3); cells[i][29] = FRAME(4); }
    cells[17][2] = FRAME(8);
    cells[17][29] = FRAME(9);
#undef FRAME
}

/* Interrupts off: the tiles of the map, then the frame's. */
static void load_map_tiles(u16 count)
{
    u16 r;
    for (r = 0; r < count; r++) VDP_loadTileData(&tiles[r * 8], MAP_TILE_INDEX + slot[r], 1, CPU);
    if (frame_ok)
        for (r = 0; r < VK_MAP16_FRAME_TILES; r++)
            VDP_loadTileData(&vk_map16_frame[r * 8], MAP_TILE_INDEX + frame_slot[r], 1, CPU);
}

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
    if (style16) style_tiles();
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
    frame_cells(count);
    return count;
}

void vk_stage_map_open(void)
{
    u16 r, c;
    u16 count;
    if(vk_item_map_step(&map_state,&vk_candles,64,0,1)!=VK_ITEM_MAP_OPEN)return;
    vk_stage_map_panel((VKByte *)tiles);
    style16 = vk_gfx16;
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
    load_map_tiles(count);
    for (r = 0; r < ROWS; r++)
        VDP_setTileMapDataRow(BG_A, cells[r], (u16)(FIRST_ROW + r), 0, COLS, CPU);
    if (style16)
    {
        PAL_getColors(PAL0 * 16, saved_palette, 16);
        PAL_setPalette(PAL0, vk_map16_palette, CPU);
    }
    SYS_enableInts();

}

void vk_stage_map_close(void)
{
    u16 r;
    SYS_disableInts();
    if (style16) PAL_setPalette(PAL0, saved_palette, CPU);
    VDP_loadTileData((const u32 *)saved_tiles, MAP_TILE_INDEX, MAP_TILE_MAX, CPU);
    for (r = 0; r < ROWS; r++)
        VDP_setTileMapDataRow(BG_A, saved_cells[r], (u16)(FIRST_ROW + r), 0, COLS, CPU);
    SYS_enableInts();
    map_state.phase=0;
}

bool vk_stage_map_is_open(void)
{
    return map_state.phase!=0;
}

void vk_stage_map_tick(bool pressed)
{
    const int event=vk_item_map_step(&map_state,&vk_candles,64,0,pressed);
    u16 count,r;
    if(event==VK_ITEM_MAP_CLOSE){vk_stage_map_close();return;}
    if(event!=VK_ITEM_MAP_DRAW)return;
    map_state.phase=1; /* 95A8 increments CF38 only after the drawing returns. */
    vk_stage_map_render((VKByte *)tiles,&vk_candles,vk_candles_stage_of(&vk_game),vk_game.room,vk_player.x,vk_player.y);
    count=make_tiles();SYS_disableInts();
    load_map_tiles(count);
    for(r=0;r<ROWS;r++)VDP_setTileMapDataRow(BG_A,cells[r],FIRST_ROW+r,0,COLS,CPU);
    SYS_enableInts();
    map_state.phase=2;
}
