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
#define REST_TICKS 8
#define FLOOR_LIMIT 0xDB
#define HEART_VX0 0x0200
#define HEART_AX0 0x0020
#define HEART_SWAY 0x20
#define HEARTS_MAX 99

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

void vk_candles_relight(VKCandles *c)
{
    c->lit = (VKByte)((1u << c->count) - 1);
    if (c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS) c->out[c->stage][c->room] = 0;
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
        d->kind = def->mode;
        d->content = def->kind;
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
    if (c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS)
        c->out[c->stage][c->room] |= (VKByte)(1u << i);
    /* Hidden block with the white key (883E): the key of items.c appears. */
    if (def->mode == 3 && def->kind == VK_OBJ_WHITE_KEY)
        return VK_CANDLE_EVENT_HIT | VK_CANDLE_EVENT_WHITE_KEY;
    kind = drop_kind(c, def->kind);
    /* 0, 0x15 and the block contents (0x1F, +7..+9) give nothing here. */
    if (kind == 0 || kind == VK_OBJ_NOTHING || kind > VK_OBJECT_SHEET_COUNT - 2)
        return VK_CANDLE_EVENT_HIT;
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
    if (d->state == VK_DROP_BURST)
    {
        if (++d->timer >= BURST_FRAMES) { d->state = VK_DROP_FALL; d->timer = 0; }
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
            d->timer = (VKByte)((VKByte)(d->kind - VK_OBJ_GOLD_KEY) < 3 ? 0xFF : REST_TICKS);
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

static void mark_taken(VKCandles *c, const VKDrop *d)
{
    if (d->fixed && c->stage < VK_CANDLE_STAGES && c->room < VK_CANDLE_ROOMS)
        c->taken[c->stage][c->room] |= (VKByte)(1u << (d->fixed - 1));
}

/* 8D30 and its table 8D3E. Returns the events of the effect. */
static int pick(VKCandles *c, VKDrop *d)
{
    const VKByte kind = d->kind;
    if (kind == VK_OBJ_CHEST)
    {
        /* 8F68: needs a gold key, which it uses up (C700 - 1), then 8C14: the
         * chest becomes its content, 8 px higher, and falls again. */
        if (!c->gold_keys) return VK_CANDLE_EVENT_NONE;
        c->gold_keys--;
        mark_taken(c, d);
        d->kind = d->content;
        d->y = (VKByte)(d->y - 8);
        d->fixed = 0;
        d->state = VK_DROP_FALL;
        d->timer = 0;
        d->xfrac = (unsigned short)(d->x << 8);
        d->vx = HEART_VX0;
        d->ax = HEART_AX0;
        return VK_CANDLE_EVENT_GOLD_KEY;
    }
    mark_taken(c, d);
    d->state = VK_DROP_NONE;
    if (kind == VK_OBJ_SMALL_HEART || kind == VK_OBJ_BIG_HEART)
    {
        const int hearts = c->hearts + (kind == VK_OBJ_SMALL_HEART ? 1 : 5);
        c->hearts = (VKByte)(hearts > HEARTS_MAX ? HEARTS_MAX : hearts);
        return VK_CANDLE_EVENT_HEARTS;
    }
    if (kind == VK_OBJ_GOLD_KEY) { c->gold_keys = 1; return VK_CANDLE_EVENT_GOLD_KEY; }
    if (kind >= VK_OBJ_CHAIN_WHIP && kind <= VK_OBJ_CHAIN_WHIP + 3)
    {
        c->weapon = (VKByte)(kind - 0x19);
        return VK_CANDLE_EVENT_WEAPON;
    }
    /* PROVISIONAL: the other effects (timers, points, C701/C702 bits) are not ported. */
    return VK_CANDLE_EVENT_NONE;
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
            events |= vk_candles_hit(c, i);
            *hit_index = i;
        }
    for (i = 0; i < VK_DROP_MAX; i++)
    {
        VKDrop *d = &c->drop[i];
        if (d->state == VK_DROP_NONE) continue;
        step_drop(c, d, map);
        /* 7FE2: pickups only out of an attack (C422 = 0). */
        if (d->state == VK_DROP_REST && whip->timer == 0 && touches_drop(&game->player, d))
            events |= pick(c, d);
    }
    return events;
}

VKByte vk_candles_drop_sprite(const VKDrop *d, VKByte clock)
{
    (void)clock;
    if (d->state == VK_DROP_REST) return d->kind;
    if (d->state == VK_DROP_FALL) return d->kind == VK_OBJ_SMALL_HEART ? VK_OBJ_SMALL_HEART : 0x07;
    return 0;
}
