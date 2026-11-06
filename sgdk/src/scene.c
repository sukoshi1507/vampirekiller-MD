#include "scene.h"
#include "frontend.h"
#include "actors.h"
#include "room_view.h"
#include "generated/intro_assets.h"
#include "psg_player.h"

static u16 front_timer;

void vk_show_title(void)
{
    u16 x;
    vk_show_scene(1);
    /* The ROM's keyboard prompt is adapted to the Mega Drive controller. */
    for (x = 9; x < 24; x++) VDP_setTileMapXY(BG_A, 0, x, 20);
    VDP_drawText("PRESS START", 10, 20);
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

void vk_begin_castle(void)
{
    vk_show_scene(2);
    VDP_loadTileData(vk_intro_sprite_tiles, PLAYER_TILE_INDEX, vk_intro_sprite_tile_count, CPU);
    show_castle_frame(0);
    vk_front_mode = FRONT_CASTLE;
    front_timer = 0;
}

bool vk_castle_tick(u16 pressed)
{
    if (pressed & BUTTON_START || ++front_timer >= 320) return TRUE;
    show_castle_frame(front_timer);
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
