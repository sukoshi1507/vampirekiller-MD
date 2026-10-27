#ifndef VK_FRONTEND_H
#define VK_FRONTEND_H

#include <genesis.h>
#include "gameplay.h"
#include "whip.h"

/* Front-end mode and play state, owned by main.c and shared with the display
 * modules (scene.c, room_view.c, actors.c). */
enum { FRONT_LOGO, FRONT_TITLE, FRONT_CASTLE, FRONT_COURTYARD,
       FRONT_INTERIOR, FRONT_VIEWER, FRONT_TEST, FRONT_SOUND };
extern u16 vk_front_mode;
extern VKGame vk_game;
/* Copy of vk_game.player drawn on screen (the movement test drives it alone). */
extern VKPlayer vk_player;
extern VKWhip vk_whip;

#endif
