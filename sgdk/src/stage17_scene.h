/* Independent room-state facade for stage 17 only; not SGDK progression. */
#ifndef VK_STAGE17_SCENE_H
#define VK_STAGE17_SCENE_H
#include "generated/stage17_foundations.h"
#include "generated/stage17_objects.h"
typedef struct { VKByte candles[VK_STAGE17_ROOMS]; VKByte fixed[VK_STAGE17_ROOMS]; } VKStage17Scene;
void vk_stage17_scene_reset(VKStage17Scene *s);
int vk_stage17_scene_reset_room(VKStage17Scene *s, unsigned room);
unsigned vk_stage17_scene_candle_count(unsigned room);
unsigned vk_stage17_scene_fixed_count(unsigned room);
unsigned vk_stage17_scene_white_key_count(unsigned room);
unsigned vk_stage17_scene_raw_count(unsigned room);
const VKCandleDef *vk_stage17_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage17_scene_fixed(unsigned room, unsigned index);
const VKCandleDef *vk_stage17_scene_white_key(unsigned room, unsigned index);
const VKStage17RawSlot *vk_stage17_scene_raw(unsigned room, unsigned index);
const VKByte *vk_stage17_scene_map(unsigned room);
const VKRoomLink *vk_stage17_scene_link(unsigned room);
int vk_stage17_scene_present(const VKStage17Scene *s, unsigned room, unsigned index, int is_fixed);
int vk_stage17_scene_mark(VKStage17Scene *s, unsigned room, unsigned index, int is_fixed);
#endif
