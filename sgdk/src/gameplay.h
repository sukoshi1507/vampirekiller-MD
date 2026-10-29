#ifndef VK_GAMEPLAY_H
#define VK_GAMEPLAY_H

#include "player.h"

/* Scene routing has no SGDK dependencies and can be unit-tested natively. */
typedef enum { VK_AREA_COURTYARD = 0, VK_AREA_INTERIOR = 1 } VKArea;
typedef enum { VK_EVENT_NONE = 0, VK_EVENT_ROOM = 1, VK_EVENT_ENTER = 2,
               VK_EVENT_VERTICAL = 3,
               VK_EVENT_ENTRANCE = 4,       /* castle door reached: Simon walks in alone */
               VK_EVENT_ENTRANCE_DARK = 5,  /* courtyard left: the screen goes black */
               VK_EVENT_FALL = 6,           /* bottom edge without a room below: life lost */
               VK_EVENT_LAND = 7            /* landing with effect 07: a fall begun on the ground
                                               (6F7E) or a jump ending at C428 >= 21 (6D38) */
             } VKEvent;
/* Castle entrance of courtyard room 2 (research/castle-entrance.md). */
typedef enum { VK_ENTRANCE_NONE = 0,
               VK_ENTRANCE_WALK = 1,        /* C5AC 5: slow walk behind the door jamb */
               VK_ENTRANCE_EXIT = 2,        /* door passed, courtyard still shown */
               VK_ENTRANCE_DARK = 3         /* display off, Simon already in the hall */
             } VKEntrance;
typedef struct {
    VKPlayer player;
    VKArea area;
    VKByte room;
    VKByte collision_verified;
    VKByte stage;            /* castle stage of the interior rooms: 1 (hall) or 2 */
    VKByte entrance;         /* VKEntrance */
    VKByte entrance_step;    /* grounded walk frames since the door (half pixel per 2) */
    VKByte entrance_timer;   /* frames since the door was passed */
    VKByte recoil;           /* VKRecoil: enemy contact recoil (C420 = 5, phase C423) */
    VKByte recoil_count;     /* C42A: arc step in the air, then frames on the floor */
    VKByte recoil_land;      /* C42A on landing: 4, or 16 when the energy is 0 (6FFF) */
    signed char recoil_dir;  /* -1 pushed left (6C2F), 1 pushed right (6C53) */
    VKByte ground_fall;      /* C420 = 4: a fall from the ground (6BA5), not a jump */
    VKByte jump_step;        /* jump past its table (port mode 4): 2 x C428 - 1 at the frame, 0 otherwise */
    VKByte recoil_wait;      /* the recoil moves once a 30 Hz pass: 1 on the frame between two passes */
    VKByte items;            /* C431, set by main.c from candles.c: bit 3 (object 0C) = recoil of 2.5 px,
                              * bit 4 (object 0D) = jump of 21 steps */
    VKByte x_frac;           /* C426: fraction of X, moved by the recoil steps (6C94) */
    VKByte stair_steps;      /* C42B: stair steps, up or down, since the last stair exit (6EF5) */
} VKGame;
#define VK_ITEM_FAST_RECOIL 0x08    /* C431 bit 3: recoil step FD80 / 0280 (6C44, 6C67) */
#define VK_ITEM_HIGH_JUMP 0x10      /* C431 bit 4 (object 0D): jump table 6D94, 21 steps (6CE7) */
/* 6D1C-6D22: a jump step with C428 - 2 >= 13h sets CFF0 := 1 (6D6D), and the landing
 * plays effect 07 only then (6D30-6D38). The table 6D95 has 19 steps: a jump landing
 * at its take-off height lands at step 19, a lower floor at step 21 or later. The
 * table 6D94 (C431 bit 4) has 21: back on the take-off floor at step 21, effect 07. */
#define VK_JUMP_THUD_STEP 21
/* Enemy contact recoil (research/player-health.md, "Recul au coup recu"). */
typedef enum { VK_RECOIL_NONE = 0,
               VK_RECOIL_START = 1,         /* C423 0 (6F93): pose set, no motion */
               VK_RECOIL_AIR = 2,           /* C423 1 or 2: arc of 707D, 2 px a pass (2 frames) */
               VK_RECOIL_FLOOR = 3,         /* C423 3 (701D): on the floor, no control */
               VK_RECOIL_STAIRS = 4,        /* hit on a staircase: C420 = 5 for one pass, then 3 (6FAF) */
               VK_RECOIL_DEAD = 5           /* C420 = 6 after a fatal recoil (7070): death pose until the respawn */
             } VKRecoil;
#define VK_RECOIL_STAIRS_FRAMES 2   /* one pass of state 5 (traced: C420 = 5 at 2 frames) */
#define VK_RECOIL_STEPS 21          /* 6FDA: D = 15h entries in 707D */
#define VK_RECOIL_LAND_TEST 11      /* 6FE5: floor tested from the 11th step on */

void vk_game_begin(VKGame *game);
/* Y where Simon is parked after a fall out of the room (76CB: Y := FA). */
#define VK_FALL_Y 0xFA
/* The castle entrance owns Simon: the player's controls are ignored. */
int vk_game_entrance_locked(const VKGame *game);
VKEvent vk_game_tick_controls(VKGame *game, int direction, int jump,
                              int up, int down);
VKEvent vk_game_tick(VKGame *game, int direction, int jump, int up);
const VKByte *vk_game_collision(const VKGame *game);
/* Loads the ROM map of the current castle room into RAM and returns it (NULL in
 * the courtyard); vk_game_collision then reads it while Simon stays in the room. */
VKByte *vk_game_room_map(const VKGame *game);
/* Writes a 4x4 tile block at (x, y) into the RAM map of the current room, or puts
 * back the ROM tiles there when tiles is NULL. */
void vk_game_room_block(const VKGame *game, VKByte x, VKByte y, const VKByte *tiles);
/* Restart after a lost life (health.c). PROVISIONAL: the MSX respawn place is
 * not traced (research/player-health.md). In the hall Simon restarts at the hall
 * entrance (room 0, x 16, as when he walks in); in the courtyard, at its start. */
void vk_game_respawn(VKGame *game);
/* Enemy contact (7EC1/7F1B): Simon is knocked back in direction dir (-1 left,
 * 1 right) and loses control until the recoil ends; dead = energy now 0. On a
 * staircase with energy left there is no recoil (6FA2-6FB9): Simon stays on his
 * step and only loses the pass of the hit; with no energy left he is knocked off
 * the staircase like on the ground (6FAD -> 6FBC). */
void vk_game_hit(VKGame *game, int dir, int dead);
int vk_game_recoiling(const VKGame *game);
/* Start of a castle stage after the door of the stage before (items.c): room 0,
 * Simon standing at x 16, y 96 (stage 2, research/door-room7.md section 16). */
void vk_game_enter_stage(VKGame *game, VKByte stage);
/* Explicit debug room selection in the current stage; returns 0 for an
 * unsupported room. */
int vk_game_debug_room(VKGame *game, VKByte room);

#endif
