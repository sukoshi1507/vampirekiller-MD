/* Stage 15 raw ROM tables only. Not a playable gameplay stage. */
#ifndef VK_STAGE15_FOUNDATIONS_H
#define VK_STAGE15_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE15_ROOM_COUNT 10
extern const VKByte vk_stage15_tiles[VK_STAGE15_ROOM_COUNT][768];
extern const VKRoomLink vk_stage15_raw_links[VK_STAGE15_ROOM_COUNT];
extern const VKByte vk_stage15_raw_spawn_flags[VK_STAGE15_ROOM_COUNT];
#define VK_STAGE15_DOOR_RAW_FLAGS 136
#define VK_STAGE15_DOOR_ROOM 8
#define VK_STAGE15_DOOR_X 12
#define VK_STAGE15_DOOR_Y 128
#endif
