#include "wall_view.h"
#include "frontend.h"
#include "hud.h"
#include "msx_text.h"
#include "generated/object_tiles_md.h"

/* Frame tiles of the two records, 12 each. Castle rooms load 192 tiles of tileset 0
 * at 16 (16..207) and the HUD text starts at 224: the maps of stages 1 and 2 use no
 * tileset tile above 159 (VRAM 175), so 200..223 are free in every room where a
 * wall object can be (stage 1 room 7, stage 2 rooms 0 and 2; never hall room 0,
 * whose scene uses up to 216). */
#define WALL_TILE_INDEX 200
_Static_assert(WALL_TILE_INDEX + VK_WALL_RECORDS * VK_WALL_TILES_PER_FRAME <= HUD_TEXT_TILE_INDEX,
               "wall object tiles before the HUD text tiles");

#define WIN_COLS (VK_SHOP_W / 8)
#define WIN_ROWS (VK_SHOP_H / 8)
#define WHITE 0x0E

VKWall vk_walls;
static s8 frame_loaded[VK_WALL_RECORDS];
/* The window: its pixels (MSX colours of the room palette), its tiles and the plane
 * A cells it covers. */
static u8 pix[VK_SHOP_H][VK_SHOP_W];
static u32 win_tiles[MSX_TEXT_TILE_MAX * 8];
static u16 win_cells[WIN_ROWS][WIN_COLS];
static u16 saved[WIN_ROWS][WIN_COLS];
static u16 win_col, win_row;
static bool win_open;

void vk_wall_view_room(void)
{
    u16 i;
    for (i = 0; i < VK_WALL_RECORDS; i++) frame_loaded[i] = -1;
}

u16 vk_wall_view_sprites(u16 first)
{
    u16 n = 0, i;
    if (vk_front_mode != FRONT_INTERIOR) return 0;
    for (i = 0; i < VK_WALL_RECORDS; i++)
    {
        const int frame = vk_wall_frame(&vk_walls, (int)i);
        const u16 tile = (u16)(WALL_TILE_INDEX + i * VK_WALL_TILES_PER_FRAME);
        if (frame < 0) continue;
        if (frame_loaded[i] != frame)
        {
            /* 925E copies the frame during the pass: shown at the next frame, as the
             * sprite table that the queue sends with it. */
            VDP_loadTileData(&vk_wall_frame_tiles[frame * VK_WALL_TILES_PER_FRAME * 8], tile,
                             VK_WALL_TILES_PER_FRAME, DMA_QUEUE);
            frame_loaded[i] = (s8)frame;
        }
        /* The first 8 lines of the 32 x 32 frame are empty: Y + 8. Low priority: Simon
         * and the enemies (earlier sprites) pass in front, as over the MSX bitmap. */
        VDP_setSpriteFull((u16)(first + n), vk_walls.obj[i].x, (s16)(vk_walls.obj[i].y + 8),
                          SPRITE_SIZE(4, 3), TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, tile),
                          (u16)(first + n + 1));
        n++;
    }
    return n;
}

/* 4ACB: code of the MSX font at (x, y) of the window (codes 20h..5Fh). */
static void glyph(u8 code, u16 x, u16 y)
{
    u16 r, c;
    if (code < VK_MSX_FONT_FIRST || code >= VK_MSX_FONT_FIRST + VK_MSX_FONT_COUNT) return;
    for (r = 0; r < 8; r++)
    {
        const u32 g = vk_msx_font_tiles[(code - VK_MSX_FONT_FIRST) * 8 + r];
        for (c = 0; c < 8; c++)
        {
            const u8 v = (u8)((g >> (28 - 4 * c)) & 15);
            if (v) pix[y + r][x + c] = v;
        }
    }
}

/* Sheet object at (x, y): its 4 tiles in sprite order (TL, BL, TR, BR). */
static void icon(VKByte object, u16 x, u16 y)
{
    u16 t, r, c;
    for (t = 0; t < 4; t++)
        for (r = 0; r < 8; r++)
        {
            const u32 g = vk_object_tiles[(object - 1) * 32 + t * 8 + r];
            for (c = 0; c < 8; c++)
            {
                const u8 v = (u8)((g >> (28 - 4 * c)) & 15);
                if (v) pix[y + (t & 1) * 8 + r][x + (t >> 1) * 8 + c] = v;
            }
        }
}

