#include "stage12_object_view.h"
#include <stddef.h>
unsigned vk_stage12_object_count(unsigned room) {
    if (room >= VK_STAGE12_RAW_ROOM_COUNT) return 0;
    return (unsigned)(vk_stage12_raw_object_start[room+1] - vk_stage12_raw_object_start[room]);
}
const VKStage12RawObject *vk_stage12_object_at(unsigned room, unsigned index) {
    unsigned start, end;
    if (room >= VK_STAGE12_RAW_ROOM_COUNT) return NULL;
    start = vk_stage12_raw_object_start[room];
    end = vk_stage12_raw_object_start[room+1];
    if (index >= end-start) return NULL;
    return &vk_stage12_raw_objects[start+index];
}
