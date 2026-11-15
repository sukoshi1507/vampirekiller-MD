#include "scene.h"
#include "frontend.h"
#include "actors.h"
#include "room_view.h"
#include "generated/intro_assets.h"
#include "psg_player.h"
#include "gfx16.h"
#include "generated/intro16_md.h"

static u16 front_timer;

/* Sound mode line of the title (SGDK font, no tile of the text budget): LEFT or RIGHT
 * switches the music between the PSG and the YM2612 (research/audio-fm.md). */
void vk_title_sound_draw(void)
{
    /* 13 characters from column 9: same centre as "PRESS START" (11 from column 10). */
    VDP_drawText(vk_sound_fm ? "SOUND < FM  >" : "SOUND < PSG >", 9, 23);
}

/* Graphics mode line of the title: DOWN switches between the MSX graphics and the 16-bit art
 * (gfx16.h). Same centre as the sound line. */
void vk_title_gfx_draw(void)
{
    VDP_drawText(vk_gfx16 ? "GRAPH < 16B >" : "GRAPH < MSX >", 9, 25);
}

void vk_show_title(void)
{
    u16 x;
    vk_show_scene(1);
    /* The ROM's keyboard prompt is adapted to the Mega Drive controller. */
    for (x = 9; x < 24; x++) VDP_setTileMapXY(BG_A, 0, x, 20);
    VDP_drawText("PRESS START", 10, 20);
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
