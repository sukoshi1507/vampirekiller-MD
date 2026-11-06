#include "candles.h"

/* Whip reach from Simon (8450): 20, or 24 with the chain whip (C416 = 1); the
 * candle is touched when the whip point is within 20 (24) of the candle centre
 * X + 8, and C425 - B - Y is in [0, 20), B = 18 standing, 10 crouched. */
#define WHIP_REACH 20
#define WHIP_REACH_CHAIN 24
#define WHIP_BODY_STAND 18
#define WHIP_BODY_CROUCH 10
#define WHIP_HEIGHT 20
/* Pickup (847A): |C427 - (X + 8)| < 13 and C425 - 2 - Y in [0, 42) standing,
 * C425 - 8 - Y in [0, 34) in a jump, C425 - 2 - Y in [0, 34) crouched. */
#define PICK_HALF_W 13
#define PICK_HEIGHT 42
#define PICK_HEIGHT_LOW 34
/* Drop timings: see step_drop (flames), then the small heart 1 px every other frame and its sway. On the floor the object stays
 * 8 x 32 frames (8BC0: -1 every 16 passes, trace), keys and chest for good. */
#define FLAME_NONE_FRAMES 32         /* objects 0 and 0x15: 16 calls of the loop of C500 objects, which runs once a pass */
#define FLAME_HEART_FRAMES 33        /* object 1: flame 1E, then the heart */
#define FLAME_HOLD_FRAMES 43         /* other objects: flame 26 still, then it falls 8 px every other frame */
/* 8BCC: +6 counts 0 to 20h once a pass (2 frames) while the content of a chest floats: 64
 * frames (openMSX trace of the 31 chests of stages 1-9, research/chests-stage1-9.md: type 05
 * to type 02 in 63-64 frames every time). Then 8AF8/8B70 take 4 frames to move it down 8 px
 * (type 03), 2 to fix it (04) and 2 to make it pickable (84): 02 at t, 03 at t + 4, 04 at
 * t + 6, 84 at t + 8, taken at t + 9 (sound 12). */
#define CHEST_WAIT_FRAMES 64
#define CHEST_DROP_FRAMES 4          /* type 02 -> 03: the 8 px fall */
#define CHEST_REST_FRAMES 9          /* type 02 -> 84 at t + 8, taken on the next frame (sound 12 at t + 9) */
#define BLOCK_OBJECT_DY 16           /* 89E3, 8A13: a block's object stands at Y + 16 */
#define BLOCK_SPECIAL 0xC0           /* 8814: +9 & C0 = C0 goes to 9179, not a chest */
#define REST_TICKS 8
#define FLOOR_LIMIT 0xDB
#define HEART_VX0 0x0200
#define HEART_AX0 0x0020
#define HEART_SWAY 0x20
#define HEARTS_MAX 99

const VKByte vk_hidden_block_tiles[16] = {
    0x01, 0x02, 0x01, 0x02, 0x09, 0x0B, 0x0A, 0x09,
    0x01, 0x02, 0x01, 0x02, 0x09, 0x0B, 0x0A, 0x09
};

const VKByte vk_item_box[5][2] = {
    { 0xE8, 0x0F }, { 0xD8, 0x0A }, { 0xC8, 0x04 }, { 0xC8, 0x03 }, { 0xB8, 0x1E }
};

static const VKByte *room_map(const VKGame *game)
{
    return vk_game_collision(game);
}

void vk_candles_begin(VKCandles *c)
{
    int s, r;
    c->stage = c->room = 0xFF;
    c->first = c->count = 0;
    c->lit = 0;
    c->clock = 0;
    for (r = 0; r < VK_DROP_MAX; r++) c->drop[r].state = VK_DROP_NONE;
    c->hearts = 0;
    c->gold_keys = 0;
    c->triple_key = 0;
    c->weapon = 0;
    c->hud_key = 0;
    c->c431 = 0;
    c->picked = 0;
    c->c701 = 0;                     /* 44C6: the RAM is cleared, C5E5/C5E6 too */
    c->c70f = 0;
    c->box_items = c->box_pick = 0;
    c->box_timer_on = c->box_timer = c->box_phase = 0;
    c->hud_weapon = 0;
    c->c702 = 0;
    for (s = 0; s < VK_CANDLE_STAGES; s++)
        for (r = 0; r < VK_CANDLE_ROOMS; r++)
            c->out[s][r] = c->taken[s][r] = 0;
}

void vk_candles_reset_stage(VKCandles *c, VKByte stage)
{
    int r;
    if (stage >= VK_CANDLE_STAGES) return;
    for (r = 0; r < VK_CANDLE_ROOMS; r++) c->out[stage][r] = c->taken[stage][r] = 0;
    c->stage = c->room = 0xFF;
}

void vk_candles_new_life(VKCandles *c, VKByte stage)
{
    vk_candles_reset_stage(c, stage);
    c->weapon = 0;
    c->gold_keys = 0;
    c->triple_key = 0;
    c->c701 &= 0x80;                 /* 62D0 -> 70DC */
}

