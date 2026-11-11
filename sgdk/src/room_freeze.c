#include "room_freeze.h"

/* openMSX measure of every arrival room (research/room-freeze-rooms.json, made by
 * tools/run_room_freeze_trace.py; research/room-links.md section 9 for the courtyard,
 * the hall and stage 2, traces T1 and T2): frames with C000 = 9 and frames from the
 * C000 = 9 start to the display off. Both depend on the arrival room only (not on the
 * room left or the direction); a run is sometimes one frame longer or shorter, the
 * table keeps the most frequent value. */
#include "generated/room_freeze_table.h"

/* Rooms that do not exist (no link leads there): the hall room 1 figures. */
static const VKRoomFreezeTiming default_timing = {20, 2};

VKRoomFreezeTiming vk_room_freeze_timing(VKFreezeByte stage, VKFreezeByte room)
{
    if (stage < VK_FREEZE_STAGES && room < VK_FREEZE_ROOMS && vk_room_freeze_table[stage][room].frames)
        return vk_room_freeze_table[stage][room];
    return default_timing;
}

void vk_room_freeze_begin(VKRoomFreeze *f, VKFreezeByte stage, VKFreezeByte room)
{
    const VKRoomFreezeTiming t = vk_room_freeze_timing(stage, room);
    f->frames = t.frames;
    f->dark = t.dark;
    f->left = t.frames;
}

void vk_room_freeze_clear(VKRoomFreeze *f)
{
    f->left = 0;
}

int vk_room_freeze_active(const VKRoomFreeze *f)
{
    return f->left != 0;
}

VKFreezeStep vk_room_freeze_tick(VKRoomFreeze *f)
{
    VKFreezeByte k;
    if (!f->left) return VK_FREEZE_OFF;
    k = (VKFreezeByte)(f->frames - f->left);
    f->left--;
    /* Display on at the end of 62F5, in the last C000 = 9 frame (47C7). */
    if (!f->left) return VK_FREEZE_SHOW;
    /* Old room without sprites (SAT cleared by 63C5) until the display goes off. */
    return k == f->dark ? VK_FREEZE_DARK : VK_FREEZE_HOLD;
}
