#include "hud.h"
#include "room_view.h"
#include "health.h"
#include "generated/intro_assets.h"
#include "msx_text.h"

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
/* ROM tiles of the bar and, per line, the nibbles of its PLAYER fill pixels:
 * made once, so that an update is a few ANDs per line. Building the tiles pixel
 * by pixel took more than a frame of 68000 time, and the enemies lost a frame at
 * each contact. */
static u32 bar_full[HUD_ROWS * HUD_COLS * 8];
static u32 bar_fill[HUD_ROWS * HUD_COLS * 8];
static u32 bar_enemy[HUD_ROWS * HUD_COLS * 8];   /* ENEMY fill: colour 8, y 23..26 */
static int enemy_pixels = 64;
static bool bar_ready = FALSE;
static int drawn_pixels = -1;

static void prepare_bar(void)
{
    const u32 *rom = vk_scenes[6].tiles;
    u16 row, col, line, p;
    for (row = 0; row < HUD_ROWS; row++)
        for (col = 0; col < HUD_COLS; col++)
        {
            const u16 source = bar_source[row][col == 0 ? 0 : (col == HUD_COLS - 1 ? 2 : 1)];
            const u16 tile = (row * HUD_COLS + col) * 8;
            for (line = 0; line < 8; line++)
            {
                const u32 pixels = rom[source * 8 + line];
                u32 fill = 0, enemy = 0;
                const u16 y = (u16)(HUD_ROW * 8 + row * 8 + line);
                for (p = 0; p < 8; p++)
                    {
                    const u32 pixel = (pixels >> (28 - 4 * p)) & 0xF;
                    if (pixel == HUD_FILL) fill |= 0xFUL << (28 - 4 * p);
                    else if (pixel == 8 && y >= 23 && y <= 26) enemy |= 0xFUL << (28 - 4 * p);
                }
                bar_full[tile + line] = pixels;
                bar_fill[tile + line] = fill;
                bar_enemy[tile + line] = enemy;
            }
        }
    bar_ready = TRUE;
}

/* Copies the ROM tiles and clears each PLAYER fill pixel at or past 'filled'.
 * Bar pixel 0 is pixel 4 of column 7 (pixels 0..3 hold the margin and border). */
static void build_bar(int filled)
{
    u16 col, i;
    if (!bar_ready) prepare_bar();
    for (col = 0; col < HUD_COLS; col++)
    {
        /* Pixels p of the column with col * 8 + p - 4 >= filled: the low nibbles. */
        const int first = filled + 4 - col * 8;
        const u32 cut = first <= 0 ? 0xFFFFFFFFUL : (first >= 8 ? 0 : 0xFFFFFFFFUL >> (4 * first));
        const int efirst = enemy_pixels + 4 - col * 8;
        const u32 ecut = efirst <= 0 ? 0xFFFFFFFFUL : (efirst >= 8 ? 0 : 0xFFFFFFFFUL >> (4 * efirst));
        u16 row;
        for (row = 0; row < HUD_ROWS; row++)
        {
            const u16 tile = (row * HUD_COLS + col) * 8;
            for (i = tile; i < tile + 8; i++) bar_tiles[i] = bar_full[i] & ~(bar_fill[i] & cut) & ~(bar_enemy[i] & ecut);
        }
    }
}

/* Fields of the top line: tile range, x of the text, and the cached value. */
#define LABEL_COLS 7                /* "PLAYER" and "ENEMY", columns 0..6 of rows 1..3 */
#define SCORE_TILE (HUD_TEXT_TILE_INDEX)
#define STAGE_TILE (SCORE_TILE + 12)
#define HEART_TILE (STAGE_TILE + 9)
#define LIVES_TILE (HEART_TILE + 4)
#define LABEL_TILE (LIVES_TILE + 5)
_Static_assert(LABEL_TILE + LABEL_COLS * HUD_ROWS == HUD_TEXT_TILE_INDEX + HUD_TEXT_TILE_COUNT,
               "HUD text tiles");
_Static_assert(16 + 201 <= HUD_TEXT_TILE_INDEX && HUD_TEXT_TILE_INDEX + HUD_TEXT_TILE_COUNT <= 400,
               "HUD text tiles between the scene tiles and FLAME_A_CLEAR_TILE_INDEX");