void vk_candles_death(VKCandles *c)
{
    c->gold_keys = 0;                /* 70F7 */
    c->triple_key = 0;
    c->c431 = 0;                     /* 70E8 */
    c->c702 = 0;                     /* 70F4: books 10 and 11 are lost */
    c->c701 &= 0x80;                 /* 70DC: keys, 0A, 03/04 and 1E are lost, 0F stays */
}

int vk_candles_map_use(VKCandles *c)
{
    if (!(c->c701 & VK_ITEM_BIT_0F) || !c->c70f) return 0;
    if (--c->c70f == 0)
    {
        c->c701 &= (VKByte)~VK_ITEM_BIT_0F;
        vk_candles_item_box(c);
    }
    return 1;
}

void vk_candles_item_box(VKCandles *c)
{
    /* 8F4A clears the box (B8, 0C, 64 x 16, colour 0), then 8EE9 draws the object
     * of each bit of C701 from bit 7 (8F05, table 8F19). */
    c->box_pick = 0;
    c->box_items = (VKByte)(c->c701 & VK_ITEM_BITS);
}

void vk_candles_armor_out(VKCandles *c)
{
    c->c701 &= (VKByte)~VK_ITEM_BIT_03;      /* 819E: RES 4,(C701) */
    vk_candles_item_box(c);                  /* 81A4 */
}

/* 8F23, called by 8D30 before the effect: any object but 01, 17 and up shows
 * alone in the box (8F4A, then the object C419 at D0, 12), and C5E5 = FF,
 * C5E6 = 20h start 8F55. */
static void box_show_pick(VKCandles *c, VKByte kind)
{
    if (kind == VK_OBJ_SMALL_HEART || (kind >= VK_OBJ_GOLD_KEY && kind != 0x1E)) return;
    c->box_timer_on = 1;
    c->box_timer = VK_ITEM_PICK_TICKS;
    c->box_phase = 0;                /* the pickup pass (88D8 after 8A4A): next pass in 2 frames */
    c->box_items = 0;
    c->box_pick = kind;
}

/* 8F55, called by 8A4A (5C77) at each pass of state 5, one every 2 frames: with
 * C5E5 = FF, C5E6 drops at each pass whose C003 & 3F is not 0, and 8EE6 draws the
 * box again when it reaches 0. C5E5 is never cleared: C5E6 goes on from 0 (FF,
 * then down), so 8EE6 runs again every 256 counted passes (same objects drawn).
 * openMSX trace (research/castle-candles.md 9.8): object alone from the frame after
 * the pickup, 8EE6 33 passes (66 frames) later when C003 crossed a multiple of 40h,
 * again 260 passes (519 frames) later. The passes are counted from the pickup
 * (box_phase); c->clock / 2 stands for C003 (PROVISIONAL phase of the skipped pass). */
static void box_step(VKCandles *c)
{
    if (!c->box_timer_on) return;
    c->box_phase ^= 1;
    if (c->box_phase || ((c->clock >> 1) & 0x3F) == 0) return;
    if (--c->box_timer == 0) vk_candles_item_box(c);
}

void vk_candles_hud(VKCandles *c)
{
    /* 8EC9: object 0x17 at (148, 12) while C701 & 6 and C700 > 0. In stages 0-2
     * no object 0x12 (C700 = 3, C701 bit 2) exists, so C701 bit 1 is set exactly
     * while C700 > 0 (8E50 sets both, 8C23 clears the bit when C700 reaches 0). */
    c->hud_key = c->gold_keys ? (c->triple_key ? 0x12 : VK_OBJ_GOLD_KEY) : 0;   /* 8EDC: bit 2 of C701 */
    c->hud_weapon = c->weapon ? (VKByte)(c->weapon + 0x19) : 0;   /* 4528 -> 8E9A */
}

void vk_candles_weapon_lost(VKCandles *c)
{
    c->weapon = 0;                   /* 8E93 */
    c->hud_weapon = 0;               /* 8E9A with C416 = 0: 8979 clears the box */
}

void vk_candles_relight(VKCandles *c)
{
    c->lit = (VKByte)((1u << c->count) - 1);
    if (c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS) c->out[c->stage][c->room] = 0;
    c->stage = c->room = 0xFF;   /* the next vk_candles_enter writes the blocks again */
}

VKByte vk_candles_stage_of(const VKGame *game)
{
    return game->area == VK_AREA_COURTYARD ? 0 : game->stage;
}

static VKDrop *free_drop(VKCandles *c)
{
    int i;
    for (i = 0; i < VK_DROP_MAX; i++)
        if (c->drop[i].state == VK_DROP_NONE) return &c->drop[i];
    return 0;
}

