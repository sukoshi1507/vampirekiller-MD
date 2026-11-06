/* Raw ROM staging: stage 13 is NOT an enabled gameplay stage. */
#ifndef VK_STAGE13_FOUNDATIONS_H
#define VK_STAGE13_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE13_ROOM_COUNT 12
extern const VKByte vk_stage13_tiles[VK_STAGE13_ROOM_COUNT][768];
extern const VKRoomLink vk_stage13_raw_links[VK_STAGE13_ROOM_COUNT];
extern const VKByte vk_stage13_raw_spawn_flags[VK_STAGE13_ROOM_COUNT];
#endif
