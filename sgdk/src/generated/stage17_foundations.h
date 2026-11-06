/* ROM MSX2 raw stage 17 static data. NOT a playable stage. */
#ifndef VK_STAGE17_FOUNDATIONS_H
#define VK_STAGE17_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE17_ROOM_COUNT 12
extern const VKByte vk_stage17_tiles[VK_STAGE17_ROOM_COUNT][768];
extern const VKRoomLink vk_stage17_raw_links[VK_STAGE17_ROOM_COUNT];
extern const VKByte vk_stage17_raw_spawn_flags[VK_STAGE17_ROOM_COUNT];
#define VK_STAGE17_DOOR_RAW_FLAGS 139
#define VK_STAGE17_DOOR_ROOM 11
#define VK_STAGE17_DOOR_X 12
#define VK_STAGE17_DOOR_Y 64
#endif