void vk_candles_enter(VKCandles *c, const VKGame *game)
{
    const VKByte stage = vk_candles_stage_of(game);
    int i;
    if (stage == c->stage && game->room == c->room) return;
    c->stage = stage;
    c->room = game->room;
    for (i = 0; i < VK_DROP_MAX; i++) c->drop[i].state = VK_DROP_NONE;
    if (stage >= VK_CANDLE_STAGES || game->room >= VK_CANDLE_ROOMS)
    {
        c->first = c->count = c->lit = 0;
        return;
    }
    c->first = vk_candle_first[stage][game->room];
    c->count = (VKByte)(vk_candle_first[stage][game->room + 1] - c->first);
    c->lit = (VKByte)(((1u << c->count) - 1) & ~c->out[stage][game->room]);
    /* 873A then 86CA: each hidden block still there is written into the room map
     * (display and collision). PROVISIONAL: the MSX does it 16 frames after the
     * room is entered (fishman runs), here at once. */
    if (vk_game_room_map(game))
        for (i = 0; i < c->count; i++)
            if (vk_candles_lit(c, i) && vk_candles_def(c, i)->mode == VK_CANDLE_HIDDEN_BLOCK)
                vk_game_room_block(game, vk_candles_def(c, i)->x, vk_candles_def(c, i)->y,
                                   vk_key_block_tiles[stage]);
    for (i = vk_fixed_object_first[stage][game->room];
         i < vk_fixed_object_first[stage][game->room + 1]; i++)
    {
        const int bit = i - vk_fixed_object_first[stage][game->room];
        const VKCandleDef *def = &vk_fixed_object_defs[i];
        VKDrop *d;
        if (c->taken[stage][game->room] & (1u << bit)) continue;
        d = free_drop(c);
        if (!d) break;
        d->state = VK_DROP_REST;
        d->x = def->x;
        d->y = def->y;
        d->kind = def->kind;
        d->content = def->content;
        d->timer = 0xFF;
        d->fixed = (VKByte)(bit + 1);
    }
}

const VKCandleDef *vk_candles_def(const VKCandles *c, int i)
{
    return &vk_candle_defs[c->first + i];
}

int vk_candles_lit(const VKCandles *c, int i)
{
    return i >= 0 && i < c->count && (c->lit & (1u << i));
}

int vk_candle_whip_touch(const VKCandleDef *def, const VKWhip *whip, int weapon)
{
    const int reach = weapon == 1 ? WHIP_REACH_CHAIN : WHIP_REACH;
    const int point = whip->facing ? whip->x - reach : whip->x + reach;
    const int dx = point - (def->x + 8);
    const int dy = whip->y - (whip->crouched ? WHIP_BODY_CROUCH : WHIP_BODY_STAND) - def->y;
    if (!vk_whip_active(whip)) return 0;   /* 7FB7: C422 = 3 only */
    if (weapon >= 2) return 0;             /* 7D7E: no whip test for a thrown weapon */
    return dx > -reach && dx < reach && dy >= 0 && dy < WHIP_HEIGHT;
}

/* 8B70: on the floor, 8 x 32 frames (-1 every 16 passes), or for good for the keys and the chest. */
static VKByte rest_timer(VKByte kind)
{
    return (VKByte)(kind - VK_OBJ_GOLD_KEY) < 3 ? 0xFF : REST_TICKS;
}

/* 89F0: an object put straight on the floor at Y + 16 of a hidden block (type 83,
 * no flame, no fall). */
static VKDrop *block_object(VKCandles *c, const VKCandleDef *def, VKByte kind)
{
    VKDrop *d = free_drop(c);
    if (!d) return 0;
    d->state = VK_DROP_REST;
    d->x = def->x;
    d->y = (VKByte)(def->y + BLOCK_OBJECT_DY);
    d->kind = kind;
    d->timer = rest_timer(kind);
    d->fixed = 0;
    d->content = 0;
    return d;
}

/* 8A29: a weapon Simon already holds comes out as a small heart. */
static VKByte drop_kind(const VKCandles *c, VKByte kind)
{
    if (kind >= VK_OBJ_CHAIN_WHIP && kind == c->weapon + 0x19) return VK_OBJ_SMALL_HEART;
    return kind;
}

