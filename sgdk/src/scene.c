#include "scene.h"
#include "frontend.h"
#include "actors.h"
#include "room_view.h"
#include "generated/intro_assets.h"
#include "psg_player.h"
#include "gfx16.h"
#include "generated/intro16_md.h"
#include "generated/screens16_md.h"
#include "msx_text.h"

static u16 front_timer;

/* Sound mode line of the title (SGDK font, no tile of the text budget): LEFT or RIGHT
 * switches the music between the PSG and the YM2612 (research/audio-fm.md). */
/* 16-bit graphics: the texts of the title are in the font of the 16-bit HUD (msx_text.h): 13 + 11 + 13 + 13 cells of plane A
 * from this tile (the title scene ends before: 16 + 131 tiles). Same cells as the SGDK texts and the (c) line of the image. */
#define TITLE16_TEXT_TILE_INDEX 600
#define TITLE16_COPYRIGHT_TILE (TITLE16_TEXT_TILE_INDEX)
#define TITLE16_PRESS_TILE (TITLE16_TEXT_TILE_INDEX + 13)
#define TITLE16_SOUND_TILE (TITLE16_TEXT_TILE_INDEX + 24)
#define TITLE16_GRAPH_TILE (TITLE16_TEXT_TILE_INDEX + 37)

/* One text of the 16-bit title, at pixel (x, y) (not inline: one copy of the composing buffer and code). */
__attribute__((noinline)) static void text16(u16 x, u16 y, const char *text, u16 tile)
{
    vk_msx_text(BG_A, x, y, text, tile, FALSE);
}

void vk_title_sound_draw(void)
{
    const char *text = vk_sound_fm ? "SOUND < FM  >" : "SOUND < PSG >";
    /* 13 characters from column 9: same centre as "PRESS START" (11 from column 10). */
    if (vk_gfx16) text16(72, 184, text, TITLE16_SOUND_TILE);
    else VDP_drawText(text, 9, 23);
}

/* "PRESS START" (the prompt of the ROM's keyboard is adapted to the Mega Drive controller) and the copyright line of the
 * title image (row 17, columns 9..21: MSX pixels, ':' is the (c) of the font): in the font of the mode. */
__attribute__((noinline)) static void title_texts(void)
{
    u16 x;
    for (x = 9; x < 24; x++) VDP_setTileMapXY(BG_A, 0, x, 20);
    if (vk_gfx16)
    {
        text16(72, 136, ": KONAMI 1986", TITLE16_COPYRIGHT_TILE);
        text16(80, 160, "PRESS START", TITLE16_PRESS_TILE);
    }
    else
    {
        VDP_drawText("PRESS START", 10, 20);
    }
}

/* Graphics mode line of the title: DOWN switches between the MSX graphics and the 16-bit art
 * (gfx16.h). Same centre as the sound line. */
/* 16-bit title (research/gfx16.md): the film strip of the Castlevania-MD title (one tile, PAL2) on
 * rows 0 to 3 and 13 to 14, the MSX band of rows 4 to 12 in a recoloured palette (PAL3); the logo keeps
 * its MSX tiles. title_band16: what is on screen. */
static u8 title_band16;
__attribute__((noinline)) static void title_band(void)
{
    const VKScene *scene = &vk_scenes[1];
    u16 cells[32];
    u16 row, col;
    if (vk_gfx16)
    {
        VDP_loadTileData(vk_title16_tile, TILE_USER_INDEX + scene->count, 1, CPU);
        PAL_setPalette(PAL2, vk_title16_pal2, CPU);
        PAL_setPalette(PAL3, vk_title16_pal3, CPU);
        VDP_setTileMapDataRectEx(BG_A, scene->map + 4 * 32, TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
            0, 4, 32, 9, 32, CPU);
        for (row = 0; row < 15; row++)
        {
            if (row == 4) row = 13;
            for (col = 0; col < 32; col++)
                cells[col] = (row & 1) && !(col & 1) ?
                    TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, TILE_USER_INDEX + scene->count) : 0;
            VDP_setTileMapDataRow(BG_A, cells, row, 0, 32, CPU);
        }
    }
    else
        VDP_setTileMapDataRectEx(BG_A, scene->map, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
            0, 0, 32, 18, 32, CPU);   /* rows 15..17: the (c) line of the image comes back (16-bit mode: text) */
    title_band16 = vk_gfx16;
}

