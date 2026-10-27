#include "room_manager.h"
#include "generated/stage2_maps.h"
/* Hall (stage 1) links decoded from the ROM link table by
 * tools/decode_room_links.py. Vertical links 1<->5 and 3<->7 are the table
 * entries (reciprocal); they are not yet verified by a trace in game. */
const VKRoomLink vk_stage1_links[VK_STAGE1_ROOM_COUNT] = {
    {VK_ROOM_NONE,VK_ROOM_NONE,3,1,0},
    {5,VK_ROOM_NONE,0,2,0},
    {VK_ROOM_NONE,VK_ROOM_NONE,1,3,0},
    {7,VK_ROOM_NONE,2,0,0},
    {VK_ROOM_NONE,VK_ROOM_NONE,VK_ROOM_NONE,5,0},
    {VK_ROOM_NONE,1,4,6,0},
    {VK_ROOM_NONE,VK_ROOM_NONE,5,7,0},
    {VK_ROOM_NONE,3,6,VK_ROOM_NONE,0}
};
const VKRoomLink *vk_stage_links(VKByte stage)
{
    return stage == 1 ? vk_stage1_links : stage == 2 ? vk_stage2_links : 0;
}

VKByte vk_stage_room_count(VKByte stage)
{
    return stage == 1 ? VK_STAGE1_ROOM_COUNT : stage == 2 ? VK_STAGE2_ROOM_COUNT : 0;
}

int vk_room_try_edge(VKByte stage, VKByte *room, VKPlayer *player, int direction)
{
    const VKRoomLink *links = vk_stage_links(stage);
    VKByte target;
    if (!room || !player || *room >= vk_stage_room_count(stage) || !direction || player->on_stairs)
        return 0;
    if (direction > 0)
    {
        if (player->x < 248) return 0;
        target = links[*room].right;
    }
    else
    {
        if (player->x > 8) return 0;
        target = links[*room].left;
    }
    if (target == VK_ROOM_NONE) return 0;
    *room = target;
    player->x = direction > 0 ? 10 : 246;
    return 1;
}