int vk_candles_hit(VKCandles *c, int i)
{
    const VKCandleDef *def;
    VKDrop *d;
    VKByte kind;
    if (!vk_candles_lit(c, i)) return VK_CANDLE_EVENT_NONE;
    def = vk_candles_def(c, i);
    c->lit &= (VKByte)~(1u << i);
    /* 8814 -> 8A13: a block of object 0x1F holds a chest (type 83, +6 FF) with the
     * content +9 & 1F at Y + 16. 8874 is skipped: the stage stream flag goes to
     * the chest (+14/+15) and is cleared when it is opened (8C0B), so the block
     * is back if Simon leaves the room first. */
    if (def->mode == VK_CANDLE_HIDDEN_BLOCK && def->kind == VK_OBJ_BLOCK_CONTENT &&
        (def->content & BLOCK_SPECIAL) != BLOCK_SPECIAL)
    {
        d = block_object(c, def, VK_OBJ_CHEST);
        if (d)
        {
            d->content = (VKByte)(def->content & 0x1F);
            d->fixed = (VKByte)(0x80 | (i + 1));
        }
        return VK_CANDLE_EVENT_HIT;
    }
    /* Hidden block with the white key (883E -> 89FD): the key of items.c appears.
     * As for a chest, 8874 is skipped: the stream flag pointer goes to the key
     * (+14/+15) and is cleared only when it is taken (8C0B), so the block is back
     * if Simon leaves the room first (vk_candles_white_key_taken). */
    if (def->mode == VK_CANDLE_HIDDEN_BLOCK && def->kind == VK_OBJ_WHITE_KEY)
        return VK_CANDLE_EVENT_HIT | VK_CANDLE_EVENT_WHITE_KEY;
    /* 8814 -> 9179: a 0x1F block with +9 & C0 = C0 holds a wall object (wall.c).
     * 8874 is skipped: the stream flag stays, so the block is back at the next room
     * load, until 9358 clears it (vk_candles_block_gone). */
    if (def->mode == VK_CANDLE_HIDDEN_BLOCK && def->kind == VK_OBJ_BLOCK_CONTENT)
        return VK_CANDLE_EVENT_HIT | VK_CANDLE_EVENT_WALL;
    if (c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS)
        c->out[c->stage][c->room] |= (VKByte)(1u << i);
    kind = drop_kind(c, def->kind);
    /* A candle of object 0 or 0x15 (898F): no C800 flame (8996), but 8A37 still
     * makes a C500 object of type 01 with +5 = 0, drawn as the red flame F4/F8
     * (8C7E) for 16 frames; then 8AA7 removes it (8B1B): nothing to take. 0x15
     * also makes, at the end (8AD0), the enemy object of 8AF0[D002 - 1] at (X, Y + 16):
     * the object 1A in the stages 4 to 6, 1C in the stages 7 to 9 (VK_CANDLE_EVENT_GHOST, enemies.c; 1B of the stage 10 is not ported), none with
     * D002 = 0 (stages 0-3). The burst stands in for the red flame. */
    if ((kind == 0 || kind == VK_OBJ_NOTHING) && def->mode < 2)
    {
        d = free_drop(c);
        if (!d) return VK_CANDLE_EVENT_HIT;
        d->state = VK_DROP_BURST;
        d->x = def->x;
        d->y = def->y;
        d->kind = 0;
        d->timer = 0;
        d->fixed = 0;
        d->content = (VKByte)(kind == VK_OBJ_NOTHING ? VK_OBJ_NOTHING : 0);   /* +4 of the C500 object: 8AA7 tests it */
        return VK_CANDLE_EVENT_HIT;
    }
    /* A block of object 0x15 (openMSX trace, stage 8 room 0, block (96, 112): the flame at Y + 16 from 925 to 956, then the object 1C
     * at 16 px lower, 956): the same flame and the same enemy object as a candle. */
    if (kind == VK_OBJ_NOTHING && def->mode == VK_CANDLE_HIDDEN_BLOCK)
    {
        d = free_drop(c);
        if (!d) return VK_CANDLE_EVENT_HIT;
        d->state = VK_DROP_BURST;
        d->x = def->x;
        d->y = (VKByte)(def->y + BLOCK_OBJECT_DY);
        d->kind = 0;
        d->timer = 0;
        d->fixed = 0;
        d->content = VK_OBJ_NOTHING;
        return VK_CANDLE_EVENT_HIT;
    }
    /* Blocks of 0 give nothing here. */
    if (kind == 0 || kind == VK_OBJ_NOTHING ||
        kind > VK_OBJECT_SHEET_COUNT - 2)
        return VK_CANDLE_EVENT_HIT;
    /* 87FA -> 89E3: any other object of a hidden block is put on the floor at once. */
    if (def->mode == VK_CANDLE_HIDDEN_BLOCK)
    {
        block_object(c, def, kind);
        return VK_CANDLE_EVENT_HIT;
    }
    d = free_drop(c);
    if (!d) return VK_CANDLE_EVENT_HIT;
    d->state = VK_DROP_BURST;
    d->x = def->x;
    d->y = def->y;
    d->kind = kind;
    d->timer = 0;
    d->fixed = 0;
    d->content = 0;
    d->xfrac = (unsigned short)(def->x << 8);
    d->vx = HEART_VX0;
    d->ax = HEART_AX0;
    return VK_CANDLE_EVENT_HIT;
}

void vk_candles_drop(VKCandles *c, VKByte x, VKByte y, VKByte kind)
{
    VKDrop *d = free_drop(c);        /* 8A37: no free C500 slot, no object */
    if (!d) return;
    d->state = VK_DROP_BURST;
    d->x = x;
    d->y = y;
    d->kind = kind;
    d->timer = 0;
    d->fixed = 0;
    d->content = 0;
    d->xfrac = (unsigned short)(x << 8);
    d->vx = HEART_VX0;
    d->ax = HEART_AX0;
}

static void heart_begin(VKCandles *c, VKDrop *d, int X);

void vk_candles_death_drop(VKCandles *c, VKByte kind, int x, int y)
{
    VKDrop *d;
    if (x < 8 || x > 0xF0 || y < 16 || y > 0xE3) return;       /* 99E5 would already have removed an object out of these */
    d = free_drop(c);                /* 8A37: no free C500 slot, no object */
    if (!d) return;
    d->x = (VKByte)(x - 8);
    d->y = (VKByte)(y - 16);
    d->kind = kind;
    d->timer = 0;
    d->fixed = 0;
    d->content = 0;
    d->catchable = 0;
    if (kind == VK_OBJ_SMALL_HEART) heart_begin(c, d, x);
    else d->state = VK_DROP_BURST;
}

