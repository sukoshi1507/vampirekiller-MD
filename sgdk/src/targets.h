#ifndef VK_TARGETS_H
#define VK_TARGETS_H
#include "whip.h"
typedef struct { int x, y, width, height; VKByte active, hit_latched; } VKTarget;
void vk_target_init(VKTarget *target, int x, int y, int width, int height);
/* Returns 1 only on a NEW hit; one swing cannot score twice. */
int vk_target_tick(VKTarget *target, const VKWhip *whip);
/* Multi-target form: ticks each target of the list against the same swing.
 * Returns the index of the first target hit by this swing, or -1. Each target
 * keeps its own latch, so a swing scores at most once per target. */
int vk_targets_tick(VKTarget *targets, int count, const VKWhip *whip);
#endif
