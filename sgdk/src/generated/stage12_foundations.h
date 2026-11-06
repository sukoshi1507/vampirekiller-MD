/* Raw ROM data staging; not an enabled gameplay stage. */
#ifndef VK_STAGE12_FOUNDATIONS_H
#define VK_STAGE12_FOUNDATIONS_H
#include "../room_manager.h"
#define VK_STAGE12_ROOM_COUNT 12
#define VK_STAGE12_PASSAGE_COUNT 10
typedef struct { VKByte source, destination, x, y; } VKStage12Passage;
extern const VKByte vk_stage12_tiles[VK_STAGE12_ROOM_COUNT][768];
extern const VKRoomLink vk_stage12_raw_links[VK_STAGE12_ROOM_COUNT];
extern const VKStage12Passage vk_stage12_raw_passages[VK_STAGE12_PASSAGE_COUNT];
extern const VKByte vk_stage12_raw_spawn_flags[VK_STAGE12_ROOM_COUNT];
/* Raw ROM table lookup only. Does not execute the MSX2 secret-passage state machine. */
const VKStage12Passage *vk_stage12_find_raw_passage(VKByte source, VKByte x, VKByte y);
#endif
