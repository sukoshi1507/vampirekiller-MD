#include "room_manager.h"
#include "generated/stage2_maps.h"
#include "generated/stage3_maps.h"
#include "generated/stage4_maps.h"
#include "generated/stage5_maps.h"
#include "generated/stage6_maps.h"
#include "generated/stage7_maps.h"
#include "generated/stage8_maps.h"
#include "generated/stage9_maps.h"
#include "generated/stage10_maps.h"
#include "generated/stage11_maps.h"
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
    {VK_ROOM_NONE,3,6,VK_ROOM_NONE,0},
};
const VKRoomLink *vk_stage_links(VKByte stage)
{
    return stage == 1 ? vk_stage1_links : stage == 2 ? vk_stage2_links : stage == 3 ? vk_stage3_links : stage == 4 ? vk_stage4_links : stage == 5 ? vk_stage5_links : stage == 6 ? vk_stage6_links : stage == 7 ? vk_stage7_links : stage == 8 ? vk_stage8_links : stage == 9 ? vk_stage9_links : stage == 10 ? vk_stage10_links : stage == 11 ? vk_stage11_links : 0;
}

VKByte vk_stage_room_count(VKByte stage)
{
    return stage == 1 ? VK_STAGE1_ROOM_COUNT : stage == 2 ? VK_STAGE2_ROOM_COUNT : stage == 3 ? VK_STAGE3_ROOM_COUNT : stage == 4 ? VK_STAGE4_ROOM_COUNT : stage == 5 ? VK_STAGE5_ROOM_COUNT : stage == 6 ? VK_STAGE6_ROOM_COUNT : stage == 7 ? VK_STAGE7_ROOM_COUNT : stage == 8 ? VK_STAGE8_ROOM_COUNT : stage == 9 ? VK_STAGE9_ROOM_COUNT : stage == 10 ? VK_STAGE10_ROOM_COUNT : stage == 11 ? VK_STAGE11_ROOM_COUNT : 0;
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
