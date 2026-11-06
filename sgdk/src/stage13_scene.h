/* Independent room-state facade for stage 13 only; not SGDK progression. */
#ifndef VK_STAGE13_SCENE_H
#define VK_STAGE13_SCENE_H
#include "generated/stage13_foundations.h"
#include "generated/stage13_objects.h"
typedef struct { VKByte candles[VK_STAGE13_ROOMS]; VKByte fixed[VK_STAGE13_ROOMS]; } VKStage13Scene;
void vk_stage13_scene_reset(VKStage13Scene *s);
int vk_stage13_scene_reset_room(VKStage13Scene *s, unsigned room);
unsigned vk_stage13_scene_candle_count(unsigned room);
unsigned vk_stage13_scene_fixed_count(unsigned room);
unsigned vk_stage13_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage13_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage13_scene_fixed(unsigned room, unsigned index);
const VKStage13RawSlot *vk_stage13_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage13_scene_map(unsigned room);
const VKRoomLink *vk_stage13_scene_link(unsigned room);
int vk_stage13_scene_present(const VKStage13Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage13_scene_mark(VKStage13Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
