/* Independent room-state facade for stage 14 only; not SGDK progression. */
#ifndef VK_STAGE14_SCENE_H
#define VK_STAGE14_SCENE_H
#include "generated/stage14_foundations.h"
#include "generated/stage14_objects.h"
typedef struct { VKByte candles[VK_STAGE14_ROOMS]; VKByte fixed[VK_STAGE14_ROOMS]; } VKStage14Scene;
void vk_stage14_scene_reset(VKStage14Scene *s);
int vk_stage14_scene_reset_room(VKStage14Scene *s, unsigned room);
unsigned vk_stage14_scene_candle_count(unsigned room);
unsigned vk_stage14_scene_fixed_count(unsigned room);
unsigned vk_stage14_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage14_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage14_scene_fixed(unsigned room, unsigned index);
const VKStage14RawSlot *vk_stage14_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage14_scene_map(unsigned room);
const VKRoomLink *vk_stage14_scene_link(unsigned room);
int vk_stage14_scene_present(const VKStage14Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage14_scene_mark(VKStage14Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
