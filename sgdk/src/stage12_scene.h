/* Stage 12 isolated room-state adapter.
 * Sources: 4F83 tile decoder and archived 5A49/5B1B Z80 object snapshots.
 * NO progression hookup, enemy creation or unverified item effects. */
#ifndef VK_STAGE12_SCENE_H
#define VK_STAGE12_SCENE_H
#include "generated/stage12_foundations.h"
#include "generated/stage12_candles.h"
#include "generated/stage12_fixed_objects.h"
#define VK_STAGE12_SCENE_ROOMS 12
#define VK_STAGE12_SCENE_MAX_OBJECTS 8
/* These state masks record external gameplay results; they do not infer their triggers. */
typedef struct {
    VKByte candle_removed[VK_STAGE12_SCENE_ROOMS];
    VKByte fixed_taken[VK_STAGE12_SCENE_ROOMS];
} VKStage12Scene;
void vk_stage12_scene_reset(VKStage12Scene *scene);
/* Reset a single room; returns 0 on invalid room. */
int vk_stage12_scene_reset_room(VKStage12Scene *scene, unsigned room);
unsigned vk_stage12_scene_candle_count(unsigned room);
unsigned vk_stage12_scene_fixed_count(unsigned room);
const VKCandleDef *vk_stage12_scene_candle(unsigned room, unsigned index);
const VKCandleDef *vk_stage12_scene_fixed(unsigned room, unsigned index);
/* Read-only collision tilemap, 768 bytes or NULL. */
const VKByte *vk_stage12_scene_collision(unsigned room);
/* Read-only ROM room link, NULL if invalid. */
const VKRoomLink *vk_stage12_scene_link(unsigned room);
/* Query or record externally validated object removal; indices are local to the room.
 * A mark is idempotent and does not itself trigger any pickup/attack behavior. */
int vk_stage12_scene_candle_present(const VKStage12Scene *scene, unsigned room, unsigned index);
int vk_stage12_scene_fixed_present(const VKStage12Scene *scene, unsigned room, unsigned index);
int vk_stage12_scene_mark_candle_removed(VKStage12Scene *scene, unsigned room, unsigned index);
int vk_stage12_scene_mark_fixed_taken(VKStage12Scene *scene, unsigned room, unsigned index);
#endif
