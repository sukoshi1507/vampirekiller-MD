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
} VKGame;

void vk_game_begin(VKGame *game);
VKEvent vk_game_tick_controls(VKGame *game, int direction, int jump,
                              int up, int down);
VKEvent vk_game_tick(VKGame *game, int direction, int jump, int up);
const VKByte *vk_game_collision(const VKGame *game);
/* Explicit debug room selection; returns 0 for unsupported room. */
int vk_game_debug_room(VKGame *game, VKByte room);

#endif
