#include "wall.h"
#include "enemies.h"   /* Enemies_randomR: the common LD A,R generator (research/hasard-R.md) */

/* 9300: C70C for row C (+5) and hits - 1 (capped at 7). 7, 8 and 9 are random
 * picks (92D6): 7 -> 3 or 5, 8 -> 4 or 5, 9 -> 3 or 4. */
static const VKByte result_table[4][8] = {
    { 0, 7, 7, 7, 8, 8, 5, 6 },
    { 0, 3, 3, 1, 5, 5, 5, 6 },
    { 9, 9, 0, 5, 5, 5, 2, 6 },
    { 5, 5, 5, 5, 0, 3, 5, 6 }
};
/* 9320: frame (C70B) shown at the end of the animation for each C70C. */
static const VKByte result_frame[7] = { 1, 4, 4, 0, 0, 3, 3 };

const VKByte vk_wall_objects[9] = { 0x0E, 0x12, 0x03, 0x04, 0x0A, 0x16, 0x1E, 0x1D, 0x1B };

const VKByte vk_shop_prices[9][4] = {
    { 0x0E, 0x20, 0x15, 0x60 }, { 0x12, 0x30, 0x20, 0x60 }, { 0x03, 0x20, 0x10, 0x60 },
    { 0x04, 0x20, 0x10, 0x80 }, { 0x0A, 0x40, 0x20, 0x80 }, { 0x16, 0x40, 0x15, 0x80 },
    { 0x1E, 0x30, 0x10, 0x50 }, { 0x1D, 0x20, 0x10, 0x80 }, { 0x1B, 0x50, 0x30, 0x90 }
};

#define SOUND_HEARTS_UP 0x0F         /* 9344 */
#define SOUND_HEARTS_DOWN 0x1D       /* 934E */
#define SOUND_GONE 0x10              /* 936D */
#define SOUND_SHOP 0x19              /* 938F */
#define SOUND_BUY 0x12               /* 94EF */
#define SOUND_REFUSE 0x02            /* 94FC */
#define HEARTS_MAX 99

static int bcd(VKByte v) { return (v >> 4) * 10 + (v & 15); }

static VKByte item_of(const VKWall *w, int slot)
{
    if (w->stage >= VK_CANDLE_STAGES || w->room >= VK_CANDLE_ROOMS) return 0;
    return vk_wall_items[w->stage][w->room][slot];
}

void vk_wall_begin(VKWall *w)
{
    int s;
    for (s = 0; s < VK_WALL_RECORDS; s++) w->obj[s].state = 0;
    for (s = 0; s < VK_CANDLE_STAGES; s++) vk_wall_reset_stage(w, (VKByte)s);
    w->stage = w->room = 0xFF;
    w->c70c = w->c70b = 0;
    w->d012 = 0;
    w->phase = 0;
    w->shop = VK_SHOP_NONE;
    w->slot = w->sale = w->price = w->timeout = w->delay = 0;
    w->keys = 0;
    w->c003 = 0;
    w->bought = 0;
}

void vk_wall_reset_stage(VKWall *w, VKByte stage)
{
    int r, s;
    if (stage >= VK_CANDLE_STAGES) return;
    /* 5AAF clears DE00-DEBF, then writes the items: the counts start at 0. */
    for (r = 0; r < VK_CANDLE_ROOMS; r++)
    {
        for (s = 0; s < VK_WALL_RECORDS; s++) w->count[stage][r][s] = 0;
        w->gone[stage][r] = 0;
    }
}

/* 944F + 1: the count byte of DE00 slot i. */
static VKByte *count_of(VKWall *w, int i)
{
    static VKByte none;
    if (w->stage >= VK_CANDLE_STAGES || w->room >= VK_CANDLE_ROOMS) { none = 0; return &none; }
    return &w->count[w->stage][w->room][i];
}

/* 91EA -> 924C: shown (82h), the map tiles under it saved (8757, nothing to do
 * here), DE00 count - 1 while its bit 7 is set (a merchant left without an
 * answer comes back at the next hit), frame 3 drawn (925C). */
