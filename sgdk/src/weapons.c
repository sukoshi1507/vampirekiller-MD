#include "weapons.h"

/* Thrown weapons (bank 1, 7116-75BF ; research/secondary-weapons.md). */

#define SIMON_JUMP 1                 /* C420 values (enemies.h ENEMY_SIMON_*) */
#define SIMON_CROUCH 2
#define SHOT_STAND_DY 0xF0           /* 7307, 7436, 73AE: Y = C425 - 16, */
#define SHOT_CROUCH_DY 0xF6          /* or C425 - 10 crouched (C420 = 2) */
#define BOOMERANG_OUT 0x18           /* 734C: passes of state 2 before turning */
#define BOOMERANG_TURN 0x17          /* 7388: passes of state 3 */
#define BOOMERANG_BACK 5             /* 7363: speed back after an edge (7359) */
#define WATER_FLAME_PASSES 0x18      /* 7430 */
#define WATER_STEPS 0x16             /* 73F6: last step of 707D held */

/* 707D: the recoil arc, read by the holy water two by two (73EB: ADD A,A). */
static const signed char arc[WATER_STEPS] = {
    -3, -3, -2, -2, -2, -1, -1, -1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3
};

static void sound(VKWeaponEvents *ev, VKByte id)
{
    if (ev->sounds < sizeof(ev->sound)) ev->sound[ev->sounds++] = id;
}

void vk_weapons_free(VKWeapons *w, int slot)
{
    VKShot *s = &w->shot[slot];
    *s = (VKShot){0};
    s->y = VK_SHOT_FREE_Y;           /* 750D-7511: +4 = E0 */
}

void vk_weapons_reset(VKWeapons *w)
{
    int i;
    for (i = 0; i < VK_WEAPON_SLOTS; i++) vk_weapons_free(w, i);
    w->launch = 0;
    w->block = 0;
}

int vk_weapons_reserve(VKWeapons *w, VKByte weapon)
{
    const int count = weapon == VK_SHOT_DAGGER ? 2 : 1;
    int i;
    for (i = 0; i < count; i++)
        if (!w->shot[i].type)
        {
            w->shot[i].state = 1;    /* 71D4 */
            return 1;
        }
    return 0;
}

void vk_weapons_launch(VKWeapons *w, VKByte weapon)
{
    w->launch = weapon;
}

int vk_weapons_water(VKWeapons *w, VKByte *hearts, int left)
{
    VKShot *s = &w->shot[1];
    if (*hearts < VK_WEAPON_WATER_HEARTS || s->type) return 0;   /* 714D, 7194/71A1 */
    s->facing = (VKByte)(left ? 1 : 0);                           /* C468 = C460 + 8 */
    s->type = VK_SHOT_WATER;                                      /* 71AF: C461 = 5 */
    *hearts = (VKByte)(*hearts - VK_WEAPON_WATER_HEARTS);         /* 71B2-71BB */
    return 1;
}

/* 8240 -> 847A with HL = 1008: Simon catches the boomerang. */
static int caught(const VKShot *s, const VKWeaponSimon *simon)
{
    const int d = (int)simon->x - s->x;
    const int low = simon->state == SIMON_JUMP || simon->state == SIMON_CROUCH;
    const int y = simon->state == SIMON_JUMP ? simon->y - 8 : simon->y;
    return d > -13 && d < 13 && (VKByte)(y - 2 - s->y + 0x10) < 0x10 + (low ? 0x12 : 0x1A);
}

/* 7307, 7436, 73AE: the shot starts from Simon. */
static void start(VKShot *s, const VKWeaponSimon *simon, int speed)
{
    s->y = (VKByte)(simon->y + (simon->state == SIMON_CROUCH ? SHOT_CROUCH_DY : SHOT_STAND_DY));
    s->dy = 0;
    s->dx = (signed char)(s->facing ? -speed : speed);
    s->x = simon->x;
}

