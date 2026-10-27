#ifndef VK_WHIP_H
#define VK_WHIP_H
#include "player.h"
/* Whip levels with a ROM-derived standing asset: 0 (wired since the first frames),
 * 1 and 2. Level 3 has no asset and is refused (see research/whip-variants.md). */
#define VK_WHIP_LEVEL_COUNT 3
typedef struct { VKByte timer; VKByte facing; VKByte x; VKByte y; VKByte crouched; VKByte level; } VKWhip;
void vk_whip_reset(VKWhip *w);
void vk_whip_tick(VKWhip *w, const VKPlayer *p, int attack_pressed);
/* Selects the whip level (0..VK_WHIP_LEVEL_COUNT-1). Returns -1 and keeps the
 * current level when the level has no asset. Reset returns to level 0. */
int vk_whip_set_level(VKWhip *w, int level);
int vk_whip_level(const VKWhip *w);
int vk_whip_active(const VKWhip *w);
/* 0 hidden/windup, 1 short extension, 2 full extension. */
int vk_whip_phase(const VKWhip *w);
/* Number of visible eight-pixel segments, matching the hitbox. */
int vk_whip_segments(const VKWhip *w);
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
