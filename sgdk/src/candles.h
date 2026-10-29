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
    VK_CANDLE_EVENT_GOLD_KEY = 16,   /* gold key count changed (key taken or chest opened) */
    VK_CANDLE_EVENT_ITEM = 32,       /* C431 changed (objects 0B, 0C, 0D) */
    VK_CANDLE_EVENT_PICK = 64,       /* an object whose effect lives in enemies.c was taken
                                      * (03-09, 16, 1A-1D): see picked, Enemies_item */
    VK_CANDLE_EVENT_BOX_ITEM = 128   /* C701 bit 3, 6 or 7 set (objects 1E, 0A, 0F) */
} VKCandleEvent;

/* C701 bits 7..3 shown in the HUD item box (8EE6, table 8F19). */
#define VK_ITEM_BIT_0F 0x80          /* 8E3D, x E8 */
#define VK_ITEM_BIT_0A 0x40          /* 8D94, x D8 */
#define VK_ITEM_BIT_04 0x20          /* 8DA1, x C8 (bit 4 cleared) */
#define VK_ITEM_BIT_03 0x10          /* 8D98, x C8 (bit 5 cleared): the armor */
#define VK_ITEM_BIT_1E 0x08          /* 8D8D, x B8 */
#define VK_ITEM_BITS 0xF8
#define VK_ITEM_SHIELD 0x30          /* 7618: Simon holds the shield while C701 & 30h */
#define VK_ITEM_BOX_Y 12             /* 8F0F: E = 0C */
#define VK_ITEM_PICK_X 0xD0          /* 8F3D: the object just taken, at D0 */
#define VK_ITEM_PICK_TICKS 0x20      /* 8F35: C5E6 */
/* 8F19: x and object for C701 bit 7 - i, i = 0..4. */
extern const VKByte vk_item_box[5][2];

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
    VKByte c431;                     /* C431: bits 2, 3, 4 set by objects 0B, 0C, 0D (8DF5-8E03) */
    VKByte picked;                   /* C419: last object taken (8D30), 0 none */
    VKByte c701;                     /* C701 bits 7..3 (VK_ITEM_BITS): objects 0F, 0A, 04, 03, 1E;
                                      * bits 0..2 (keys) live in items.c and gold_keys */
    VKByte box_items;                /* C701 bits 7..3 that 8EE6 last drew in the item box */
    VKByte box_pick;                 /* object 8F2D drew at (D0, 12) in the box, 0 none */
    VKByte box_timer_on;             /* C5E5 = FF: 8F55 counts C5E6 down */
    VKByte box_timer;                /* C5E6 */
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
/* End of a death (70BF, state 6): C700 = 0, C701 &= 80h. The HUD key and the
 * item box stay drawn until the HUD is drawn again (vk_candles_hud, 8EE6). */
void vk_candles_death(VKCandles *c);
/* HUD drawn again (4513 -> 8EB4 -> 8EC9: state 4 at 4237, game over at 42F8):
 * the gold key icon while C700 > 0 (and C701 bit 1 or 2), else nothing. */
void vk_candles_hud(VKCandles *c);
/* 8EE6 (room load 6316, HUD redraw 4513, armor used up 81A4): the item box
 * (x B8..F7, y 12..27) is cleared and the objects of C701 bits 7..3 drawn. */
void vk_candles_item_box(VKCandles *c);
/* 819E: C441 ran out with the armor (object 03): C701 bit 4 cleared, then 8EE6. */
void vk_candles_armor_out(VKCandles *c);
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
/* Sound request returned by the 8D3E handler of an object (A at its RET, played by
 * 509F): 0F hearts, 1B object 05, 16 object 08, 10 objects 13-14, 14 keys, 12 the
 * others; 0 for 00, 15 and 19 (no effect). */
VKByte vk_candles_sound(VKByte kind);
/* Sheet object of a drop to draw, 0 for none. Bursting drops show nothing here
 * (the flame is drawn by the caller); falling ones show the small heart (E8) or
 * the orb (EC), PROVISIONAL stand-ins taken from the sheet. */
VKByte vk_candles_drop_sprite(const VKDrop *d, VKByte clock);

#endif