/* 734F: next state, count 0 (and 999F: the enemies can be hit again). */
static void next_state(VKShot *s)
{
    s->state++;
    s->count = 0;
}

/* 7359/735C: back at once from a screen edge. */
static void edge_back(VKShot *s)
{
    s->state++;
    s->dx = (signed char)(s->facing ? BOOMERANG_BACK : -BOOMERANG_BACK);
    s->count = 0;
}

/* 7307-739D: the axe (speed 3) and the cross (speed 5) go out 24 passes, slow down
 * by 1 every other pass for 23 passes and come back; Simon catches them in states
 * 3 and 4. Returns 1 when caught (74E9). */
static int boomerang(VKShot *s, const VKWeaponSimon *simon)
{
    switch (s->state)
    {
    case 1:
        start(s, simon, s->type == VK_SHOT_AXE ? 3 : 5);
        next_state(s);
        return 0;
    case 2:
        if ((VKByte)(s->x - 10) >= 0xEC) { s->state++; edge_back(s); return 0; }
        if (++s->count >= BOOMERANG_OUT) next_state(s);
        return 0;
    case 3:
        if (caught(s, simon)) return 1;
        if ((VKByte)(s->x - 10) >= 0xEC) { edge_back(s); return 0; }
        if (++s->count >= BOOMERANG_TURN) { next_state(s); return 0; }
        if (s->count & 1) s->dx = (signed char)(s->facing ? s->dx + 1 : s->dx - 1);
        return 0;
    default:
        return caught(s, simon);
    }
}

/* 73A4: the holy water. States 0 and 1 start from Simon (twice: 73A4 takes any
 * state but 2 and 3 for 73AE), 2 falls on the arc of 707D until 7B98 finds a floor
 * tile at X or X - 10, 3 burns 24 passes. Returns 1 when the flames end. */
static int water(VKShot *s, const VKWeaponSimon *simon, VKByte tick, const VKByte *map,
                 VKByte stage, VKWeaponEvents *ev)
{
    if (s->state == 2)
    {
        s->y = (VKByte)(s->y + 2 * arc[s->count]);
        if (++s->count >= WATER_STEPS) s->count--;
        if (map && (vk_tile_solid(vk_sample_tile(map, s->x, s->y), stage, 0) ||
                    vk_tile_solid(vk_sample_tile(map, (VKByte)(s->x - 10), s->y), stage, 0)))
        {
            s->state = 3;            /* 7407 */
            s->dy = s->dx = 0;
            s->count = 0;
            sound(ev, 0x18);
        }
        return 0;
    }
    if (s->state == 3)
    {
        s->pattern = (tick & 4) ? 0xF8 : 0xF4;    /* 7419 (F8: 999F too) */
        return ++s->count >= WATER_FLAME_PASSES;
    }
    start(s, simon, 2);
    s->pattern = 0x38;
    next_state(s);
    return 0;
}

