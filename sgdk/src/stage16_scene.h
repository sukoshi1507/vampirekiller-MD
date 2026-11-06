/* Independent room-state facade for stage 16 only; not SGDK progression. */
#ifndef VK_STAGE16_SCENE_H
#define VK_STAGE16_SCENE_H
#include "generated/stage16_foundations.h"
#include "generated/stage16_objects.h"
typedef struct { VKByte candles[VK_STAGE16_ROOMS]; VKByte fixed[VK_STAGE16_ROOMS]; } VKStage16Scene;
void vk_stage16_scene_reset(VKStage16Scene *s);
int vk_stage16_scene_reset_room(VKStage16Scene *s, unsigned room);
unsigned vk_stage16_scene_candle_count(unsigned room);
unsigned vk_stage16_scene_fixed_count(unsigned room);
unsigned vk_stage16_scene_white_key_count(unsigned room);
unsigned vk_stage16_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage16_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage16_scene_fixed(unsigned room, unsigned index);
const VKCandleDef *vk_stage16_scene_white_key(unsigned room, unsigned index);
const VKStage16RawSlot *vk_stage16_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage16_scene_map(unsigned room);
const VKRoomLink *vk_stage16_scene_link(unsigned room);
int vk_stage16_scene_present(const VKStage16Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage16_scene_mark(VKStage16Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
