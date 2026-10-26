#include "room_manager.h"
/* Sequential edges are deliberately a PLAYTEST graph, NOT a claim about
 * the original MSX2 castle topology. Real doors/stairs await Z80 mapping. */
const VKRoomLink vk_stage1_links[VK_STAGE1_ROOM_COUNT] = {
    {VK_ROOM_NONE,1,1},{0,2,0},{1,3,0},{2,4,0},
    {3,5,0},{4,6,0},{5,7,0},{6,VK_ROOM_NONE,0}
};
int vk_room_try_edge(VKByte *room, VKPlayer *player, int direction)
{
    VKByte target;
    if (!room || !player || *room >= VK_STAGE1_ROOM_COUNT || !direction || player->on_stairs)
        return 0;
    if (direction > 0)
    {
        if (player->x < 248) return 0;
        target = vk_stage1_links[*room].right;
    }
    else
    {
        if (player->x > 8) return 0;
        target = vk_stage1_links[*room].left;
    }
    if (target == VK_ROOM_NONE) return 0;
    *room = target;
    player->x = direction > 0 ? 10 : 246;
    return 1;
}
