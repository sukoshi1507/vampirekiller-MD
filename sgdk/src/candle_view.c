#include "candle_view.h"
#include "wall_view.h"
#include "actors.h"
#include "frontend.h"
#include "generated/enemy_sprites_md.h"
#include "generated/candle_flames_md.h"
#include "generated/object_tiles_md.h"
#include "generated/rooms.h"
#include "hud.h"
#include "msx_text.h"

/* Free VRAM between the HUD tiles (hud.h) and the sprite tiles of actors.h (1024).
 * Not past the sprite tiles: SGDK keeps its font at 1440..1535. */
#define DIGIT_TILE_INDEX (HUD_TILE_INDEX + 27)
#define CANDLE_TILE_INDEX (DIGIT_TILE_INDEX + 10)
#define DROP_TILE_INDEX (CANDLE_TILE_INDEX + 8)
/* Background under the candles baked in scene 6 (hall room 0), 4 tiles each. */
#define BAKED_TILE_INDEX (DROP_TILE_INDEX + VK_DROP_MAX * 4)
#define BAKED_CANDLES 2
/* HUD gold key (8EC9): sheet object 0x17 copied at (148, 12) while C700 > 0; it
 * stays until 8EC9 runs again (pickup, last key used, HUD redraw 4513). */
#define GOLD_KEY_TILE_INDEX (BAKED_TILE_INDEX + BAKED_CANDLES * 4)
#define HUD_GOLD_KEY_X 148
#define HUD_GOLD_KEY_Y 12
/* HUD item box (8EE6, 8F2D): at most four 16x16 objects at once (x B8, C8, D8, E8,
 * or the object just taken alone at D0), sheet objects copied into four slots
 * of 4 tiles: two after the stair poses, two after the type 4 enemy tiles. */
#define ITEM_SLOT_TILE_A (STAIR_TILE_INDEX + VK_PLAYER_STAIR_POSES * VK_PLAYER_TILES_PER_POSE)
#define ITEM_SLOT_TILE_B (ENEMY4_TILE_INDEX + VK_ENEMY4_TILE_COUNT)
#define ITEM_SLOTS 4
/* HUD weapon box (8E9A): object C416 + 19h copied at (80h, 0Ch), in the 5 free
 * tiles between the HUD text tiles and the death pose (actors.h). */
#define WEAPON_ICON_TILE_INDEX (HUD_TEXT_TILE_INDEX + HUD_TEXT_TILE_COUNT)
#define HUD_WEAPON_X 0x80
#define HUD_WEAPON_Y 0x0C
/* 869D-86C7 (8671, once per state 5 pass): the counter +6 of each candle goes up by
 * one; when its bits 0-1 are 0 the candle is drawn again, frame = bit 3 of the
 * counter: each frame stays 8 passes = 16 frames (executed 70 passes in three rooms,
 * tools/verify_rom_markers.py). The room load (62F5, 6310) runs 8671 once, which draws
 * the candle whatever the counter (86A1-86BB), with the frame of a room byte
 * (vk_candle_load_frame); each counter starts from the low byte of its stream address + 2
 * (vk_candle_stream, VKCandles.phase) and takes the frame over at its next multiple of 4. */

_Static_assert(WEAPON_ICON_TILE_INDEX + 4 <= DEATH_TILE_INDEX,
               "HUD weapon icon tiles before the death pose");
_Static_assert(GOLD_KEY_TILE_INDEX + 4 <= MSX_TEXT_TILE_INDEX,
               "candle tiles must end before the text tiles (msx_text.h)");
_Static_assert(ITEM_SLOT_TILE_A + 2 * 4 <= FLAME_A_CLEAR_TILE_INDEX,
               "item box slots A before the torch tiles");
_Static_assert(ITEM_SLOT_TILE_B + 2 * 4 <= 700,   /* KEY_TILE_INDEX of item_view.c */
               "item box slots B before the key tiles");
_Static_assert(CROUCH_TILE_INDEX + VK_CROUCH_TILE_COUNT <= 1440,   /* TILE_FONT_INDEX, 1536 - 96 */
               "sprite tiles must end before the SGDK font");

VKCandles vk_candles;
/* Sheet object whose tiles sit in each drop slot, 0 when none is loaded. */
static VKByte drop_loaded[VK_DROP_MAX];
/* Sheet object in each item box slot, 0 when none is loaded. */
static VKByte item_loaded[ITEM_SLOTS];
/* Sheet object at WEAPON_ICON_TILE_INDEX, 0 when none is loaded. */
static VKByte weapon_loaded;
static VKByte key_loaded;            /* object drawn in the HUD key sprite: 17, or 12 (three keys) */

static u16 item_slot_tile(u16 slot)
{
    return (u16)((slot < 2 ? ITEM_SLOT_TILE_A : ITEM_SLOT_TILE_B) + (slot & 1) * 4);
}

/* Queued (VBlank DMA) like the sprite table that shows the object: a CPU load would
 * show the new object for one frame in the slot's previous sprite. */