static void show(VKWall *w, int i)
{
    VKByte *count = count_of(w, i);
    w->obj[i].state = VK_WALL_SHOWN;
    if (*count & 0x80) (*count)--;
    w->obj[i].frame = VK_WALL_FIRST_FRAME;
}

/* 9179 with B = (entry & 3C) >> 2 and C = entry & 3: 91A2 looks for the slot of
 * DE00 whose item is 5B0B[B] (slot 0 first); none, nothing is made. */
static int make(VKWall *w, VKByte x, VKByte y, VKByte entry, VKByte flag)
{
    const VKByte b = (VKByte)((entry & 0x3C) >> 2);
    const VKByte item = b < 9 ? vk_wall_objects[b] : 0;
    int i;
    for (i = 0; i < VK_WALL_RECORDS; i++)
    {
        if (item_of(w, i) != item) continue;
        w->obj[i].state = VK_WALL_NEW;
        w->obj[i].y = y;
        w->obj[i].x = x;
        w->obj[i].hits = 0;
        w->obj[i].row = (VKByte)(entry & 3);
        w->obj[i].timer = 0;
        w->obj[i].flag = flag;
        show(w, i);                  /* 91BE runs later in the same pass (5C74) */
        return i;
    }
    return -1;
}

void vk_wall_enter(VKWall *w, VKByte stage, VKByte room)
{
    int i;
    w->stage = stage;
    w->room = room;
    for (i = 0; i < VK_WALL_RECORDS; i++) w->obj[i].state = 0;
    if (stage >= VK_CANDLE_STAGES || room >= VK_CANDLE_ROOMS) return;
    for (i = vk_wall_first[stage][room]; i < vk_wall_first[stage][room + 1]; i++)
    {
        const int bit = i - vk_wall_first[stage][room];
        const VKWallDef *def = &vk_wall_defs[i];
        if (w->gone[stage][room] & (1u << bit)) continue;   /* 5B23: flag 0, skipped */
        make(w, def->x, def->y, def->code, (VKByte)(0x80 | (bit + 1)));
    }
}

int vk_wall_block(VKWall *w, int candle, const VKCandleDef *def)
{
    /* 8814: DE = the block's Y and X (POP DE), entry = +9. */
    return make(w, def->x, def->y, def->content, (VKByte)(candle + 1));
}

int vk_wall_hittable(const VKWall *w, int i)
{
    return i >= 0 && i < VK_WALL_RECORDS && (w->obj[i].state & 0x80);
}

int vk_wall_whip_touch(const VKWall *w, int i, const VKWhip *whip, int weapon)
{
    VKCandleDef box;
    if (!vk_wall_hittable(w, i)) return 0;
    /* 83BC: E = Y + 20h, D = X + 10h; 84A7 with HL = 1008 is the test of a candle
     * whose E = Y' + 10h and D = X' + 8 (8450). */
    box.x = (VKByte)(w->obj[i].x + 8);
    box.y = (VKByte)(w->obj[i].y + 16);
    box.mode = box.kind = box.content = 0;
    return vk_candle_whip_touch(&box, whip, weapon);
}

void vk_wall_hit(VKWall *w, int i)
{
    if (vk_wall_hittable(w, i)) w->obj[i].hits++;
}

void vk_wall_cross(VKWall *w)
{
    int i;
    for (i = 0; i < VK_WALL_RECORDS; i++)
        if (w->obj[i].state & 0x80) w->obj[i].hits = 0xFF;
}

/* 92BB: C70C from the table 9300 and the count, C70B from 9320. */
static void pick_result(VKWall *w, int i, VKByte count)
{
    VKByte n = (VKByte)(count - 1);
    VKByte r;
    if (n >= 8) n = 7;
    r = result_table[w->obj[i].row & 3][n];
    if (r >= 7)
    {
        /* 92E8: LD A,R, RRA: bit 0 set gives H (first of the pair), else L. Traced: 137 calls, bit 0 set 75 times (55 %), bit 7 never set. */
        static const VKByte pairs[3][2] = { { 3, 5 }, { 4, 5 }, { 3, 4 } };
        r = pairs[r - 7][(Enemies_randomR() & 1) ? 0 : 1];
    }
    w->c70c = r;
    w->c70b = result_frame[r];
}