void vk_title_gfx_draw(void)
{
    if (vk_gfx16) text16(72, 200, "GRAPH < 16B >", TITLE16_GRAPH_TILE);
    else VDP_drawText("GRAPH < MSX >", 9, 25);
    if (vk_gfx16 != title_band16)
    {
        title_band();
        if (vk_gfx16) PAL_setPalette(PAL1, vk_item16_palette, CPU);   /* the palette of the letters (the MSX title only reads entry 15) */
        title_texts();
        vk_title_sound_draw();
    }
}

void vk_show_title(void)
{
    vk_show_scene(1);
    title_band16 = 0;
    title_texts();
    vk_title_sound_draw();
    vk_title_gfx_draw();
    vk_front_mode = FRONT_TITLE;
    front_timer = 0;
    /* The MSX title is silent (no 509F request in states 0 to 2). */
    vk_psg_stop();
}

static void show_castle_frame(u16 frame)
{
    u16 i;
    for (i = 0; i < 8; i++)
    {
        const VKIntroSprite *sprite = &vk_intro_frames[frame][i];
        VDP_setSpriteFull(i, sprite->x, sprite->y, SPRITE_SIZE(2, 2),
            TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, PLAYER_TILE_INDEX + sprite->tile),
            i == 7 ? 0 : i + 1);
    }
    VDP_updateSprites(8, DMA_QUEUE);
}

/* 16-bit graphics: the NES introduction in the 16-bit art (generated/intro16_md.h) takes the place
 * of the MSX castle scene. It lasts as long (CASTLE_FRAMES turns, START skips it): its 458 frames
 * are spread over them. It uses the four palettes; the courtyard loads them all again. */
#define CASTLE_FRAMES 320
static void show_castle16_frame(u16 turn)
{
    const u16 frame = (u16)((u32)turn * VK_INTRO16_FRAMES / CASTLE_FRAMES);
    const VKIntro16List *list = &vk_intro16_lists[vk_intro16_frames[frame]];
    u16 i;
    for (i = 0; i < list->count; i++)
    {
        const VKIntro16Sprite *sprite = &vk_intro16_sprites[list->first + i];
        VDP_setSpriteFull(i, sprite->x, sprite->y, SPRITE_SIZE(1, 2),
            (u16)(sprite->attr + TILE_USER_INDEX), i + 1 == list->count ? 0 : i + 1);
    }
    if (!list->count) VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(1, 1), 0, 0);
    VDP_updateSprites(list->count ? list->count : 1, DMA_QUEUE);
}

static void begin_castle16(void)
{
    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    VDP_loadTileData(vk_intro16_tiles, TILE_USER_INDEX, VK_INTRO16_TILES, CPU);
    PAL_setColors(0, vk_intro16_palette, 64, CPU);
    VDP_setTileMapDataRectEx(BG_A, vk_intro16_map, TILE_USER_INDEX, 0, 0, 32, 28, 32, CPU);
    show_castle16_frame(0);
    VDP_setEnable(TRUE);
}

void vk_begin_castle(void)
{
    if (vk_gfx16)
    {
        begin_castle16();
        vk_front_mode = FRONT_CASTLE;
        front_timer = 0;
        return;
    }
    vk_show_scene(2);
    VDP_loadTileData(vk_intro_sprite_tiles, PLAYER_TILE_INDEX, vk_intro_sprite_tile_count, CPU);
    show_castle_frame(0);
    vk_front_mode = FRONT_CASTLE;
    front_timer = 0;
}

bool vk_castle_tick(u16 pressed)
{
    if (pressed & BUTTON_START || ++front_timer >= CASTLE_FRAMES) return TRUE;
    if (vk_gfx16) show_castle16_frame(front_timer);
    else show_castle_frame(front_timer);
    return FALSE;
}

static void logo_palette(u16 level)
{
    /* DMA_QUEUE keeps the pointer until the VBlank flush: the buffer must be static. */
    static u16 colors[16];
    u16 i;
    for (i = 0; i < 16; i++)
    {
        const u16 source = vk_scenes[0].palette[i];
        u16 r = ((source >> 1) & 7) * level / 16;
        u16 g = ((source >> 5) & 7) * level / 16;
        u16 b = ((source >> 9) & 7) * level / 16;
        colors[i] = (b << 9) | (g << 5) | (r << 1);
    }
    PAL_setPalette(PAL0, colors, DMA_QUEUE);
}

void vk_logo_start(void)
{
    vk_front_mode = FRONT_LOGO;
    front_timer = 0;
    vk_show_scene(0);
    logo_palette(0);
}

void vk_logo_tick(u16 pressed)
{
    front_timer++;
    if (pressed & BUTTON_START || front_timer >= 120) vk_show_title();
    else if (front_timer <= 16) logo_palette(front_timer);
    else if (front_timer > 104) logo_palette(120 - front_timer);
}