int vk_weapons_frame(VKWeapons *w, const VKWeaponSimon *simon, VKByte weapon,
                     const VKByte *map, VKByte stage, VKWeaponEvents *ev)
{
    int i;
    *ev = (VKWeaponEvents){0};
    w->phase ^= 1;
    if (w->phase) return 0;
    w->tick++;
    /* 7286: the reserved slot (+0 = 1) is launched, then 72B2 runs it the same pass. */
    if (w->launch)
    {
        for (i = 0; i < VK_WEAPON_SLOTS; i++)
            if (w->shot[i].state == 1 && !w->shot[i].type)
            {
                w->shot[i].pattern = 0x18;
                w->shot[i].facing = simon->facing;
                w->shot[i].type = w->launch;
                break;
            }
        w->launch = 0;
    }
    for (i = 0; i < VK_WEAPON_SLOTS; i++)
    {
        VKShot *s = &w->shot[i];
        int done = 0;
        if (!s->type) continue;
        switch (s->type)
        {
        case VK_SHOT_DAGGER:         /* 7436 */
            if (s->state == 1)
            {
                start(s, simon, 5);
                s->pattern = s->facing ? 0x18 : 0x20;
                s->state = 2;
                sound(ev, 0x04);
            }
            break;
        case VK_SHOT_AXE:            /* 7473 */
            s->pattern = (VKByte)(0x18 + 8 * ((w->tick >> 1) & 3));
            if ((w->tick & 7) == 0) sound(ev, 0x06);
            if ((VKByte)(w->block - 2) < 2)
            {
                /* 74A5: a block touched: the axe falls as object 1C (8992 at X,
                 * Y - 16) and C416 = 0 (8E93). */
                w->block = 0;
                ev->drop = 1;
                ev->drop_x = s->x;
                ev->drop_y = (VKByte)(s->y - 0x10);
                ev->lost = 1;
                vk_weapons_free(w, i);
                continue;
            }
            done = boomerang(s, simon);
            break;
        case VK_SHOT_CROSS:          /* 72DE */
            s->pattern = (VKByte)(0x18 + 8 * ((w->tick >> 2) & 3));
            if ((w->tick & 7) == 0) sound(ev, 0x03);
            done = boomerang(s, simon);
            break;
        default:                     /* 73A4 */
            done = water(s, simon, w->tick, map, stage, ev);
            break;
        }
        if (done) { vk_weapons_free(w, i); continue; }
        s->y = (VKByte)(s->y + s->dy);    /* 74BB */
        s->x = (VKByte)(s->x + s->dx);
        /* 74CE: out at the bottom, or at a side edge, where C416 3 or 4 is lost
         * whatever the shot (the holy water too). */
        if ((VKByte)(s->y - 0xD8) < 0x10) vk_weapons_free(w, i);
        else if ((VKByte)(s->x - 0xFB) < 10)
        {
            if ((VKByte)(weapon - 3) < 2) ev->lost = 1;
            vk_weapons_free(w, i);
        }
    }
    return 1;
}

int vk_shot_touches(const VKShot *s, int d, int e, int h, int l)
{
    const int dx = (int)s->x - d;
    l += 6;
    return dx > -l && dx < l && (VKByte)(s->y - e + h) < h + (s->type == VK_SHOT_DAGGER ? 6 : 12);
}

int vk_shot_touches_candle(const VKWeapons *w, VKByte x, VKByte y, VKByte mode)
{
    const int block = mode >= 2;
    const int h = block ? 0x20 : 0x10, l = block ? 0x10 : 8;
    int i;
    for (i = 0; i < VK_WEAPON_SLOTS; i++)
        if (vk_shot_touches(&w->shot[i], (VKByte)(x + l), (VKByte)(y + h), h, l)) return i;
    return -1;
}

void vk_weapons_candle_hit(VKWeapons *w, int slot, VKByte mode)
{
    w->block = mode;
    if (w->shot[slot].type == VK_SHOT_DAGGER) vk_weapons_free(w, slot);
}

void vk_weapons_enemy_hit(VKWeapons *w, int slot)
{
    if (w->shot[slot].type == VK_SHOT_DAGGER) vk_weapons_free(w, slot);
}

void vk_weapons_pick(VKWeapons *w)
{
    w->block = 0;
}

int vk_weapons_frame_index(const VKShot *s)
{
    /* Frames of tools/build_weapons_md.py: dagger 0-1, axe 2-5, cross 6-9, bottle 10,
     * flames 11-12. 7535 shows a slot whose +0 is set; a reserved slot has +1 = 0. */
    if (!s->type || !s->state || s->y >= 0xD8) return -1;
    switch (s->type)
    {
    case VK_SHOT_DAGGER: return s->pattern == 0x20 ? 1 : 0;
    case VK_SHOT_AXE: return 2 + ((s->pattern - 0x18) >> 3);
    case VK_SHOT_CROSS: return 6 + ((s->pattern - 0x18) >> 3);
    default: return s->pattern == 0xF8 ? 12 : (s->pattern == 0xF4 ? 11 : 10);
    }
}