static void load_object(VKByte object, u16 index)
{
    VDP_loadTileData(&vk_object_tiles[(object - 1) * 32], index, 4, DMA_QUEUE);
}

/* 4x4 cells of a hidden block of the current castle room (rooms drawn from
 * vk_maps at TILE_USER_INDEX): the brick block of 86CA, or the room map tiles
 * when it is broken (885E). Screen row = Y / 8, vk_maps row = Y / 8 - 4. */
static void draw_hidden_block(const VKCandleDef *def, bool present)
{
    const u16 *map = vk_maps[vk_stages[vk_game.stage - 1].first + vk_game.room];
    u16 r, k;
    for (r = 0; r < 4; r++)
    {
        u16 cells[4];
        const u16 row = (u16)(def->y / 8 + r);
        for (k = 0; k < 4; k++)
        {
            const u16 col = (u16)(def->x / 8 + k);
            const u16 tile = present ? vk_key_block_tiles[vk_game.stage][r * 4 + k] : map[(row - 4) * 32 + col];
            cells[k] = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX + tile);
        }
        /* The MSX copies the block (HMMM) during the pass: it shows in the next frame
         * (p + 1, research/frame-timing.md). Queued for the VBlank, as the sprites. */
        VDP_setTileMapDataRow(BG_A, cells, row, (u16)(def->x / 8), 4, DMA_QUEUE_COPY);
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
    u16 cells[4];
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
        VDP_loadTileData(&vk_tiles[group->tileset][tile * 8], index, 1, DMA_QUEUE);
        cells[k] = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, index);
    }
    /* Next frame, as the MSX restore (p + 1): queued for the VBlank. */
    VDP_setTileMapDataRow(BG_A, cells, (u16)(def->y / 8), (u16)(def->x / 8), 2, DMA_QUEUE_COPY);
    VDP_setTileMapDataRow(BG_A, cells + 2, (u16)(def->y / 8 + 1), (u16)(def->x / 8), 2, DMA_QUEUE_COPY);
}

void vk_candle_view_room(void)
{
    u16 i;
    load_object(VK_OBJ_CANDLE_FRAME, CANDLE_TILE_INDEX);
    load_object(VK_OBJ_CANDLE_FRAME + 1, CANDLE_TILE_INDEX + 4);
    load_object(VK_OBJ_GOLD_KEY, GOLD_KEY_TILE_INDEX);
    key_loaded = VK_OBJ_GOLD_KEY;
    for (i = 0; i < VK_DROP_MAX; i++) drop_loaded[i] = 0;
    for (i = 0; i < ITEM_SLOTS; i++) item_loaded[i] = 0;
    weapon_loaded = 0;
    vk_candles_enter(&vk_candles, &vk_game);
    vk_candles_item_box(&vk_candles);   /* 6316: 8EE6 at each room load */
    for (i = 0; i < vk_candles.count; i++)
    {
        if (!vk_candles_lit(&vk_candles, i)) vk_candle_view_out(i);
        else if (castle_map_room() && vk_candles_def(&vk_candles, i)->mode == VK_CANDLE_HIDDEN_BLOCK)
            draw_hidden_block(vk_candles_def(&vk_candles, i), TRUE);
    }
}

static void set_sprite(u16 n, s16 x, s16 y, u16 attr)
{
    VDP_setSpriteFull((u16)(CANDLE_SPRITE_BASE + n), x, y, SPRITE_SIZE(2, 2), attr,
                      (u16)(CANDLE_SPRITE_BASE + n + 1));
}

/* The icons of the HUD: the gold key, the weapon and the item box. */
static u16 hud_sprites(void)
{
    const VKCandles *c = &vk_candles;
    u16 n = 0;
    u16 i;
    if (c->hud_key && key_loaded != c->hud_key)
    {
        load_object(c->hud_key, GOLD_KEY_TILE_INDEX);   /* 8EC9: object 12 with C701 bit 2 */
        key_loaded = c->hud_key;
    }
    if (c->hud_key)
        set_sprite(n++, HUD_GOLD_KEY_X, HUD_GOLD_KEY_Y,
                   TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, GOLD_KEY_TILE_INDEX));
    if (c->hud_weapon)
    {
        if (weapon_loaded != c->hud_weapon)
        {
            /* Queued with the sprite table, as the item box objects. */
            VDP_loadTileData(&vk_object_tiles[(c->hud_weapon - 1) * 32], WEAPON_ICON_TILE_INDEX, 4, DMA_QUEUE);
            weapon_loaded = c->hud_weapon;
        }
        set_sprite(n++, HUD_WEAPON_X, HUD_WEAPON_Y,
                   TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, WEAPON_ICON_TILE_INDEX));
    }
    /* HUD item box: the object just taken (8F2D), or the objects of C701 bits
     * 7..3 (8EE6), PAL0 sprites over the black box as the gold key. */
    {
        u16 slot = 0;
        for (i = 0; i < 5 && slot < ITEM_SLOTS; i++)
        {
            VKByte object, x;
            if (i == 0 && c->box_pick) { object = c->box_pick; x = VK_ITEM_PICK_X; }
            else if (c->box_pick || !(c->box_items & (0x80 >> i))) continue;
            else { x = vk_item_box[i][0]; object = vk_item_box[i][1]; }
            if (item_loaded[slot] != object)
            {
                /* Queued with the sprite table (VBlank DMA): a CPU load would show the
                 * new object for one frame at the slot's previous place. */
                VDP_loadTileData(&vk_object_tiles[(object - 1) * 32], item_slot_tile(slot), 4, DMA_QUEUE);
                item_loaded[slot] = object;
            }
            set_sprite(n++, x, VK_ITEM_BOX_Y,
                       TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, item_slot_tile(slot)));
            slot++;
        }
    }
    return n;
}