/* 8AF8: lands when the tile under X + 8, Y + 16 is a floor (tile 1..9) or past
 * Y + 16 = DB. */
static int drop_landed(const VKByte *map, const VKDrop *d)
{
    const int probe_y = d->y + 16;
    if (probe_y > FLOOR_LIMIT) return 1;
    return (VKByte)(vk_sample_tile(map, (VKByte)(d->x + 8), (VKByte)probe_y) - 1) < 9;
}

/* 8B70: snapped to the 8 px grid, then on the floor. */
static void land_drop(VKDrop *d)
{
    d->x = (VKByte)((d->x + 4) & 0xF8);
    d->y = (VKByte)((d->y + 4) & 0xF8);
    d->state = VK_DROP_REST;
    d->timer = rest_timer(d->kind);
}

/* The falling heart is the C800 object 24 (9BD9, then 99B9 and 99E5), at (X, Y) = (x + 8, y + 16) of its drop, X in d->xfrac (8.8). One update
 * per pass (every other frame, in the phase of its creation, bit 1 of catchable): the landing test (7B98: the tile at (X, Y), then at (X - 10, Y), 7C5E:
 * the stage limit), +12 (timer) + 1, vx - ax (the sway: ax flips every 32 updates, 9BF0), X + vx, Y + 1/2 px (vy = 80h: y + 1 at every even
 * +12), then 99E5: out for Y >= E4, X >= F1 or X < 7. Landing (9C03): the object 83 at (X & F8, (Y - 16) & F8). Traced in 40 hearts of stages 1-9
 * (research/death-flame.md): X, Y, vx and +12 of every pass, 36 landings and 14 exits. */
static void heart_begin(VKCandles *c, VKDrop *d, int X)
{
    d->state = VK_DROP_FALL;
    d->catchable = (VKByte)(1 | ((c->clock & 1) << 1));
    d->vx = HEART_VX0;
    d->ax = HEART_AX0;
    d->xfrac = (unsigned short)((X << 8) + d->vx);       /* 5F1D then 99B9 in the same pass: 2 px, half a pixel down */
    d->timer = 1;
    d->x = (VKByte)((d->xfrac >> 8) - 8);
}

static int heart_landed(const VKCandles *c, const VKByte *map, int X, int Y)
{
    return vk_tile_solid(vk_sample_tile(map, (VKByte)X, (VKByte)Y), c->stage, 0) ||
           vk_tile_solid(vk_sample_tile(map, (VKByte)(X - 10), (VKByte)Y), c->stage, 0);
}

/* 9C5D -> 7B98 for the flame 26 (X = candle + 8, Y = candle + 16 + the fall): the same two tiles, (X, Y) and (X - 10, Y), 7D2F reading the row of
 * Y - 16, and the stage limit of 7C5E; no floor limit (99E5 removes the flame at Y >= E4, traced: 24 flames fell out of the room without an object).
 * In the coordinates of the drop (x, y): (x + 8, y + 16), (x - 2, y + 16). */
static int flame_landed(const VKCandles *c, const VKByte *map, const VKDrop *d)
{
    return heart_landed(c, map, d->x + 8, d->y + 16);
}

static void heart_pass(VKCandles *c, VKDrop *d, const VKByte *map)
{
    int X = d->xfrac >> 8;
    if (heart_landed(c, map, X, d->y + 16))
    {
        d->x = (VKByte)(X & 0xF8);
        d->y = (VKByte)(d->y & 0xF8);
        d->state = VK_DROP_REST;
        d->catchable = 0;
        d->timer = rest_timer(d->kind);
        return;
    }
    d->timer++;
    d->vx = (short)(d->vx - d->ax);
    d->xfrac = (unsigned short)(d->xfrac + d->vx);
    if (!(d->timer & 1)) d->y++;
    if (d->timer == HEART_SWAY) { d->timer = 0; d->ax = (short)-d->ax; }
    X = d->xfrac >> 8;
    d->x = (VKByte)(X - 8);
    if (d->y + 16 >= 0xE4 || X >= 0xF1 || X < 7) d->state = VK_DROP_NONE;
}

