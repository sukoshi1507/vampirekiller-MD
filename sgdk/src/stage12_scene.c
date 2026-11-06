#include "stage12_scene.h"
#include "generated/stage12_gameplay_data.h"

void vk_stage12_scene_reset(VKStage12Scene *scene)
{
    unsigned room;
    if (!scene) return;
    for (room = 0; room < VK_STAGE12_SCENE_ROOMS; room++) {
        scene->candle_removed[room] = 0;
        scene->fixed_taken[room] = 0;
    }
}

int vk_stage12_scene_reset_room(VKStage12Scene *scene, unsigned room)
{
    if (!scene || room >= VK_STAGE12_SCENE_ROOMS) return 0;
    scene->candle_removed[room] = 0;
    scene->fixed_taken[room] = 0;
    return 1;
}

unsigned vk_stage12_scene_candle_count(unsigned room)
{
    if (room >= VK_STAGE12_SCENE_ROOMS) return 0;
    return (unsigned)(vk_stage12_candle_first[room+1] - vk_stage12_candle_first[room]);
}

unsigned vk_stage12_scene_fixed_count(unsigned room)
{
    if (room >= VK_STAGE12_SCENE_ROOMS) return 0;
    return (unsigned)(vk_stage12_fixed_object_first[room+1] - vk_stage12_fixed_object_first[room]);
}

const VKCandleDef *vk_stage12_scene_candle(unsigned room, unsigned index)
{
    if (index >= vk_stage12_scene_candle_count(room)) return 0;
    return &vk_stage12_candle_defs[vk_stage12_candle_first[room]+index];
}

const VKCandleDef *vk_stage12_scene_fixed(unsigned room, unsigned index)
{
    if (index >= vk_stage12_scene_fixed_count(room)) return 0;
    return &vk_stage12_fixed_object_defs[vk_stage12_fixed_object_first[room]+index];
}

const VKByte *vk_stage12_scene_collision(unsigned room)
{
    return room < VK_STAGE12_SCENE_ROOMS ? vk_stage12_collision_tiles((VKByte)room) : 0;
}

const VKRoomLink *vk_stage12_scene_link(unsigned room)
{
    return room < VK_STAGE12_SCENE_ROOMS ? &vk_stage12_raw_links[room] : 0;
}

int vk_stage12_scene_candle_present(const VKStage12Scene *scene, unsigned room, unsigned index)
{
    if (!scene || index >= vk_stage12_scene_candle_count(room)) return 0;
    return !(scene->candle_removed[room] & (1u << index));
}

int vk_stage12_scene_fixed_present(const VKStage12Scene *scene, unsigned room, unsigned index)
{
    if (!scene || index >= vk_stage12_scene_fixed_count(room)) return 0;
    return !(scene->fixed_taken[room] & (1u << index));
}

int vk_stage12_scene_mark_candle_removed(VKStage12Scene *scene, unsigned room, unsigned index)
{
    if (!scene || index >= vk_stage12_scene_candle_count(room)) return 0;
    scene->candle_removed[room] |= (VKByte)(1u << index);
    return 1;
}

int vk_stage12_scene_mark_fixed_taken(VKStage12Scene *scene, unsigned room, unsigned index)
{
    if (!scene || index >= vk_stage12_scene_fixed_count(room)) return 0;
    scene->fixed_taken[room] |= (VKByte)(1u << index);
    return 1;
}
