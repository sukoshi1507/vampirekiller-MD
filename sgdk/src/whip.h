#ifndef VK_WHIP_H
#define VK_WHIP_H
#include "player.h"
typedef struct { VKByte timer; VKByte facing; VKByte x; VKByte y; } VKWhip;
void vk_whip_reset(VKWhip *w);
void vk_whip_tick(VKWhip *w, const VKPlayer *p, int attack_pressed);
int vk_whip_active(const VKWhip *w);
/* 0 hidden/windup, 1 short extension, 2 full extension. */
int vk_whip_phase(const VKWhip *w);
/* Number of visible eight-pixel segments, matching the hitbox. */
int vk_whip_segments(const VKWhip *w);
/* Visual arm keyframe: 0 hidden, 1 windup, 2 short, 3 full, 4 recovery. */
int vk_whip_arm_frame(const VKWhip *w);
/* Position of the dedicated animated arm sprite (screen pixel coordinates). */
void vk_whip_arm_position(const VKWhip *w, int *x, int *y);
/* Inclusive hitbox (pixels); NULL outputs forbidden. */
void vk_whip_bounds(const VKWhip *w, int *left, int *top, int *right, int *bottom);
#endif