static void sound(VKWallEvents *ev, VKByte s)
{
    if (ev->sounds < 2) ev->sound[ev->sounds++] = s;
}

/* 92A7: the result once the animation is over. */
static void dispatch(VKWall *w, VKCandles *c, int i, VKWallEvents *ev)
{
    switch (w->c70c)
    {
    case VK_WALL_MERCHANT:
        /* 9327: C40C = FF (state 0C after this pass), C703 = +9 (the record). */
        w->shop = VK_SHOP_OPEN;
        w->slot = (VKByte)i;
        ev->shop = 1;
        break;
    case VK_WALL_D012_UP: if (w->d012 != 3) w->d012++; break;
    case VK_WALL_D012_DOWN: if (w->d012) w->d012--; break;
    case VK_WALL_HEARTS_UP:
        sound(ev, SOUND_HEARTS_UP);
        c->hearts = (VKByte)(c->hearts + 5 > HEARTS_MAX ? HEARTS_MAX : c->hearts + 5);   /* 4594 */
        ev->hearts = 1;
        break;
    case VK_WALL_HEARTS_DOWN:
        sound(ev, SOUND_HEARTS_DOWN);
        c->hearts = (VKByte)(c->hearts >= 5 ? c->hearts - 5 : 0);   /* 45A0 */
        ev->hearts = 1;
        break;
    case VK_WALL_GONE:
        /* 9358: +0 = 0, the saved tiles back (9378, 889C), stream flag (+7/+8) = 0,
         * effect 10, 5000 points (44EC). */
        w->obj[i].state = 0;
        ev->redraw |= (VKByte)(1u << i);
        if (w->obj[i].flag & 0x80)
        {
            if (w->stage < VK_CANDLE_STAGES && w->room < VK_CANDLE_ROOMS)
                w->gone[w->stage][w->room] |= (VKByte)(1u << ((w->obj[i].flag & 0x7F) - 1));
        }
        else
        {
            vk_candles_block_gone(c, w->obj[i].flag - 1);
            ev->block_gone = w->obj[i].flag;
        }
        sound(ev, SOUND_GONE);
        ev->points = 1;
        break;
    default: break;                  /* 5: 9343, nothing */
    }
}

void vk_wall_step(VKWall *w, VKCandles *c, VKWallEvents *ev)
{
    int i, pending = 0;
    ev->redraw = 0;
    ev->sounds = 0;
    ev->hearts = ev->points = ev->shop = ev->block_gone = 0;
    for (i = 0; i < VK_WALL_RECORDS; i++)
        if ((w->obj[i].state & 0x80) && w->obj[i].hits) pending = 1;
    /* A contact is only made in a pass (whip C422 = 3, shots, 7FE2): its 91BE runs
     * at once and the next pass comes 2 frames later. */
    if (pending) w->phase = 0;
    else if ((w->phase ^= 1) != 0) return;
    for (i = 0; i < VK_WALL_RECORDS; i++)
    {
        VKWallObject *o = &w->obj[i];
        if (o->state == VK_WALL_NEW) { show(w, i); ev->redraw |= (VKByte)(1u << i); continue; }
        if (o->state == VK_WALL_SHOWN)
        {
            /* 91F2: the contacts are read and cleared. */
            const VKByte hits = o->hits;
            if (!hits) continue;
            o->hits = 0;
            if (hits == 0xFF) { w->c70c = VK_WALL_GONE; w->c70b = 3; }   /* 9215 */
            else
            {
                /* 9221: DE00 count, bit 7 cleared, + 1. */
                VKByte *count = count_of(w, i);
                *count = (VKByte)((*count & 0x7F) + 1);
                pick_result(w, i, *count);
            }
            o->timer = VK_WALL_ANIM_PASSES;      /* 920C */
            o->state = VK_WALL_ANIM;
            continue;
        }
        if (o->state == VK_WALL_ANIM)
        {
            /* 9229: frame 2 when +0A is odd after the DEC, else C70B. */
            const VKByte frame = (VKByte)((--o->timer & 1) ? VK_WALL_BLINK_FRAME : w->c70b);
            if (frame != o->frame) { o->frame = frame; ev->redraw |= (VKByte)(1u << i); }
            if (o->timer) continue;
            o->state = VK_WALL_SHOWN;            /* 9245 */
            dispatch(w, c, i, ev);
        }
    }
}

