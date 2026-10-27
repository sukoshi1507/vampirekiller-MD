#ifndef VK_GAMEPLAY_H
#define VK_GAMEPLAY_H

#include "player.h"

/* Scene routing has no SGDK dependencies and can be unit-tested natively. */
typedef enum { VK_AREA_COURTYARD = 0, VK_AREA_INTERIOR = 1 } VKArea;
typedef enum { VK_EVENT_NONE = 0, VK_EVENT_ROOM = 1, VK_EVENT_ENTER = 2,
               VK_EVENT_VERTICAL = 3 } VKEvent;
typedef struct {
    VKPlayer player;
    VKArea area;
    VKByte room;
    VKByte collision_verified;
    VKByte stage;            /* castle stage of the interior rooms: 1 (hall) or 2 */
} VKGame;

void vk_game_begin(VKGame *game);
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
