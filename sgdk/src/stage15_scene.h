/* Independent room-state facade for stage 15 only; not SGDK progression. */
#ifndef VK_STAGE15_SCENE_H
#define VK_STAGE15_SCENE_H
#include "generated/stage15_foundations.h"
#include "generated/stage15_objects.h"
typedef struct { VKByte candles[VK_STAGE15_ROOMS]; VKByte fixed[VK_STAGE15_ROOMS]; } VKStage15Scene;
void vk_stage15_scene_reset(VKStage15Scene *s);
int vk_stage15_scene_reset_room(VKStage15Scene *s, unsigned room);
unsigned vk_stage15_scene_candle_count(unsigned room);
unsigned vk_stage15_scene_fixed_count(unsigned room);
unsigned vk_stage15_scene_white_key_count(unsigned room);
unsigned vk_stage15_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage15_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage15_scene_fixed(unsigned room, unsigned index);
const VKCandleDef *vk_stage15_scene_white_key(unsigned room, unsigned index);
const VKStage15RawSlot *vk_stage15_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage15_scene_map(unsigned room);
const VKRoomLink *vk_stage15_scene_link(unsigned room);
int vk_stage15_scene_present(const VKStage15Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage15_scene_mark(VKStage15Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