void vk_candle_view_hud_only(void)
{
    const u16 n = hud_sprites();
    if (n) VDP_setSpriteLink((u16)(CANDLE_SPRITE_BASE + n - 1), 0);
    /* The chain starts at sprite 0: out of the screen, it only links the icons. */
    VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(1, 1), 0, n ? CANDLE_SPRITE_BASE : 0);
    VDP_updateSprites(n ? CANDLE_SPRITE_BASE + n : 1, DMA_QUEUE);
}

/* Flame of a candle or block that was hit (research/candle-flames.md, openMSX traces; T = d->timer, the hit frame being 0, the
 * SAT of the MSX one frame after the objects). One 16x16 sprite on the candle, colour numbers of the room palette (PAL2):
 *  - object 00/15, C500 type 01 (8C7E): hidden T < 4, then the pattern F4 for 4 frames, F8 for 8, F4 for 8..., colour 8, same Y;
 *  - object 01, C800 type 1E: as the death flame (9B71): from T = 2, F4 / F8 changing at T = 6, 14, 22, 30, F8 one line lower;
 *  - the other objects, C800 type 26: the pattern EC, one line lower, from T = 2, colour 0E until T = 5 then 0E, 01, 08, 01 every
 *    2 frames (T - 4 >> 1 & 3), also while it falls. */
static void burst_sprite(u16 *n, u16 slot, const VKDrop *d)
{
    const u16 t = d->timer;
    s16 y = d->y;
    u16 tile;
    if (!d->kind)
    {
        if (t < 4) return;
        tile = DEATH_FLAME_TILE_INDEX + ((t >> 3) & 1) * 4;
    }
    else if (d->kind == VK_OBJ_SMALL_HEART)
    {
        const u16 pose = ((t + 2) >> 3) & 1;
        if (t < 2) return;
        tile = DEATH_FLAME_TILE_INDEX + pose * 4;
        y += pose;
    }
    else
    {
        static const VKByte cycle[4] = { 0, 1, 2, 1 };     /* colours 0E, 01, 08, 01 */
        const VKByte color = t < 6 ? 0 : cycle[((t - 4) >> 1) & 3];
        const VKByte loaded = (VKByte)(0xE0 | color);
        if (t < 2) return;
        if (drop_loaded[slot] != loaded)
        {
            VDP_loadTileData(&vk_candle_flame_tiles[color * 32], (u16)(DROP_TILE_INDEX + slot * 4), 4, DMA_QUEUE);
            drop_loaded[slot] = loaded;
        }
        tile = DROP_TILE_INDEX + slot * 4;
        y += 1;
    }
    set_sprite((*n)++, d->x, y, TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, tile));
}

u16 vk_candle_view_sprites(void)
{
    const VKCandles *c = &vk_candles;
    u16 n;
    u16 i;
    if (vk_front_mode != FRONT_INTERIOR && vk_front_mode != FRONT_COURTYARD) return 0;
    n = hud_sprites();
    /* Wall candles (mode 0) of the castle; courtyard torches are room_view.c flames
     * and hidden blocks (mode 3) are part of the wall. */
    if (vk_front_mode == FRONT_INTERIOR)
    {
        for (i = 0; i < c->count; i++)
        {
            const u16 frame = (c->frame >> i) & 1;
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
        if(d->state && (d->catchable&0x40)){
            VKByte sp[4];const VKByte *raw=vk_candles_fallback_record(c,i);
            if(vk_candles_fallback_sprite(c,i,sp)){
                u16 tile;
                if(sp[2]>=0xF4)tile=DEATH_FLAME_TILE_INDEX+(sp[2]==0xF8?4:0);
                else tile=LOOT_TILE_INDEX+(sp[2]==0xE8?0:sp[3]==8?4:sp[3]==1?8:12);
                set_sprite(n++,sp[0],sp[1],TILE_ATTR_FULL(PAL2,TRUE,FALSE,FALSE,tile));
            }
            if(!raw || raw[0]!=0x84)continue;
        }
        if (d->state == VK_DROP_BURST)
        {
            burst_sprite(&n, i, d);
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
    n = (u16)(n + vk_wall_view_sprites((u16)(CANDLE_SPRITE_BASE + n)));
    if (n) VDP_setSpriteLink((u16)(CANDLE_SPRITE_BASE + n - 1), 0);
    return n;
}
