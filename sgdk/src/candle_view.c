#include "candle_view.h"
#include "actors.h"
#include "frontend.h"
#include "generated/object_tiles_md.h"

/* Free VRAM between the sprite tiles of actors.h and the plane tables (1536). */
#define DIGIT_TILE_INDEX 1400
#define CANDLE_TILE_INDEX (DIGIT_TILE_INDEX + 10)
#define DROP_TILE_INDEX (CANDLE_TILE_INDEX + 8)
#define HUD_DIGIT_COL 24
#define CANDLE_FRAME_TICKS 8          /* PROVISIONAL: the MSX candle cadence is not measured */

_Static_assert(CROUCH_TILE_INDEX + VK_CROUCH_TILE_COUNT <= DIGIT_TILE_INDEX,
               "candle tiles must follow the sprite tiles");
_Static_assert(DROP_TILE_INDEX + VK_DROP_MAX * 4 <= 1536,
               "candle tiles must end before the plane tables");

VKCandles vk_candles;
/* Sheet object whose tiles sit in each drop slot, 0 when none is loaded. */
static VKByte drop_loaded[VK_DROP_MAX];

static void load_object(VKByte object, u16 index)
{
    VDP_loadTileData(&vk_object_tiles[(object - 1) * 32], index, 4, CPU);
}

void vk_candle_view_hearts(void)
{
    const u16 tens = vk_candles.hearts / 10;
    const u16 units = vk_candles.hearts % 10;
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, DIGIT_TILE_INDEX + tens),
                     HUD_DIGIT_COL, 0);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, DIGIT_TILE_INDEX + units),
                     HUD_DIGIT_COL + 1, 0);
}

void vk_candle_view_room(void)
{
    u16 i;
    VDP_loadTileData(vk_hud_digit_tiles, DIGIT_TILE_INDEX, 10, CPU);
    load_object(VK_OBJ_CANDLE_FRAME, CANDLE_TILE_INDEX);
    load_object(VK_OBJ_CANDLE_FRAME + 1, CANDLE_TILE_INDEX + 4);
    for (i = 0; i < VK_DROP_MAX; i++) drop_loaded[i] = 0;
    vk_candles_enter(&vk_candles, &vk_game);
    vk_candle_view_hearts();
}

static void set_sprite(u16 n, s16 x, s16 y, u16 attr)
{
    VDP_setSpriteFull((u16)(CANDLE_SPRITE_BASE + n), x, y, SPRITE_SIZE(2, 2), attr,
                      (u16)(CANDLE_SPRITE_BASE + n + 1));
}

u16 vk_candle_view_sprites(void)
{
    const VKCandles *c = &vk_candles;
    u16 n = 0;
    u16 i;
    if (vk_front_mode != FRONT_INTERIOR && vk_front_mode != FRONT_COURTYARD) return 0;
    /* Wall candles (mode 0) of the castle; courtyard torches are room_view.c flames
     * and hidden blocks (mode 3) are part of the wall. */
    if (vk_front_mode == FRONT_INTERIOR)
    {
        const u16 frame = (c->clock / CANDLE_FRAME_TICKS) & 1;
        for (i = 0; i < c->count; i++)
        {
            const VKCandleDef *def = vk_candles_def(c, i);
            if (def->mode != 0 || !vk_candles_lit(c, i)) continue;
            set_sprite(n++, def->x, def->y,
                       TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, CANDLE_TILE_INDEX + frame * 4));
        }
    }
    for (i = 0; i < VK_DROP_MAX; i++)
    {
        const VKDrop *d = &c->drop[i];
        VKByte object;
        if (d->state == VK_DROP_BURST)
        {
            /* The C800 flame of 89BF: the spark frames of the flame effect stand in. */
            const u16 spark = (u16)(VK_FLAME_FRAMES + (d->timer >> 2) % VK_SPARK_FRAMES);
            set_sprite(n++, d->x, d->y, TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE,
                       FLAME_TILE_INDEX + spark * VK_FLAME_TILES_PER_FRAME));
            continue;
        }
        object = vk_candles_drop_sprite(d, c->clock);
        if (!object) continue;
        if (drop_loaded[i] != object)
        {
            load_object(object, (u16)(DROP_TILE_INDEX + i * 4));
            drop_loaded[i] = object;
        }
        set_sprite(n++, d->x, d->y, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, DROP_TILE_INDEX + i * 4));
    }
    if (n) VDP_setSpriteLink((u16)(CANDLE_SPRITE_BASE + n - 1), 0);
    return n;
}
