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
} VKGame;

void vk_game_begin(VKGame *game);
/* Y where Simon is parked after a fall out of the room (76CB: Y := FA). */
#define VK_FALL_Y 0xFA
/* The castle entrance owns Simon: the player's controls are ignored. */
int vk_game_entrance_locked(const VKGame *game);
VKEvent vk_game_tick_controls(VKGame *game, int direction, int jump,
                              int up, int down);
VKEvent vk_game_tick(VKGame *game, int direction, int jump, int up);
const VKByte *vk_game_collision(const VKGame *game);
/* Restart after a lost life (health.c). PROVISIONAL: the MSX respawn place is
 * not traced (research/player-health.md). In the hall Simon restarts at the hall
 * entrance (room 0, x 16, as when he walks in); in the courtyard, at its start. */
void vk_game_respawn(VKGame *game);
/* Enemy contact recoil of dx pixels. PROVISIONAL (research/player-health.md):
 * Simon moves pixel by pixel with the walk limits (x 8..248, side wall probes)
 * and stops at the first blocked pixel; no recoil on a staircase. */
void vk_game_push(VKGame *game, int dx);
/* Start of a castle stage after the door of the stage before (items.c): room 0,
 * Simon standing at x 16, y 96 (stage 2, research/door-room7.md section 16). */
void vk_game_enter_stage(VKGame *game, VKByte stage);
/* Explicit debug room selection in the current stage; returns 0 for an
 * unsupported room. */
int vk_game_debug_room(VKGame *game, VKByte room);

#endif