static void frame(u16 x, u16 y, u16 w, u16 h)
{
    u16 k;
    for (k = 0; k < w; k++) pix[y][x + k] = pix[y + h - 1][x + k] = WHITE;
    for (k = 0; k < h; k++) pix[y + k][x] = pix[y + k][x + w - 1] = WHITE;
}

/* 9397: 80 x 32 black, white border; "OK ?" at (8, 4); heart, cross and the price
 * (4578, 2 BCD digits) at (8, 16); the arrows at (40, 16); a 19 x 20 frame at (56, 6)
 * and the object sold at (57, 8). Codes: 3F 3B 00 4C, 50 4D, 4E 4F. */
static void compose(void)
{
    u16 x, y;
    for (y = 0; y < VK_SHOP_H; y++)
        for (x = 0; x < VK_SHOP_W; x++) pix[y][x] = 0;
    frame(0, 0, VK_SHOP_W, VK_SHOP_H);
    glyph(0x3F, 8, 4);
    glyph(0x3B, 16, 4);
    glyph(0x4C, 32, 4);
    glyph(0x50, 8, 16);
    glyph(0x4D, 16, 16);
    glyph((u8)(0x20 + (vk_walls.price >> 4)), 24, 16);
    glyph((u8)(0x20 + (vk_walls.price & 15)), 32, 16);
    glyph(0x4E, 40, 16);
    glyph(0x4F, 48, 16);
    frame(56, 6, 19, 20);
    if (vk_walls.sale) icon(vk_walls.sale, 57, 8);
}

/* The window cells as tiles: blank cells use tile 0, the others are shared when
 * equal, at most MSX_TEXT_TILE_MAX (30 for the window of the hall). */
static u16 make_tiles(void)
{
    u16 count = 0, cx, cy;
    for (cy = 0; cy < WIN_ROWS; cy++)
        for (cx = 0; cx < WIN_COLS; cx++)
        {
            u32 cell[8];
            u16 r, c, k;
            bool blank = TRUE;
            for (r = 0; r < 8; r++)
            {
                u32 v = 0;
                for (c = 0; c < 8; c++) v = (v << 4) | pix[cy * 8 + r][cx * 8 + c];
                cell[r] = v;
                if (v) blank = FALSE;
            }
            win_cells[cy][cx] = 0;
            if (blank) continue;
            for (k = 0; k < count; k++)
            {
                for (r = 0; r < 8 && win_tiles[k * 8 + r] == cell[r]; r++) {}
                if (r == 8) break;
            }
            if (k == count)
            {
                if (count == MSX_TEXT_TILE_MAX) k = (u16)(count - 1);   /* never seen */
                else
                {
                    for (r = 0; r < 8; r++) win_tiles[count * 8 + r] = cell[r];
                    count++;
                }
            }
            win_cells[cy][cx] = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, MSX_TEXT_TILE_INDEX + k);
        }
    return count;
}

void vk_shop_view_open(void)
{
    VKByte wx, wy;
    u16 r, c, count;
    vk_shop_window(&vk_walls, &wx, &wy);
    /* Stages 0-2: X and Y are multiples of 8, the window falls on whole cells. */
    win_col = wx / 8;
    win_row = wy / 8;
    compose();
    count = make_tiles();
    /* 4946: the area under the window is saved (page 1, B080): here the plane A
     * cells, read back from VRAM (interrupts off: the V-Int also drives the VDP). */
    SYS_disableInts();
    for (r = 0; r < WIN_ROWS; r++)
    {
        *(vu32 *)VDP_CTRL_PORT = VDP_READ_VRAM_ADDR((u32)VDP_getPlaneAddress(BG_A, win_col, (u16)(win_row + r)));
        for (c = 0; c < WIN_COLS; c++) saved[r][c] = *(vu16 *)VDP_DATA_PORT;
    }
    /* The game is frozen behind the window: written at once, like the MSX (9397). */
    if (count) VDP_loadTileData(win_tiles, MSX_TEXT_TILE_INDEX, count, CPU);
    for (r = 0; r < WIN_ROWS; r++)
        VDP_setTileMapDataRow(BG_A, win_cells[r], (u16)(win_row + r), win_col, WIN_COLS, CPU);
    SYS_enableInts();
    win_open = TRUE;
}

void vk_shop_view_close(void)
{
    u16 r;
    if (!win_open) return;
    SYS_disableInts();
    for (r = 0; r < WIN_ROWS; r++)
        VDP_setTileMapDataRow(BG_A, saved[r], (u16)(win_row + r), win_col, WIN_COLS, CPU);
    SYS_enableInts();
    win_open = FALSE;
}
