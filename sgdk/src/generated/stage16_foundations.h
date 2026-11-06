/* Stage 16 raw ROM tables only. Not a playable gameplay stage. */
#ifndef VK_STAGE16_FOUNDATIONS_H
#define VK_STAGE16_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE16_ROOM_COUNT 10
extern const VKByte vk_stage16_tiles[VK_STAGE16_ROOM_COUNT][768];
extern const VKRoomLink vk_stage16_raw_links[VK_STAGE16_ROOM_COUNT];
extern const VKByte vk_stage16_raw_spawn_flags[VK_STAGE16_ROOM_COUNT];
#define VK_STAGE16_DOOR_RAW_FLAGS 133
#define VK_STAGE16_DOOR_ROOM 5
#define VK_STAGE16_DOOR_X 12
#define VK_STAGE16_DOOR_Y 64
#endif