int vk_wall_frame(const VKWall *w, int i)
{
    if (i < 0 || i >= VK_WALL_RECORDS || !w->obj[i].state) return -1;
    return w->obj[i].frame;
}

void vk_shop_window(const VKWall *w, VKByte *x, VKByte *y)
{
    const VKWallObject *o = &w->obj[w->slot & 1];
    *x = (VKByte)(o->x < 0x80 ? o->x - 0x10 : o->x - 0x20);   /* 93A8 */
    *y = (VKByte)(o->y - 0x18);                               /* 93B4 */
}

/* 93FF: C708 = the item of the slot, its count gets bit 7; 9409/9418: price. */
static void open_shop(VKWall *w, const VKCandles *c)
{
    int k;
    VKByte *count = count_of(w, w->slot & 1);
    w->sale = item_of(w, w->slot & 1);
    *count |= 0x80;
    w->price = 0;
    for (k = 0; k < 9; k++)
    {
        if (vk_shop_prices[k][0] != w->sale) continue;
        w->price = vk_shop_prices[k][(c->c702 & 0x80) ? 2 : (c->c702 & 0x40) ? 3 : 1];
        break;
    }
}

int vk_shop_step(VKWall *w, VKCandles *c, VKByte keys, VKWallEvents *ev)
{
    int events = 0;
    VKByte fresh;
    ev->redraw = 0;
    ev->sounds = 0;
    ev->hearts = ev->points = ev->shop = ev->block_gone = 0;
    w->c003++;                       /* 414D: one pass a frame in state 0C */
    switch (w->shop)
    {
    case VK_SHOP_OPEN:
        /* 440A: C40C = 0, 9387: 93FF, C706 = 14h, effect 19, window (9397). */
        open_shop(w, c);
        w->timeout = VK_SHOP_TIMEOUT;
        sound(ev, SOUND_SHOP);
        w->bought = 0;
        /* PROVISIONAL: C709 at the opening is not traced; the key that struck the
         * last blow does not count as a new press. */
        w->keys = keys;
        w->shop = VK_SHOP_WAIT;
        break;
    case VK_SHOP_WAIT:
        /* 94BA: C706 - 1 when C003 & 1F = 0; at 0 the answer is no. */
        if ((w->c003 & 0x1F) == 0 && --w->timeout == 0) goto refuse;
        /* 951F: keys newly pressed since the last read (C709). */
        fresh = (VKByte)(keys & ~w->keys);
        w->keys = keys;
        if (!fresh) break;
        if (!(fresh & 1)) goto refuse;
        if (c->hearts < bcd(w->price)) goto refuse;
        /* 94D9: hearts - price (45A0), 8D30 gives C708, effect 12, bit 7 of the
         * DE00 count cleared (94F8). 94DC: with the window over the HUD (C704 < 20h)
         * it is restored and drawn again around the HUD redraw: same picture. */
        c->hearts = (VKByte)(c->hearts - bcd(w->price));
        ev->hearts = 1;
        events = vk_candles_give(c, w->sale);
        sound(ev, SOUND_BUY);
        *count_of(w, w->slot & 1) &= 0x7F;
        w->bought = 1;
        w->delay = VK_SHOP_CLOSE_PASSES;
        w->shop = VK_SHOP_CLOSE;
        break;
    refuse:
        sound(ev, SOUND_REFUSE);
        w->delay = VK_SHOP_CLOSE_PASSES;
        w->shop = VK_SHOP_CLOSE;
        break;
    case VK_SHOP_CLOSE:
        /* 43FD: C004 - 1; at 0, 9507 (window restored, 8707) and state 5. */
        if (--w->delay == 0) w->shop = VK_SHOP_NONE;
        break;
    default: break;
    }
    return events;
}