u32 vk_hud_score;
static u32 shown_score;
static u16 shown_stage, shown_hearts, shown_lives;

void vk_hud_add_score(u32 points)
{
    vk_hud_score += points;
    if (vk_hud_score > 999999) vk_hud_score = 999999;   /* 4506: 999999 on a carry */
}

/* Two decimal digits of v at text[0..1]. */
static void digits2(char *text, u16 v)
{
    text[0] = (char)('0' + (v / 10) % 10);
    text[1] = (char)('0' + v % 10);
}

/* The label and status tiles stay in VRAM (no scene or room loads there):
 * composed once, then only their cells are set again. */
static bool tiles_ready;
void vk_hud_invalidate_tiles(void){tiles_ready=FALSE;}

static void show_labels(void)
{
    if (!tiles_ready)
    {
        u32 cells[LABEL_COLS * HUD_ROWS * 8];
        u16 i;
        for (i = 0; i < LABEL_COLS * HUD_ROWS * 8; i++) cells[i] = 0;
        /* Rows 1..3 start at y 8. */
        vk_msx_text_compose(cells, LABEL_COLS, HUD_ROWS, 8, 12 - 8, "PLAYER");
        vk_msx_text_compose(cells, LABEL_COLS, HUD_ROWS, 8, 20 - 8, "ENEMY");
        VDP_loadTileData(cells, LABEL_TILE, LABEL_COLS * HUD_ROWS, VK_TM);
    }
    VDP_fillTileMapRectInc(BG_A, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, LABEL_TILE),
                           0, HUD_ROW, LABEL_COLS, HUD_ROWS);
}

/* Changed fields go through the DMA queue (static buffers: the transfer is at the next V-Int); a forced redraw (room drawing,
 * display off) writes the tiles at once. A score change used to cost ~160 lines of CPU writes in the turn. */
static char shown_score_text[16];
static bool score_text_shown;   /* the score tiles hold shown_score_text (set by a forced draw) */
static u32 status_buf[4][16 * 8];   /* longest field: SCORE-000000, 12 cells */
static void status_text(u16 x, const char *text, u16 tile, u32 *buf, bool all)
{
    if (all) vk_msx_text(BG_A, x, 0, text, tile, TRUE);
    else vk_msx_text_queued(BG_A, x, 0, text, tile, TRUE, buf);
}

/* The four fields of the top line, drawn when forced or changed. */
static void draw_status(bool all, u32 score, u16 stage, u16 hearts, u16 lives)
{
    char text[16];
    if (all || score != shown_score)
    {
        /* score < 10^6: one 32-bit division, the digits by 16-bit ones (the 32-bit libgcc division is ~600 cycles). */
        const u32 hi = score / 1000;
        u16 a = (u16)hi, b = (u16)(score - hi * 1000);
        int i;
        strcpy(text, "SCORE-000000");
        for (i = 11; i >= 9; i--) { text[i] = (char)('0' + b % 10); b /= 10; }
        for (i = 8; i >= 6; i--) { text[i] = (char)('0' + a % 10); a /= 10; }
        if (all || !score_text_shown) status_text(8, text, SCORE_TILE, status_buf[0], all);
        else
        {
            /* x = 8: one cell per character, only the changed ones are composed and queued; the tile map is unchanged. */
            int k = 0;
            while (k < 12)
            {
                int n;
                if (text[k] == shown_score_text[k]) { k++; continue; }
                for (n = k; n < 12 && text[n] != shown_score_text[n]; n++) vk_msx_glyph_cell(&status_buf[0][n * 8], text[n]);
                VDP_loadTileData(&status_buf[0][k * 8], (u16)(SCORE_TILE + k), (u16)(n - k), DMA_QUEUE);
                k = n;
            }
        }
        strcpy(shown_score_text, text);
        score_text_shown = TRUE;
        shown_score = score;
    }
    if (all || stage != shown_stage)
    {
        strcpy(text, "STAGE-00");
        digits2(text + 6, stage);
        status_text(108, text, STAGE_TILE, status_buf[1], all);
        shown_stage = stage;
    }
    if (all || hearts != shown_hearts)
    {
        strcpy(text, "`-00");
        digits2(text + 2, hearts);
        status_text(176, text, HEART_TILE, status_buf[2], all);
        shown_hearts = hearts;
    }
    if (all || lives != shown_lives)
    {
        strcpy(text, "P-00");
        digits2(text + 2, lives);
        status_text(212, text, LIVES_TILE, status_buf[3], all);
        shown_lives = lives;
    }
}

