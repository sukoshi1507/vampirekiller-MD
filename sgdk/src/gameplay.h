#ifndef VK_GAMEPLAY_H
#define VK_GAMEPLAY_H

#include "player.h"

/* Scene routing has no SGDK dependencies and can be unit-tested natively. */
typedef enum { VK_AREA_COURTYARD = 0, VK_AREA_INTERIOR = 1 } VKArea;
typedef enum { VK_EVENT_NONE = 0, VK_EVENT_ROOM = 1, VK_EVENT_ENTER = 2,
               VK_EVENT_VERTICAL = 3,
               VK_EVENT_ENTRANCE = 4,       /* castle door reached: Simon walks in alone */
               VK_EVENT_ENTRANCE_DARK = 5,  /* courtyard left: the screen goes black */
               VK_EVENT_FALL = 6            /* bottom edge without a room below: life lost */
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
} VKGame;
/* Enemy contact recoil (research/player-health.md, "Recul au coup recu"). */
typedef enum { VK_RECOIL_NONE = 0,
               VK_RECOIL_START = 1,         /* C423 0 (6F93): pose set, no motion */
               VK_RECOIL_AIR = 2,           /* C423 1 or 2: arc of 707D, 2 px a frame */
               VK_RECOIL_FLOOR = 3          /* C423 3 (701D): on the floor, no control */
             } VKRecoil;
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
 * staircase there is no recoil (6FA2: C420 := 3, PROVISIONAL, not ported). */
void vk_game_hit(VKGame *game, int dir, int dead);
int vk_game_recoiling(const VKGame *game);
/* Start of a castle stage after the door of the stage before (items.c): room 0,
 * Simon standing at x 16, y 96 (stage 2, research/door-room7.md section 16). */
void vk_game_enter_stage(VKGame *game, VKByte stage);
/* Explicit debug room selection in the current stage; returns 0 for an
 * unsupported room. */
int vk_game_debug_room(VKGame *game, VKByte room);

#endif
