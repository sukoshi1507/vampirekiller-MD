/* Stage 14 raw ROM tables only. Not a playable gameplay stage. */
#ifndef VK_STAGE14_FOUNDATIONS_H
#define VK_STAGE14_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE14_ROOM_COUNT 8
extern const VKByte vk_stage14_tiles[VK_STAGE14_ROOM_COUNT][768];
extern const VKRoomLink vk_stage14_raw_links[VK_STAGE14_ROOM_COUNT];
extern const VKByte vk_stage14_raw_spawn_flags[VK_STAGE14_ROOM_COUNT];
#define VK_STAGE14_DOOR_RAW_FLAGS 135
#define VK_STAGE14_DOOR_ROOM 7
#define VK_STAGE14_DOOR_X 12
#define VK_STAGE14_DOOR_Y 128
#endif
