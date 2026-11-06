/* Stage 12 staging data only; no SGDK runtime hookup. */
#ifndef VK_STAGE12_GAMEPLAY_DATA_H
#define VK_STAGE12_GAMEPLAY_DATA_H
#include "../room_manager.h"
#define VK_STAGE12_CANDLE_POSITION_COUNT 30
#define VK_STAGE12_OTHER_OBJECT_POSITION_COUNT 5
/* X,Y position; no unproven type, drop or behavior is assigned. */
typedef struct { VKByte x, y; } VKStage12Position;
extern const VKStage12Position vk_stage12_candle_positions[VK_STAGE12_CANDLE_POSITION_COUNT];
extern const VKStage12Position vk_stage12_other_object_positions[VK_STAGE12_OTHER_OBJECT_POSITION_COUNT];
extern const VKByte vk_stage12_candle_start[13];
extern const VKByte vk_stage12_other_object_start[13];
/* Tile bytes are the validated normal 4F83 decode; runtime collision semantics remain to verify. */
const VKByte *vk_stage12_collision_tiles(VKByte room);
#endif
