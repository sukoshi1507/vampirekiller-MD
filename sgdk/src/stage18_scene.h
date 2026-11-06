/* Independent room-state facade for stage 18 only; not SGDK progression. */
#ifndef VK_STAGE18_SCENE_H
#define VK_STAGE18_SCENE_H
#include "generated/stage18_foundations.h"
#include "generated/stage18_objects.h"
typedef struct { VKByte candles[VK_STAGE18_ROOMS]; VKByte fixed[VK_STAGE18_ROOMS]; } VKStage18Scene;
void vk_stage18_scene_reset(VKStage18Scene *s);
int vk_stage18_scene_reset_room(VKStage18Scene *s, unsigned room);
unsigned vk_stage18_scene_candle_count(unsigned room);
unsigned vk_stage18_scene_fixed_count(unsigned room);
unsigned vk_stage18_scene_white_key_count(unsigned room);
unsigned vk_stage18_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage18_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage18_scene_fixed(unsigned room, unsigned index);
const VKCandleDef *vk_stage18_scene_white_key(unsigned room, unsigned index);
const VKStage18RawSlot *vk_stage18_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage18_scene_map(unsigned room);
const VKRoomLink *vk_stage18_scene_link(unsigned room);
int vk_stage18_scene_present(const VKStage18Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage18_scene_mark(VKStage18Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