/* Cells of the frame, PAL1 colours 8 (red) and 14 (white); the bottom of a box is the
 * top flipped vertically (y 11 and y 28 are lines 3 and 4 of their rows). */
static const u32 frame_tiles[HUD_FRAME_TILE_COUNT][8] = {
    { 0, 0, 0, 0x88888888UL },                          /* red box, top */
    { 0, 0, 0, 0xEEEEEEEEUL },                          /* white box, top */
    { 0, 0, 0, 0xE0000000UL, 0xE0000000UL, 0xE0000000UL, 0xE0000000UL, 0xE0000000UL },   /* corner, x 248 */
    { 0xE0000000UL, 0xE0000000UL, 0xE0000000UL, 0xE0000000UL,
      0xE0000000UL, 0xE0000000UL, 0xE0000000UL, 0xE0000000UL },                          /* right edge */
};

static void frame_cell(u16 tile, bool flip, u16 col, u16 row)
{
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, FALSE, flip, FALSE, HUD_FRAME_TILE_INDEX + tile), col, row);
}

void vk_hud_show_frame(void)
{
    u16 col;
    VDP_loadTileData(&frame_tiles[0][0], HUD_FRAME_TILE_INDEX, HUD_FRAME_TILE_COUNT, VK_TM);
    for (col = 16; col <= 17; col++) { frame_cell(0, FALSE, col, 1); frame_cell(0, TRUE, col, 3); }
    for (col = 23; col <= 30; col++) { frame_cell(1, FALSE, col, 1); frame_cell(1, TRUE, col, 3); }
    frame_cell(2, FALSE, 31, 1);
    frame_cell(3, FALSE, 31, 2);
    frame_cell(2, TRUE, 31, 3);
}

/* As 4513: the whole HUD at once, with the last values; vk_hud_status then
 * redraws what changed. */
void vk_hud_show(void)
{
    u16 row, col;
    if (!bar_ready) prepare_bar();   /* once, here: the first bar update of play took ~100 lines of a turn */
    for (row = 0; row < HUD_ROWS; row++)
        for (col = 0; col < HUD_COLS; col++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE,
                HUD_TILE_INDEX + row * HUD_COLS + col), HUD_COL + col, HUD_ROW + row);
    show_labels();
    if (!tiles_ready) draw_status(TRUE, shown_score, shown_stage, shown_hearts, shown_lives);
    else
    {
        /* Cells of SCORE-000000 (1..12), STAGE-00 at 108 (13..21), heart (22..25),
         * P-00 at 212 (26..30). */
        VDP_fillTileMapRectInc(BG_A, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, SCORE_TILE), 1, 0, 12, 1);
        VDP_fillTileMapRectInc(BG_A, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, STAGE_TILE), 13, 0, 9, 1);
        VDP_fillTileMapRectInc(BG_A, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, HEART_TILE), 22, 0, 4, 1);
        VDP_fillTileMapRectInc(BG_A, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, LIVES_TILE), 26, 0, 5, 1);
    }
    tiles_ready = TRUE;
    drawn_pixels = -1;
}

void vk_hud_status(u32 score, u16 stage, u16 hearts, u16 lives)
{
    draw_status(FALSE, score, stage, hearts, lives);
}

void vk_hud_enemy(int c418)
{
    int pixels = (c418 & 0xFC) / 2;
    if (c418 && !pixels) pixels = 2;
    if (pixels == enemy_pixels) return;
    enemy_pixels = pixels;
    drawn_pixels = -1;
}

void vk_hud_update(int energy)
{
    const int filled = vk_health_bar_pixels(energy);
    if (filled == drawn_pixels) return;
    drawn_pixels = filled;
    build_bar(filled);
    /* The bar changes with every hit on the boss: the CPU load of 27 tiles with the display on took ~100 lines of a turn. */
    VDP_loadTileData(bar_tiles, HUD_TILE_INDEX, HUD_ROWS * HUD_COLS, vk_tiles_by_dma ? DMA : DMA_QUEUE);
}
