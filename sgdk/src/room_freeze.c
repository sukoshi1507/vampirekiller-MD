#include "room_freeze.h"

/* openMSX traces T1 and T2 (scratchpad/transition, research/room-links.md section 9):
 * frames with C000 = 9 and frames from the C000 = 9 start to the display off, for
 * each arrival room. Both depend on the arrival room only (not on the room left or
 * the direction); a run is sometimes one frame longer (hall rooms 0, 5 and 6). */
static const VKRoomFreezeTiming courtyard_timing[3] = { {18, 1}, {18, 1}, {18, 1} };
static const VKRoomFreezeTiming hall_timing[8] = {
    {19, 2}, {20, 2}, {22, 4}, {22, 4}, {21, 3}, {19, 2}, {19, 2}, {23, 4}
};
static const VKRoomFreezeTiming stage2_timing[6] = {
    {23, 3}, {24, 3}, {20, 2}, {21, 3}, {21, 3}, {21, 3}
};
/* PROVISIONAL: rooms without a trace (stages 3 and beyond are not playable). */
static const VKRoomFreezeTiming default_timing = {20, 2};

VKRoomFreezeTiming vk_room_freeze_timing(VKFreezeByte stage, VKFreezeByte room)
{
    if (stage == 0 && room < 3) return courtyard_timing[room];
    if (stage == 1 && room < 8) return hall_timing[room];
    if (stage == 2 && room < 6) return stage2_timing[room];
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
