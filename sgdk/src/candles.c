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
/* Drop timings: 16 frames of flame (8A99), then the fall (8AF8: 8 px a frame, the
 * small heart 1 px every other frame and its sway). On the floor the object stays
 * 8 x 16 frames (8BC0), keys and chest for good. */
#define BURST_FRAMES 16
#define CHEST_WAIT_MASK 0x1F         /* 8BCC: the content of a chest falls after 32 frames */
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
    c->weapon = 0;
    c->hud_key = 0;
    c->c431 = 0;
    c->picked = 0;
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
}

void vk_candles_death(VKCandles *c)
{
    c->gold_keys = 0;                /* 70F7 */
    c->c431 = 0;                     /* 70E8 */
}

void vk_candles_hud(VKCandles *c)
{
    /* 8EC9: object 0x17 at (148, 12) while C701 & 6 and C700 > 0. In stages 0-2
     * no object 0x12 (C700 = 3, C701 bit 2) exists, so C701 bit 1 is set exactly
     * while C700 > 0 (8E50 sets both, 8C23 clears the bit when C700 reaches 0). */
    c->hud_key = c->gold_keys ? VK_OBJ_GOLD_KEY : 0;
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
                                   vk_hidden_block_tiles);
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
    if (vk_whip_phase(whip) != 2) return 0;
    return dx > -reach && dx < reach && dy >= 0 && dy < WHIP_HEIGHT;
}

/* 8B70: on the floor, 8 x 16 frames, or for good for the keys and the chest. */
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
    if (c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS)
        c->out[c->stage][c->room] |= (VKByte)(1u << i);
    /* Hidden block with the white key (883E): the key of items.c appears. */
    if (def->mode == 3 && def->kind == VK_OBJ_WHITE_KEY)
        return VK_CANDLE_EVENT_HIT | VK_CANDLE_EVENT_WHITE_KEY;
    kind = drop_kind(c, def->kind);
    /* 0 and 0x15 give nothing here (0x15 is a hidden enemy on the MSX).
     * PROVISIONAL: a 0x1F block with +9 & C0 = C0 (9179, object C5B5/C5C5) is not
     * ported and gives nothing. */
    if (kind == 0 || kind == VK_OBJ_NOTHING || def->kind == VK_OBJ_BLOCK_CONTENT ||
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

/* 8AF8: lands when the tile under X + 8, Y + 16 is a floor (tile 1..9) or past
 * Y + 16 = DB. */
static int drop_landed(const VKByte *map, const VKDrop *d)
{
    const int probe_y = d->y + 16;
    if (probe_y > FLOOR_LIMIT) return 1;
    return (VKByte)(vk_sample_tile(map, (VKByte)(d->x + 8), (VKByte)probe_y) - 1) < 9;
}

static void step_drop(VKCandles *c, VKDrop *d, const VKByte *map)
{
    if (d->state == VK_DROP_WAIT)
    {
        /* 8BCC: after 32 frames the content falls (type 02); nothing (0) goes (8B1B). */
        if ((++d->timer & CHEST_WAIT_MASK) == 0)
        {
            d->state = d->kind ? VK_DROP_FALL : VK_DROP_NONE;
            d->timer = 0;
            d->catchable = 0;
        }
        return;
    }
    if (d->state == VK_DROP_BURST)
    {
        if (++d->timer >= BURST_FRAMES)
        {
            d->state = VK_DROP_FALL;
            d->timer = 0;
            d->catchable = 1;
        }
        return;
    }
    if (d->state == VK_DROP_FALL)
    {
        if (drop_landed(map, d))
        {
            /* 8B70: snapped to the 8 px grid. */
            d->x = (VKByte)((d->x + 4) & 0xF8);
            d->y = (VKByte)((d->y + 4) & 0xF8);
            d->state = VK_DROP_REST;
            d->timer = rest_timer(d->kind);
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
    if (d->state == VK_DROP_REST && d->timer != 0xFF && (c->clock & 15) == 0)
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
        if (!c->gold_keys) vk_candles_hud(c);
        return VK_CANDLE_EVENT_GOLD_KEY;
    }
    /* 8F83: a gold key is not taken while C701 bit 1 or 2 is set (a key held). */
    if (kind == VK_OBJ_GOLD_KEY && c->gold_keys) return VK_CANDLE_EVENT_NONE;
    mark_taken(c, d);
    d->state = VK_DROP_NONE;
    c->picked = kind;                /* 8D30: LD (C419),A */
    if (kind == VK_OBJ_SMALL_HEART || kind == VK_OBJ_BIG_HEART)
    {
        const int hearts = c->hearts + (kind == VK_OBJ_SMALL_HEART ? 1 : 5);
        c->hearts = (VKByte)(hearts > HEARTS_MAX ? HEARTS_MAX : hearts);
        return VK_CANDLE_EVENT_HEARTS;
    }
    if (kind == VK_OBJ_GOLD_KEY)
    {
        c->gold_keys = 1;            /* 8E50: C701 bit 1, C700 = 1, 8EC9 */
        vk_candles_hud(c);
        return VK_CANDLE_EVENT_GOLD_KEY;
    }
    if (kind >= VK_OBJ_CHAIN_WHIP && kind <= VK_OBJ_CHAIN_WHIP + 3)
    {
        c->weapon = (VKByte)(kind - 0x19);
        /* 8D70 falls into 8D7C: C440 too (enemies.c). */
        return VK_CANDLE_EVENT_WEAPON | VK_CANDLE_EVENT_PICK;
    }
    /* 8DF5-8E03: C431 bits 2, 3, 4, then 8D8A (effect 12). Bit 3: recoil of 2.5 px,
     * bit 4: jump table 6D94 of 21 steps (gameplay.c, player.c); bit 2 doubles the
     * item timers (enemies.c). */
    if (kind >= 0x0B && kind <= 0x0D)
    {
        c->c431 |= (VKByte)(1u << (kind - 0x0B + 2));
        return VK_CANDLE_EVENT_ITEM;
    }
    /* 8D98 (03), 8DBB (05), 8D7C (06), 8DDE (07), 8DCD (08), 8DE5 (09), 8D94 (0A, C701
     * bit 6: time stop), 8E0D (16): energy, armor, timers, the enemy wipe and the time
     * stop live in enemies.c (Enemies_item, Enemies_useTimeStop). */
    if (kind == 0x03 || (kind >= 0x05 && kind <= 0x0A) || kind == 0x16)
        return VK_CANDLE_EVENT_PICK;
    /* PROVISIONAL: the other effects (C701/C702 bits of 04, 0E-12, 1E, points of
     * 13-14) are not ported; none but 0F, 10 and 11 is found in stages 0-2. */
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
        }
    for (i = 0; i < VK_DROP_MAX; i++)
    {
        VKDrop *d = &c->drop[i];
        if (d->state == VK_DROP_NONE) continue;
        step_drop(c, d, map);
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
    if (d->state == VK_DROP_FALL) return d->kind == VK_OBJ_SMALL_HEART ? VK_OBJ_SMALL_HEART : 0x07;
    return 0;
}