static void step_drop(VKCandles *c, VKDrop *d, const VKByte *map)
{
    if (d->state == VK_DROP_WAIT)
    {
        /* 8BCC: after 32 passes (64 frames) the content falls (type 02); nothing (0) goes (8B1B). */
        if (++d->timer >= CHEST_WAIT_FRAMES)
        {
            d->state = d->kind ? VK_DROP_FALL : VK_DROP_NONE;
            d->timer = 0;
            d->catchable = 0;
        }
        return;
    }
    if (d->state == VK_DROP_BURST)
    {
        /* Flames, openMSX traces (research/candle-flames.md; the hit frame is one before the first frame of the flame, hence the +1):
         * objects 0 and 0x15: the C500 flame (8A99 counts 16 calls of a loop that runs once a pass) is put out 31 frames after the hit,
         * 8AA7 removes it (8B1B) or 8AD0 makes the enemy object; object 1: the C800 flame 1E is there 31 frames, then the heart (type 24)
         * sways down; every other object: the C800 flame 26 (89BF, 5F1F) stays 41 frames, then falls 8 px every other frame until it
         * lands (5F1F makes no C500 object, the object is made where the flame lands). */
        ++d->timer;
        if (!d->kind)
        {
            if (d->timer < FLAME_NONE_FRAMES) return;
            /* 8AA7: object 0 goes (8B1B); 8AD0: object 0x15 goes too, and makes the enemy object of the stage group. */
            if (d->content == VK_OBJ_NOTHING) { c->ghost_x = d->x; c->ghost_y = d->y; c->ghost_pending = 1; }
            d->state = VK_DROP_NONE;
            return;
        }
        if (d->kind == VK_OBJ_SMALL_HEART)
        {
            if (d->timer < FLAME_HEART_FRAMES) return;
            heart_begin(c, d, d->x + 8);
            return;
        }
        if (d->timer < FLAME_HOLD_FRAMES || ((d->timer - FLAME_HOLD_FRAMES) & 1)) return;
        if (flame_landed(c, map, d))
        {
            /* 9C54 -> 9C0B: ((X - 4) & F8, (Y - 16) & F8) for the flame at (x + 8, y + 16): the Y of a flame left by an enemy is not a multiple of 8
             * (traced: 8 flames 26 and 31 hearts of the death flame), unlike a candle's. */
            d->x = (VKByte)((d->x + 4) & 0xF8);
            d->y = (VKByte)(d->y & 0xF8);
            d->state = VK_DROP_REST;
            d->timer = rest_timer(d->kind);
            d->catchable = 0;
        }
        else
        {
            d->y = (VKByte)(d->y + 8);
            if (d->y + 16 >= 0xE4) d->state = VK_DROP_NONE;   /* 99E5 */
        }
        return;
    }
    if (d->state == VK_DROP_FALL && !d->catchable && d->kind != VK_OBJ_SMALL_HEART)
    {
        /* The content of a chest (type 02, traced): +8 px at t + 4, then 03 and 04 (8B70)
         * until it can be taken at t + 8. */
        d->timer++;
        if (d->timer == CHEST_DROP_FRAMES) d->y = (VKByte)(d->y + 8);
        if (d->timer < CHEST_REST_FRAMES) return;
        if (!drop_landed(map, d)) { d->timer = CHEST_DROP_FRAMES; return; }
        d->x = (VKByte)((d->x + 4) & 0xF8);
        d->y = (VKByte)((d->y + 4) & 0xF8);
        d->state = VK_DROP_REST;
        d->timer = rest_timer(d->kind);
        return;
    }
    if (d->state == VK_DROP_FALL && d->kind == VK_OBJ_SMALL_HEART && d->catchable)
    {
        if (((c->clock ^ (d->catchable >> 1)) & 1) == 0) heart_pass(c, d, map);
        return;
    }
    if (d->state == VK_DROP_FALL)
    {
        if (drop_landed(map, d))
        {
            land_drop(d);
            return;
        }
        if (d->kind != VK_OBJ_SMALL_HEART) { d->y = (VKByte)(d->y + 8); return; }
        /* 8B22: down one pixel every other frame, X swings with a speed that
         * changes by ax each frame; ax changes sign every 32 frames. */
        d->timer++;
        if (c->clock & 1) d->y++;
        d->xfrac = (unsigned short)(d->xfrac + d->vx);
        d->x = (VKByte)(d->xfrac >> 8);
        d->vx = (short)(d->vx - d->ax);
        if (d->timer == HEART_SWAY) { d->timer = 0; d->ax = (short)-d->ax; }
        return;
    }
    if (d->state == VK_DROP_REST && d->timer != 0xFF && (c->clock & 31) == 0)
    {
        if (--d->timer == 0) d->state = VK_DROP_NONE;
    }
}

static int touches_drop(const VKPlayer *p, const VKDrop *d)
{
    const int dx = p->x - (d->x + 8);
    const int lift = p->mode != 0 ? 8 : 2;
    const int height = (p->mode != 0 || p->crouched) ? PICK_HEIGHT_LOW : PICK_HEIGHT;
    const int dy = p->y - lift - d->y;
    return dx > -PICK_HALF_W && dx < PICK_HALF_W && dy >= 0 && dy < height;
}

/* 8C0B: the stage stream flag of a fixed object, or of the hidden block that
 * held the chest, is cleared: it does not come back. */
static void mark_taken(VKCandles *c, const VKDrop *d)
{
    if (!d->fixed || c->stage >= VK_CANDLE_STAGES || c->room >= VK_CANDLE_ROOMS) return;
    if (d->fixed & 0x80) c->out[c->stage][c->room] |= (VKByte)(1u << ((d->fixed & 0x7F) - 1));
    else c->taken[c->stage][c->room] |= (VKByte)(1u << (d->fixed - 1));
}

void vk_candles_white_key_taken(VKCandles *c)
{
    int i;
    if (c->stage >= VK_CANDLE_STAGES || c->room >= VK_CANDLE_ROOMS) return;
    for (i = 0; i < c->count; i++)
    {
        const VKCandleDef *def = vk_candles_def(c, i);
        if (def->mode == VK_CANDLE_HIDDEN_BLOCK && def->kind == VK_OBJ_WHITE_KEY)
            c->out[c->stage][c->room] |= (VKByte)(1u << i);
    }
}

