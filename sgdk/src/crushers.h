/* Spiked crushers of stage 6, room 1: records C580, routines 8F9A, 8FEA and 85A6 (research/stage6-enemies.md). */
#ifndef VK_CRUSHERS_H
#define VK_CRUSHERS_H
#include "player.h"

#define VK_CRUSHER_MAX 3
/* C580 + 8n: state (0 = none, 1 = going down, 2 = going up), Y, X, mask, pass counter, limit, step counter. */
typedef struct { VKByte state, y, x, mask, pass, limit, count; } VKCrusher;
typedef struct { VKCrusher slot[VK_CRUSHER_MAX]; VKByte room; } VKCrushers;

/* The room of the crushers (stage 6, room 1) or none; the records appear on the first pass (8F9A). */
void vk_crushers_room(VKCrushers *c, VKByte stage, VKByte room);
/* 8FCF, once per pass of 2 frames: 8F9A creates the records when absent, 8FD2 moves them. */
void vk_crushers_step(VKCrushers *c);
/* 85A6, after the move: the damage (8 going up, 16 going down) of the crusher over Simon's head, 0 for none.
 * The caller applies the C42D, C43A and C420 = 6 guards. */
int vk_crushers_hit(const VKCrushers *c, VKByte simon_x, VKByte simon_y);

#endif
