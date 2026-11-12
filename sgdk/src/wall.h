#ifndef VK_WALL_H
#define VK_WALL_H

#include "candles.h"
#include "generated/trade_data.h"

/* Wall objects (the old man in the wall, records C5B5 and C5C5) and the merchant
 * of stages0..18 (research/merchant.md, research/trade-late.md; bank2,9179-954F, and
 * openMSX traces). No SGDK dependency: tested natively. Coordinates are the MSX
 * ones (X/Y of the record: top left of the 32 x 32 frame copied by 925E). */

#define VK_WALL_RECORDS 2            /* C5B5, C5C5 */
#define VK_WALL_SHOWN 0x82           /* +0 = 82h: shown, can be hit (80E5, 928D) */
#define VK_WALL_ANIM 0x03            /* +0 = 3: animation of 9229 (+0A = 20h passes) */
#define VK_WALL_NEW 0x01             /* +0 = 1: made by 9179, shown by 91EA at the next 91BE */
#define VK_WALL_ANIM_PASSES 0x20     /* 920C */
#define VK_WALL_BLINK_FRAME 2        /* 9231: frame 2 every other pass of the animation */
#define VK_WALL_FIRST_FRAME 3        /* 925C: frame shown by 91EA */
#define VK_WALL_POINTS 5000          /* 9372: DE = 5000h (BCD), 44EC */
#define VK_SHOP_TIMEOUT 0x14         /* 938A: C706 */
#define VK_SHOP_CLOSE_PASSES 0x0F    /* 43F6: C004 = 0Fh */
#define VK_SHOP_W 0x50               /* 93BE: 80 x 32 window (saved by 4946 at B080) */
#define VK_SHOP_H 0x20

/* C70C, the result of a hit (92A7 table). */
typedef enum {
    VK_WALL_MERCHANT = 0,            /* 9327: C40C = FF, state 0C */
    VK_WALL_D012_UP = 1,             /* 9333: D012 + 1, max 3 */
    VK_WALL_D012_DOWN = 2,           /* 933C: D012 - 1, min 0 */
    VK_WALL_HEARTS_UP = 3,           /* 9344: effect 0F, hearts + 5 */
    VK_WALL_HEARTS_DOWN = 4,         /* 934E: effect 1D, hearts - 5 */
    VK_WALL_NOTHING = 5,             /* 9343 */
    VK_WALL_GONE = 6                 /* 9358: removed for good, effect 10, 5000 points */
} VKWallResult;

/* One record: the bytes of C5B5/C5C5 that the routines read. */
typedef struct {
    VKByte state;                    /* +0: 0, VK_WALL_NEW, VK_WALL_SHOWN or VK_WALL_ANIM */
    VKByte y, x;                     /* +1 (E), +2 (D) */
    VKByte hits;                     /* +3: contacts since the last 91F2 (FF: object 05) */
    VKByte row;                      /* +5: C = entry & 3, row of the table 9300 */
    VKByte timer;                    /* +0A: passes of the animation */
    VKByte frame;                    /* frame drawn last by 925E (0..4) */
    VKByte flag;                     /* +7/+8, the stream flag 9358 clears: candle index + 1
                                      * of the hidden block, or 0x80 | direct def bit + 1 */
} VKWallObject;

typedef struct {
    VKWallObject obj[VK_WALL_RECORDS];
    VKByte stage, room;              /* room of the records, FF before the first */
    /* DE00 (5AAF, 944F): per room, item and count of slot 0 and 1. The count byte
     * (+1) has the number of hits in bits 0..6 and bit 7 while the merchant waits
     * for an answer (9407); it lasts until the stream is decoded again (5A49:
     * new life or stage start). */
    /* Keep early rows at12: late rows were appended by LOT08. */
    VKByte count[12][VK_CANDLE_ROOMS][VK_WALL_RECORDS];
    VKByte gone[12][VK_CANDLE_ROOMS];   /* direct wall objects removed (9358) */
    VKByte pad2;                     /* even offsets for the 68000 since the stage 11 rows (count and gone grew by 27 bytes) */
    VKByte c70c, c70b;               /* result and frame of the last hit (92BB) */
    VKByte d012;                     /* D012: spawn speed (9CD8), changed by 9333/933C */
    VKByte phase;                    /* 91BE runs once a pass (2 frames): 1 between */
    VKByte pad;                      /* was the stand-in of R (92E8); kept so that shop and slot stay at the offsets the 68000 reads (even) and the scenarios poke */
    /* Merchant (state 0C, 43F0): one pass a frame. */
    VKByte shop;                     /* VKShopState */
    VKByte slot;                     /* C703: record (and DE00 slot) of the merchant */
    VKByte sale;                     /* C708: object sold (93FF) */
    VKByte price;                    /* C707: price in hearts, BCD (9418) */
    VKByte timeout;                  /* C706: 14h, - 1 when C003 & 1F = 0 (94BA) */
    VKByte delay;                    /* C004: passes before the window goes (43FD) */
    VKByte keys;                     /* C709: keys of the last read (951F) */
    VKByte c003;                     /* C003 for the shop: + 1 at each pass */
    VKByte bought;                   /* 1 when the last answer was a purchase */
    /* LOT08 appended: preserve all offsets used by the MD scenarios. */
    VKByte late_count[7][VK_TRADE_ROOMS][VK_WALL_RECORDS];
    VKByte late_gone[7][VK_TRADE_ROOMS];
    VKByte late_block_gone[7][VK_TRADE_ROOMS];
} VKWall;