/* 8D30 and its table 8D3E. Returns the events of the effect. */
static int pick(VKCandles *c, VKDrop *d)
{
    const VKByte kind = d->kind;
    if (kind == VK_OBJ_CHEST)
    {
        /* 8F68: needs a gold key, which it uses up (C700 - 1), then 8C14/8C2F:
         * the chest becomes its content, 8 px higher, waits 32 frames (type 05)
         * and falls. Its +9..+12 are 0: a small heart would not sway. */
        if (!c->gold_keys) return VK_CANDLE_EVENT_NONE;
        c->gold_keys--;
        mark_taken(c, d);
        d->kind = d->content;
        d->y = (VKByte)(d->y - 8);
        d->fixed = 0;
        d->state = VK_DROP_WAIT;
        d->timer = 0;
        d->xfrac = (unsigned short)(d->x << 8);
        d->vx = d->ax = 0;
        /* 8C1D: the last key used, C701 &= F9 and 8EC9 clears the HUD key. */
        if (!c->gold_keys) { c->triple_key = 0; vk_candles_hud(c); }   /* 8C1D: C701 &= F9 */
        else if (c->triple_key) vk_candles_hud(c);
        return VK_CANDLE_EVENT_GOLD_KEY;
    }
    /* 8F83: a gold key is not taken while C701 bit 1 or 2 is set (a key held). */
    if (kind == VK_OBJ_GOLD_KEY && c->gold_keys) return VK_CANDLE_EVENT_NONE;
    mark_taken(c, d);
    d->state = VK_DROP_NONE;
    return vk_candles_give(c, kind);
}

void vk_candles_block_gone(VKCandles *c, int i)
{
    if (c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS && i >= 0 && i < c->count)
        c->out[c->stage][c->room] |= (VKByte)(1u << i);
}

int vk_candles_give(VKCandles *c, VKByte kind)
{
    c->picked = kind;                /* 8D30: LD (C419),A */
    box_show_pick(c, kind);          /* 8D33: CALL 8F23 */
    if (kind == VK_OBJ_SMALL_HEART || kind == VK_OBJ_BIG_HEART)
    {
        const int hearts = c->hearts + (kind == VK_OBJ_SMALL_HEART ? 1 : 5);
        c->hearts = (VKByte)(hearts > HEARTS_MAX ? HEARTS_MAX : hearts);
        return VK_CANDLE_EVENT_HEARTS;
    }
    if (kind == VK_OBJ_GOLD_KEY)
    {
        c->gold_keys = 1;            /* 8E50: C701 bit 1, C700 = 1, 8EC9 */
        c->triple_key = 0;
        vk_candles_hud(c);
        return VK_CANDLE_EVENT_GOLD_KEY;
    }
    if (kind >= VK_OBJ_CHAIN_WHIP && kind <= VK_OBJ_CHAIN_WHIP + 3)
    {
        c->weapon = (VKByte)(kind - 0x19);
        c->hud_weapon = kind;        /* 8D79 -> 8E9A: the weapon box of the HUD */
        /* 8D70 falls into 8D7C: C440 too (enemies.c). */
        return VK_CANDLE_EVENT_WEAPON | VK_CANDLE_EVENT_PICK;
    }
    /* 8E79 (12, traced in a chest of stage 8: C431 00 -> 02, C700 0 -> 3, C701 -> 04, sound 0F):
     * three gold keys, shown as the object 12 in the HUD (8EDC). */
    if (kind == 0x12)
    {
        c->c431 |= 0x02;
        c->gold_keys = 3;
        c->triple_key = 1;
        vk_candles_hud(c);
        return VK_CANDLE_EVENT_ITEM;
    }
    /* 8DF5-8E03: C431 bits 2, 3, 4, then 8D8A (effect 12). Bit 3: recoil of 2.5 px,
     * bit 4: jump table 6D94 of 21 steps (gameplay.c, player.c); bit 2 doubles the
     * item timers (enemies.c). */
    if (kind >= 0x0B && kind <= 0x0D)
    {
        c->c431 |= (VKByte)(1u << (kind - 0x0B + 2));
        return VK_CANDLE_EVENT_ITEM;
    }
    /* 8D98 (03), 8DBB (05), 8D7C (06), 8DDE (07), 8DCD (08), 8DE5 (09), 8E0D (16):
     * energy, armor, timers and the enemy wipe live in enemies.c (Enemies_item). */
    if (kind == 0x03 || (kind >= 0x05 && kind <= 0x09) || kind == 0x16)
    {
        /* 8D98: C701 bit 5 cleared, bit 4 set (8D8F); its icon waits for 8EE6. */
        if (kind == 0x03) c->c701 = (VKByte)((c->c701 & ~VK_ITEM_BIT_04) | VK_ITEM_BIT_03);
        return VK_CANDLE_EVENT_PICK;
    }
    /* 8DA1: C701 bit 4 cleared (no more armor, enemies.c), bit 5 set. The D700
     * projectiles it guards against (8610) are not ported. */
    if (kind == 0x04)
    {
        c->c701 = (VKByte)((c->c701 & ~VK_ITEM_BIT_03) | VK_ITEM_BIT_04);
        return VK_CANDLE_EVENT_PICK;
    }
    /* 8D94 (0A): C701 bit 6, the only record of the object: the time stop (713B)
     * reads it through Enemies_useTimeStop (actors.c vk_step_time_stop). */
    if (kind == 0x0A)
    {
        c->c701 |= VK_ITEM_BIT_0A;
        return VK_CANDLE_EVENT_BOX_ITEM;
    }
    /* 8D8D (1E), 8E33 (0F): the C701 bit, and C70F = 3 and C431 bit 6 for 0F (traced:
     * C431 00 -> 40, C701 -> 80; nothing else reads bit 6 in the port). */
    if (kind == 0x1E || kind == 0x0F)
    {
        c->c701 |= kind == 0x1E ? VK_ITEM_BIT_1E : VK_ITEM_BIT_0F;
        if (kind == 0x0F) { c->c70f = 3; c->c431 |= 0x40; }
        return VK_CANDLE_EVENT_BOX_ITEM;
    }
    /* 8E1D (10): C702 bit 6, bit 7 cleared; 8E26 (11): bit 7, bit 6 cleared. Only
     * the merchant prices read them (9418). */
    if (kind == 0x10 || kind == 0x11)
    {
        c->c702 = (VKByte)((c->c702 & 0x3F) | (kind == 0x10 ? 0x40 : 0x80));
        return VK_CANDLE_EVENT_ITEM;
    }
    /* 8E42 (13: 5000 points), 8E4B (14: 1000 points) through 44EC; the score is added
     * by main.c, the effect 10 by vk_candles_sound. */
    if (kind == 0x13 || kind == 0x14) return VK_CANDLE_EVENT_ITEM;
    /* PROVISIONAL: the other effects (C702 bit 0 of 0E, object 12) are not ported;
     * none of them is found or sold in stages 0-2. */
    return VK_CANDLE_EVENT_NONE;
}

