#include "candle_view.h"
#include "actors.h"
#include "frontend.h"
#include "generated/object_tiles_md.h"
#include "generated/rooms.h"
#include "hud.h"

/* Free VRAM between the HUD tiles (hud.h) and the sprite tiles of actors.h (1024).
 * Not past the sprite tiles: SGDK keeps its font at 1440..1535. */
#define DIGIT_TILE_INDEX (HUD_TILE_INDEX + 27)
#define CANDLE_TILE_INDEX (DIGIT_TILE_INDEX + 10)
#define DROP_TILE_INDEX (CANDLE_TILE_INDEX + 8)
/* Background under the candles baked in scene 6 (hall room 0), 4 tiles each. */
#define BAKED_TILE_INDEX (DROP_TILE_INDEX + VK_DROP_MAX * 4)
#define BAKED_CANDLES 2
/* HUD gold key (8EC9): sheet object 0x17 copied at (148, 12) while C700 > 0. */
#define GOLD_KEY_TILE_INDEX (BAKED_TILE_INDEX + BAKED_CANDLES * 4)
#define HUD_GOLD_KEY_X 148
#define HUD_GOLD_KEY_Y 12
#define HUD_DIGIT_COL 24
#define CANDLE_FRAME_TICKS 8          /* PROVISIONAL: the MSX candle cadence is not measured */

_Static_assert(GOLD_KEY_TILE_INDEX + 4 <= PLAYER_TILE_INDEX,
               "candle tiles must end before the sprite tiles");
_Static_assert(CROUCH_TILE_INDEX + VK_CROUCH_TILE_COUNT <= 1440,   /* TILE_FONT_INDEX, 1536 - 96 */
               "sprite tiles must end before the SGDK font");

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

/* 4x4 cells of a hidden block of the current castle room (rooms drawn from
 * vk_maps at TILE_USER_INDEX): the brick block of 86CA, or the room map tiles
 * when it is broken (885E). Screen row = Y / 8, vk_maps row = Y / 8 - 4. */
static void draw_hidden_block(const VKCandleDef *def, bool present)
{
    const u16 *map = vk_maps[vk_stages[vk_game.stage - 1].first + vk_game.room];
    u16 k;
    for (k = 0; k < 16; k++)
    {
        const u16 col = (u16)(def->x / 8 + (k & 3));
        const u16 row = (u16)(def->y / 8 + (k >> 2));
        const u16 tile = present ? vk_hidden_block_tiles[k] : map[(row - 4) * 32 + col];
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX + tile),
                         col, row);
    }
}

static bool castle_map_room(void)
{
    return vk_front_mode == FRONT_INTERIOR && !(vk_game.stage == 1 && vk_game.room == 0);
}

/* Hall room 0 is shown from the ROM scene 6, whose bitmap already holds its two
 * candles. A candle put out gets back the room map tiles under it, as the MSX
 * restores the tiles saved in E480 (873A, 884A). Map row = (Y - 16) / 8, and
 * vk_maps starts at map row 2. */
void vk_candle_view_out(int i)
{
    const VKStage *group = &vk_stages[0];
    const VKCandleDef *def;
    u16 k;
    if (castle_map_room() && i >= 0 && i < vk_candles.count &&
        vk_candles_def(&vk_candles, i)->mode == VK_CANDLE_HIDDEN_BLOCK)
    {
        draw_hidden_block(vk_candles_def(&vk_candles, i), FALSE);
        return;
    }
    if (vk_front_mode != FRONT_INTERIOR || vk_game.stage != 1 || vk_game.room != 0 ||
        i < 0 || i >= BAKED_CANDLES)
        return;
    def = vk_candles_def(&vk_candles, i);
    for (k = 0; k < 4; k++)
    {
        const u16 col = (u16)(def->x / 8 + (k & 1));
        const u16 row = (u16)(def->y / 8 + (k >> 1));
        const u16 tile = vk_maps[group->first][(row - 4) * 32 + col];
        const u16 index = (u16)(BAKED_TILE_INDEX + i * 4 + k);
        VDP_loadTileData(&vk_tiles[group->tileset][tile * 8], index, 1, CPU);
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, index), col, row);
    }
}

void vk_candle_view_room(void)
{
    u16 i;
    VDP_loadTileData(vk_hud_digit_tiles, DIGIT_TILE_INDEX, 10, CPU);
    load_object(VK_OBJ_CANDLE_FRAME, CANDLE_TILE_INDEX);
    load_object(VK_OBJ_CANDLE_FRAME + 1, CANDLE_TILE_INDEX + 4);
    load_object(VK_OBJ_GOLD_KEY, GOLD_KEY_TILE_INDEX);
    for (i = 0; i < VK_DROP_MAX; i++) drop_loaded[i] = 0;
    vk_candles_enter(&vk_candles, &vk_game);
    for (i = 0; i < vk_candles.count; i++)
    {
        if (!vk_candles_lit(&vk_candles, i)) vk_candle_view_out(i);
        else if (castle_map_room() && vk_candles_def(&vk_candles, i)->mode == VK_CANDLE_HIDDEN_BLOCK)
            draw_hidden_block(vk_candles_def(&vk_candles, i), TRUE);
    }
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
    if (c->gold_keys)
        set_sprite(n++, HUD_GOLD_KEY_X, HUD_GOLD_KEY_Y,
                   TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, GOLD_KEY_TILE_INDEX));
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
