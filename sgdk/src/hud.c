#include "hud.h"
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
static int drawn_pixels = -1;

/* Copies the ROM tiles and clears each PLAYER fill pixel at or past 'filled'.
 * Bar pixel 0 is pixel 4 of column 7 (pixels 0..3 hold the margin and border). */
static void build_bar(int filled)
{
    const u32 *rom = vk_scenes[6].tiles;
    u16 row, col, line;
    for (row = 0; row < HUD_ROWS; row++)
        for (col = 0; col < HUD_COLS; col++)
        {
            const u16 source = bar_source[row][col == 0 ? 0 : (col == HUD_COLS - 1 ? 2 : 1)];
            u32 *out = &bar_tiles[(row * HUD_COLS + col) * 8];
            for (line = 0; line < 8; line++)
            {
                u32 pixels = rom[source * 8 + line];
                u16 p;
                for (p = 0; p < 8; p++)
                {
                    const u16 shift = 28 - 4 * p;
                    const int bar_x = col * 8 + p - 4;
                    if (((pixels >> shift) & 0xF) == HUD_FILL && bar_x >= filled)
                        pixels &= ~(0xFUL << shift);
                }
                out[line] = pixels;
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
        VDP_loadTileData(cells, LABEL_TILE, LABEL_COLS * HUD_ROWS, CPU);
    }
    VDP_fillTileMapRectInc(BG_A, TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, LABEL_TILE),
                           0, HUD_ROW, LABEL_COLS, HUD_ROWS);
}

/* The four fields of the top line, drawn when forced or changed. */
static void draw_status(bool all, u32 score, u16 stage, u16 hearts, u16 lives)
{
    char text[16];
    if (all || score != shown_score)
    {
        u32 v = score;
        int i;
        strcpy(text, "SCORE-000000");
        for (i = 11; i >= 6; i--) { text[i] = (char)('0' + v % 10); v /= 10; }
        vk_msx_text(BG_A, 8, 0, text, SCORE_TILE, TRUE);
        shown_score = score;
    }
    if (all || stage != shown_stage)
    {
        strcpy(text, "STAGE-00");
        digits2(text + 6, stage);
        vk_msx_text(BG_A, 108, 0, text, STAGE_TILE, TRUE);
        shown_stage = stage;
    }
    if (all || hearts != shown_hearts)
    {
        strcpy(text, "`-00");
        digits2(text + 2, hearts);
        vk_msx_text(BG_A, 176, 0, text, HEART_TILE, TRUE);
        shown_hearts = hearts;
    }
    if (all || lives != shown_lives)
    {
        strcpy(text, "P-00");
        digits2(text + 2, lives);
        vk_msx_text(BG_A, 212, 0, text, LIVES_TILE, TRUE);
        shown_lives = lives;
    }
}

/* As 4513: the whole HUD at once, with the last values; vk_hud_status then
 * redraws what changed. */
void vk_hud_show(void)
{
    u16 row, col;
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

void vk_hud_update(int energy)
{
    const int filled = vk_health_bar_pixels(energy);
    if (filled == drawn_pixels) return;
    drawn_pixels = filled;
    build_bar(filled);
    VDP_loadTileData(bar_tiles, HUD_TILE_INDEX, HUD_ROWS * HUD_COLS, CPU);
}