VKByte vk_candles_sound(VKByte kind)
{
    switch (kind)
    {
    case 0x00: case 0x15: case 0x19: return 0;                 /* 8E14: POP HL, no 509F */
    case 0x01: case 0x02: case 0x12: return 0x0F;              /* 8DB4, 8E90 */
    case 0x05: return 0x1B;                                    /* 8DCA */
    case 0x08: return 0x16;                                    /* 8DDB */
    case 0x13: case 0x14: return 0x10;                         /* 8E48 */
    case 0x17: case 0x18: return 0x14;                         /* 8E5D, 8E68 */
    default: return kind < 0x1F ? 0x12 : 0;                    /* 8D8A, 8E30 */
    }
}

int vk_candles_tick(VKCandles *c, VKGame *game, const VKWhip *whip, int *hit_index)
{
    const VKByte *map = room_map(game);
    int events = VK_CANDLE_EVENT_NONE;
    int i;
    vk_candles_enter(c, game);
    c->clock++;
    *hit_index = -1;
    for (i = 0; i < c->count; i++)
        if (vk_candles_lit(c, i) && vk_candle_whip_touch(vk_candles_def(c, i), whip, c->weapon))
        {
            const VKCandleDef *def = vk_candles_def(c, i);
            events |= vk_candles_hit(c, i);
            *hit_index = i;
            /* 8899: the tiles saved under the block (E4A0) go back into the map. */
            if (def->mode == VK_CANDLE_HIDDEN_BLOCK) vk_game_room_block(game, def->x, def->y, 0);
            break;                   /* 80D1: one hit a pass, the view draws this one */
        }
    box_step(c);                     /* 8A4A: CALL 8F55 before the C500 loop */
    for (i = 0; i < VK_DROP_MAX; i++)
    {
        VKDrop *d = &c->drop[i];
        if (d->state == VK_DROP_NONE) continue;
        step_drop(c, d, map);
        if (c->ghost_pending) { c->ghost_pending = 0; events |= VK_CANDLE_EVENT_GHOST; }
        /* 7FE2: pickups only out of an attack (C422 = 0), of the objects whose
         * type has bit 7 set: on the floor (84), or falling from a candle (82). */
        if ((d->state == VK_DROP_REST || (d->state == VK_DROP_FALL && d->catchable))
            && whip->timer == 0 && touches_drop(&game->player, d))
            events |= pick(c, d);
    }
    return events;
}

VKByte vk_candles_drop_sprite(const VKDrop *d, VKByte clock)
{
    (void)clock;
    if (d->state == VK_DROP_REST) return d->kind;
    if (d->state == VK_DROP_WAIT) return 0x07;
    if (d->state == VK_DROP_FALL)
    {
        if (d->kind == VK_OBJ_SMALL_HEART) return VK_OBJ_SMALL_HEART;
        /* 8B70 draws the object in the decor from type 03 on (t + 4). */
        return !d->catchable && d->timer >= CHEST_DROP_FRAMES ? d->kind : 0x07;
    }
    return 0;
}
