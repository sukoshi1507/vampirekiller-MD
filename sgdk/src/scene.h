#ifndef VK_SCENE_H
#define VK_SCENE_H

#include <genesis.h>

/* Logo fade, title screen and castle intro (ROM scenes 0..2). */
void vk_logo_start(void);
/* One logo frame: fade in, hold, fade out, then the title (START skips). */
void vk_logo_tick(u16 pressed);
void vk_show_title(void);
void vk_begin_castle(void);
/* One castle intro frame. Returns TRUE when the intro ends (START or timeout):
 * the caller then starts the game. */
bool vk_castle_tick(u16 pressed);

#endif
