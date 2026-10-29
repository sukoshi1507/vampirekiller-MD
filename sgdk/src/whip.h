#ifndef VK_WHIP_H
#define VK_WHIP_H
#include "player.h"
/* C416: 0 the leather whip, 1 the chain whip (object 1A), 2..4 the dagger, axe and
 * cross (objects 1B..1D), thrown by weapons.c: their stroke is the throw pose (upper
 * compositions 12, 13, 14, the level 2 frames) and no whip test is made (7D7E). */
#define VK_WHIP_LEVEL_COUNT 5
#define VK_WHIP_LEVEL_THROW 2
/* Timer where a throw ends (C422 = 4 -> 7286, the 4th pass of 2 frames): the slot
 * is launched and the stroke is over. */
#define VK_WHIP_THROW_END 8
typedef struct { VKByte timer; VKByte facing; VKByte x; VKByte y; VKByte crouched; VKByte level; } VKWhip;
void vk_whip_reset(VKWhip *w);
/* One frame. Returns 1 on the frame a throw (level >= 2) ends: launch the shot. */
int vk_whip_tick(VKWhip *w, const VKPlayer *p, int attack_pressed);
/* Selects the whip level (0..VK_WHIP_LEVEL_COUNT-1). Returns -1 and keeps the
 * current level when out of range. Reset returns to level 0. */
int vk_whip_set_level(VKWhip *w, int level);
int vk_whip_level(const VKWhip *w);
/* Timer of the only frame where the whip touches (the MSX pass with C422 = 3). */
#define VK_WHIP_STRIKE_TIMER 11
/* Nonzero on the frame where the whip tests candles, enemies and targets; never for
 * levels >= 2 (7D7E). */
int vk_whip_active(const VKWhip *w);
/* Visual arm keyframe: 0 hidden, 1 windup, 2 short, 3 full, 4 recovery. */
int vk_whip_arm_frame(const VKWhip *w);
/* Experimental ROM-derived animation step (0..7); independent of hitbox timing. */
int vk_whip_visual_step(const VKWhip *w);
/* ROM-derived crouch/crouch-whip step (0..3): 0 idle, 1..3 attack (see whip.c). */
int vk_whip_crouch_visual_step(const VKWhip *w);
/* Position of the dedicated animated arm sprite (screen pixel coordinates). */
void vk_whip_arm_position(const VKWhip *w, int *x, int *y);
/* Inclusive hitbox (pixels); NULL outputs forbidden. */
void vk_whip_bounds(const VKWhip *w, int *left, int *top, int *right, int *bottom);
#endif