typedef enum {
    VK_SHOP_NONE = 0,
    VK_SHOP_OPEN = 1,                /* C40C = FF: state 0C, C001 = 0 at the next pass (440A) */
    VK_SHOP_WAIT = 2,                /* C001 = 1: 94BA reads the answer */
    VK_SHOP_CLOSE = 3                /* C001 = 2: 15 passes, then 9507 and state 5 */
} VKShopState;

/* What vk_wall_step asks of the caller. */
typedef struct {
    VKByte redraw;                   /* bit i: record i changed frame or went */
    VKByte sound[2];                 /* 509F requests, in order */
    VKByte sounds;
    VKByte hearts;                   /* hearts changed (HUD) */
    VKByte points;                   /* 5000 points to add */
    VKByte shop;                     /* the merchant opens (C40C = FF) */
    VKByte block_gone;               /* candle index + 1 of a hidden block removed for good */
} VKWallEvents;

/* Sale prices of 9428: object, then the price normal, with 11 (C702 bit 7), with
 * 10 (C702 bit 6), BCD hearts. */
/* ROM9428 contains nine products, shared by all19 stages; generated declaration above. */
/* 5B0B: object sold for B = (entry & 3C) >> 2. */
/* All16 selector values, including the seven reserved zeros, are generated. */

/* New game (44C6): records, counts, D012 and the shop at 0. */
void vk_wall_begin(VKWall *w);
/* 5A49 decodes the stage stream again (new life, stage start): DE00 counts and
 * the removed objects of that stage come back. */
void vk_wall_reset_stage(VKWall *w, VKByte stage);
/* Room load (5B1B rewrites C420-C5FF): the records go, then each direct wall
 * object of the room not removed is made (5BB7 -> 9179) and shown (91EA). */
void vk_wall_enter(VKWall *w, VKByte stage, VKByte room);
/* 8814 -> 9179: hidden block i of the room (0x1F, +9 & C0 = C0) was broken: its
 * wall object is made and shown in the same pass (91BE after 8671). Returns the
 * record, -1 when DE00 has no slot for its object (91A2: nothing). */
/* Stages 12-18: 0 when hidden block `candle` of the room was already broken
 * (late_block_gone), so vk_wall_block makes nothing; always 1 before stage 12.
 * vk_wall_block queries it; checked on the ROM by tools/verify_trade_trace.py. */
int vk_wall_block_present(const VKWall *w,VKByte stage,VKByte room,int candle);
int vk_wall_block(VKWall *w, int candle, const VKCandleDef *def);
/* 80E5: record i is shown (+0 bit 7) and can be hit. */
int vk_wall_hittable(const VKWall *w, int i);
/* 80E5 -> 83A1: the whip (C422 = 3, C416 < 2) touches record i; the box is the
 * candle box of 8450 at (X + 8, Y + 16), D = X + 16, E = Y + 32. */
int vk_wall_whip_touch(const VKWall *w, int i, const VKWhip *whip, int weapon);
/* 8112: + 1 to the contacts of record i (effect 0C by the caller). */
void vk_wall_hit(VKWall *w, int i);
/* 928D (object 05): every shown record gets +3 = FF (removed for 5000 points). */
void vk_wall_cross(VKWall *w);
/* One frame of 91BE (once a pass, 2 frames; a pass with contacts runs at once).
 * c gives the hearts (C417) and the stream flags of the hidden blocks. */
void vk_wall_step(VKWall *w, VKCandles *c, VKWallEvents *ev);
/* Object of the frame now drawn for record i, -1 when nothing is drawn. */
int vk_wall_frame(const VKWall *w, int i);

/* State 0C, one pass a frame. keys: bit 0 SPACE / button 1 (yes), bit 1 SHIFT /
 * button 2 (no), as 951F reads them (held keys; only new presses count).
 * Returns VK_CANDLE_EVENT bits of the purchase (vk_candles_give) and fills ev
 * (sounds 19, 12, 02; hearts). shop goes back to VK_SHOP_NONE at the end. */
int vk_shop_step(VKWall *w, VKCandles *c, VKByte keys, VKWallEvents *ev);
/* 9397: window top left (C704/C705) for record slot: X - 10h (X < 80h) or X - 20h,
 * Y - 18h. */
void vk_shop_window(const VKWall *w, VKByte *x, VKByte *y);

#endif
