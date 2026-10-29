#ifndef VK_CANDLES_H
#define VK_CANDLES_H

#include "gameplay.h"
#include "whip.h"
#include "generated/candles_md.h"

/* Candles, dropped objects and fixed objects of stages 0-2
 * (research/castle-candles.md, section 9; ROM code read, banks 1 and 2).
 * No SGDK dependency: tested natively. Coordinates are the MSX ones (C427/C425
 * for Simon, X/Y of the C470 and C500 records), which are also the MD sprite
 * coordinates. */

#define VK_CANDLE_MAX 8
#define VK_DROP_MAX 8                /* C500-C570 */
#define VK_CANDLE_STAGES 3
#define VK_CANDLE_ROOMS 8

#define VK_OBJ_SMALL_HEART 0x01
#define VK_OBJ_BIG_HEART 0x02
#define VK_OBJ_NOTHING 0x15          /* no flame, no object (on the MSX: a hidden enemy) */
#define VK_OBJ_GOLD_KEY 0x17
#define VK_OBJ_WHITE_KEY 0x18
#define VK_OBJ_CHEST 0x19
#define VK_OBJ_CHAIN_WHIP 0x1A       /* 1A..1D: C416 = object - 0x19 */
#define VK_OBJ_BLOCK_CONTENT 0x1F    /* hidden block whose content is its byte +9 (8814) */
#define VK_OBJ_CANDLE_FRAME 0x21     /* wall candle, frames 0x21 and 0x22 of the sheet */
#define VK_CANDLE_HIDDEN_BLOCK 3     /* mode +4: a 4x4 brick block written in the map */

/* Tiles of the hidden block (86CA, table 8792 + 4 for D002 = 0, stages 0-3), row by row. */
extern const VKByte vk_hidden_block_tiles[16];

typedef enum {
    VK_DROP_NONE = 0,
    VK_DROP_BURST = 1,               /* type 01: 16 frames at the candle (8A99) */
    VK_DROP_FALL = 2,                /* type 82: falls to a floor (8AF8) */
    VK_DROP_REST = 3,                /* type 84: on the floor, can be picked up (8BA2) */
    VK_DROP_WAIT = 4                 /* type 05: content of an opened chest, 32 frames (8BCC) */
} VKDropState;

typedef struct {
    VKByte state;                    /* VKDropState */
    VKByte x, y;                     /* C500 +2, +1 */
    VKByte kind;                     /* +4 */
    VKByte timer;                    /* +6 */
    VKByte fixed;                    /* fixed object index + 1, or 0x80 | (candle index + 1)
                                      * for the chest of a hidden block, 0 for a drop */
    VKByte content;                  /* chest: object inside (+13) */
    VKByte catchable;                /* type bit 7 while falling: 82 from a candle,
                                      * not 02 from a chest (7FE2) */
    unsigned short xfrac;            /* small heart: X in 8.8 (+2, +8) */
    short vx, ax;                    /* small heart: speed (+9, +10) and its change (+11, +12) */
} VKDrop;

typedef enum {
    VK_CANDLE_EVENT_NONE = 0,
    VK_CANDLE_EVENT_HIT = 1,         /* a candle went out (redraw the candles) */
    VK_CANDLE_EVENT_HEARTS = 2,      /* heart count changed */
    VK_CANDLE_EVENT_WHITE_KEY = 4,   /* the hidden white key block was hit (items.c) */
    VK_CANDLE_EVENT_WEAPON = 8,      /* C416 changed */
    VK_CANDLE_EVENT_GOLD_KEY = 16    /* gold key count changed (key taken or chest opened) */
} VKCandleEvent;

typedef struct {
    VKByte stage, room;              /* room the candles belong to, 0xFF before the first */
    VKByte first, count;             /* vk_candle_defs[first .. first + count) */
    VKByte lit;                      /* bit i: candle i of the room still there */
    VKByte clock;
    VKDrop drop[VK_DROP_MAX];
    VKByte hearts;                   /* C417 (decimal here, BCD on the MSX), max 99 */
    VKByte gold_keys;                /* C700 */
    VKByte weapon;                   /* C416 */
    VKByte out[VK_CANDLE_STAGES][VK_CANDLE_ROOMS];    /* candles hit, per room */
    VKByte taken[VK_CANDLE_STAGES][VK_CANDLE_ROOMS];  /* fixed objects picked up */
    VKByte hud_key;                  /* object 8EC9 last drew at (148, 12) in the HUD, 0 none */
} VKCandles;

void vk_candles_begin(VKCandles *c);
/* Entering a stage again (a new stage start): every candle and fixed object of
 * that stage is back. PROVISIONAL: the MSX keeps them in the decoded stage stream
 * (flag pointers +6/+7, +14/+15), whose reload is not traced. */
void vk_candles_reset_stage(VKCandles *c, VKByte stage);
/* New life (state 4): 62E6 decodes the stage stream again (5A49), so every
 * candle and fixed object of the stage is back, and 70DC clears the weapon
 * (C416) and the gold keys (C700). The hearts stay. */
void vk_candles_new_life(VKCandles *c, VKByte stage);
/* End of a death (70BF, state 6): C700 = 0. The HUD key stays drawn until the
 * HUD is drawn again (vk_candles_hud). */
void vk_candles_death(VKCandles *c);
/* HUD drawn again (4513 -> 8EB4 -> 8EC9: state 4 at 4237, game over at 42F8):
 * the gold key icon while C700 > 0 (and C701 bit 1 or 2), else nothing. */
void vk_candles_hud(VKCandles *c);
/* Debug (A+B): every candle of the current room is lit again. */
void vk_candles_relight(VKCandles *c);
/* Stage index of vk_candle_first: 0 courtyard, then the castle stage. */
VKByte vk_candles_stage_of(const VKGame *game);
/* Follows the current room: on a change, the drops go and the fixed objects of
 * the new room are placed. */
void vk_candles_enter(VKCandles *c, const VKGame *game);
const VKCandleDef *vk_candles_def(const VKCandles *c, int i);
int vk_candles_lit(const VKCandles *c, int i);
/* One frame: whip hits (phase 2 of whip.c stands for C422 = 3), drops, pickups.
 * Returns VKCandleEvent bits. Writes the touched candle index to *hit_index. */
int vk_candles_tick(VKCandles *c, VKGame *game, const VKWhip *whip, int *hit_index);
/* Whip test of the ROM (8450/84A7): exported for the courtyard torches drawn by
 * room_view.c and for the tests. */
int vk_candle_whip_touch(const VKCandleDef *def, const VKWhip *whip, int weapon);
/* Puts out candle i of the current room and creates its drop (898F). */
int vk_candles_hit(VKCandles *c, int i);
/* Sheet object of a drop to draw, 0 for none. Bursting drops show nothing here
 * (the flame is drawn by the caller); falling ones show the small heart (E8) or
 * the orb (EC), PROVISIONAL stand-ins taken from the sheet. */
VKByte vk_candles_drop_sprite(const VKDrop *d, VKByte clock);

#endif
