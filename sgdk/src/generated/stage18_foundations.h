/* ROM MSX2 raw stage 18 static data. NOT a playable stage. */
#ifndef VK_STAGE18_FOUNDATIONS_H
#define VK_STAGE18_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE18_ROOM_COUNT 10
extern const VKByte vk_stage18_tiles[VK_STAGE18_ROOM_COUNT][768];
extern const VKRoomLink vk_stage18_raw_links[VK_STAGE18_ROOM_COUNT];
extern const VKByte vk_stage18_raw_spawn_flags[VK_STAGE18_ROOM_COUNT];
#define VK_STAGE18_DOOR_RAW_FLAGS 136
#define VK_STAGE18_DOOR_ROOM 8
#define VK_STAGE18_DOOR_X 12
#define VK_STAGE18_DOOR_Y 128
#endif
