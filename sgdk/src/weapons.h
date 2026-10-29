#ifndef VK_WEAPONS_H
#define VK_WEAPONS_H

#include "player.h"

/* Thrown weapons of the MSX2 (research/secondary-weapons.md): the two 16-byte slots
 * C450 and C460, bank 1 routines 7116-75BF. C416 (VKCandles.weapon) picks what B
 * does: 0 and 1 the whips; 2 the dagger (object 1B), 3 the axe (1C), 4 the cross
 * (1D) are thrown and 7D7E then skips every whip test. Object 1E (C701 bit 3) adds
 * the holy water, thrown in a jump with LEFT or RIGHT for 5 hearts (714D).
 * No SGDK dependency: tested natively. Coordinates are the MSX ones (C427/C425). */

#define VK_WEAPON_SLOTS 2
#define VK_SHOT_DAGGER 2             /* +1: C436 of the throw, or 5 for the holy water */
#define VK_SHOT_AXE 3
#define VK_SHOT_CROSS 4
#define VK_SHOT_WATER 5
#define VK_SHOT_FREE_Y 0xE0          /* 74E9 and 7513: Y of an empty slot */
#define VK_WEAPON_WATER_HEARTS 5     /* 714D: C417 >= 5, then 71B2: - 5 */

/* One slot, the bytes +0..+8 of C450/C460. */
typedef struct {
    VKByte state;                    /* +0: 1 reserved by 71BE (or the first pass), then 2.. */
    VKByte type;                     /* +1: 0 none (reserved slot), VK_SHOT_* */
    signed char dy, dx;              /* +2, +3: added at each pass (74BB) */
    VKByte y, x;                     /* +4, +5 */
    VKByte pattern;                  /* +6: MSX sprite pattern of layer 0 (layer 1 = + 4) */
    VKByte count;                    /* +7: passes in the current state */
    VKByte facing;                   /* +8: C42C at the launch, C468 for the holy water */
} VKShot;

typedef struct {
    VKShot shot[VK_WEAPON_SLOTS];
    VKByte tick;                     /* C003 for the shots: +1 at each pass */
    VKByte phase;                    /* passes run every other frame (phase 0) */
    VKByte launch;                   /* 728C waits for the next pass: C436 to launch, 0 none */
    VKByte block;                    /* C433: mode +4 of the last candle a shot touched */
} VKWeapons;

/* Simon as the shots read him: C427, C425, C420 (ENEMY_SIMON_* values of enemies.h:
 * 1 jump, 2 crouched), C42C. */
typedef struct {
    VKByte x, y, state, facing;
} VKWeaponSimon;

/* What one pass asks of the caller. */
typedef struct {
    VKByte sound[4];                 /* 509F requests, in order */
    VKByte sounds;
    VKByte lost;                     /* 8E93: C416 = 0 and the HUD weapon box cleared */
    VKByte drop;                     /* 74A5 -> 8992: object 1C dropped at (drop_x, drop_y) */
    VKByte drop_x, drop_y;
} VKWeaponEvents;

/* 7513 (room load 639E, door 774F) and 63B7 (state 4 clears C420-C46F, C433 too). */
void vk_weapons_reset(VKWeapons *w);
/* 71BE, at a B press with C416 >= 2 and no stroke: B = 2 slots for the dagger, 1 for
 * the others; the first with +1 = 0 gets +0 = 1. Returns 0 (the press is ignored,
 * no stroke at all) when there is none. */
int vk_weapons_reserve(VKWeapons *w, VKByte weapon);
/* 7286 (C422 = 4, the 4th pass of the stroke): the reserved slot gets +6 = 18h,
 * +8 = C42C and +1 = C436. Done at the next pass of vk_weapons_frame. */
void vk_weapons_launch(VKWeapons *w, VKByte weapon);
/* 714D (C701 bit 3, in a jump, not on a B press): with 5 hearts, LEFT (7194) or RIGHT
 * (71A1) newly pressed throws the holy water from slot C460 if its +1 is 0: C468 = 1
 * for LEFT, C461 = 5, 5 hearts. Returns 1 when thrown. */
int vk_weapons_water(VKWeapons *w, VKByte *hearts, int left);
/* One frame: every other frame, the pass of 72B2 (handlers 7436, 7473, 72DE, 73A4,
 * move 74BB, screen edges 74CE). weapon = C416, map = the room collision map,
 * stage = D000 for the floor test of the holy water (7B98). Returns 1 on a pass
 * frame (the collisions of 801E and 80A6 then follow). */
int vk_weapons_frame(VKWeapons *w, const VKWeaponSimon *simon, VKByte weapon,
                     const VKByte *map, VKByte stage, VKWeaponEvents *ev);
/* 8500: a shot touches the box of a candle or an enemy class (D = X, E = Y of the
 * box, H and L of its class): |X - D| < L + 6 and (Y - E + H) & FF < H + 6 for the
 * dagger, H + 12 for the others. */
int vk_shot_touches(const VKShot *s, int d, int e, int h, int l);
/* 80A6 -> 8460: box of a candle, mode < 2: D = X + 8, E = Y + 16, H = 10h, L = 8;
 * blocks: D = X + 16, E = Y + 32, H = 20h, L = 10h. Returns the slot that touches
 * (C450 first), -1 for none. */
int vk_shot_touches_candle(const VKWeapons *w, VKByte x, VKByte y, VKByte mode);
/* After a hit on a candle (80B9-80CC): C433 = its mode, the dagger goes. */
void vk_weapons_candle_hit(VKWeapons *w, int slot, VKByte mode);
/* After a hit on an enemy (8078): the dagger goes. */
void vk_weapons_enemy_hit(VKWeapons *w, int slot);
/* 74E9: the slot is emptied, Y = E0. */
void vk_weapons_free(VKWeapons *w, int slot);
/* Any object taken (7FFD): C433 = 0. */
void vk_weapons_pick(VKWeapons *w);
/* Sheet frame of generated/weapons_md.h for a slot, -1 when nothing is shown. */
int vk_weapons_frame_index(const VKShot *s);

#endif
