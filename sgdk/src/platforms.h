/* Moving platforms of stage 5, rooms 1 and 4: records C598 and C59F (research/stage5-enemies.md). */
#ifndef VK_PLATFORMS_H
#define VK_PLATFORMS_H
#include "player.h"

#define VK_PLATFORM_MAX 2
#define VK_PLATFORM_CELLS 2   /* two 16 px cells, X and X + 10h (9112) */
/* C598 + 0..6: id (0 = none), Y, X, step, length, pass counter, step counter. */
typedef struct { VKByte id, y, x; signed char dx; VKByte length, pass, count; } VKPlatform;
typedef struct { VKPlatform slot[VK_PLATFORM_MAX]; VKByte under; } VKPlatforms;

/* 902D: the records of a room, none for a room without platform. */
void vk_platforms_room(VKPlatforms *p, VKByte stage, VKByte room);
/* 90B8, once per pass of 2 frames. */
void vk_platforms_step(VKPlatforms *p);
/* 8524: C439 for Simon at (x, y). It keeps its value during the first falling passes. */
void vk_platforms_probe(VKPlatforms *p, VKByte x, VKByte y, int falling, VKByte fall_phase);
/* 6BAF: the step (+1 or -1) of the platform Simon stands on, 0 for none. */
int vk_platforms_carry(const VKPlatforms *p);

#endif
