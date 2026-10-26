#include <assert.h>
#include "room_manager.h"
int main(void)
{
    VKPlayer p = {0}; VKByte room = 0;
    p.x = 248; p.y = 176; p.mode = 1; p.phase = 7; p.facing = 0;
    assert(vk_room_try_edge(&room,&p,1));
    assert(room == 1 && p.x == 10 && p.y == 176 && p.mode == 1 && p.phase == 7);
    assert(!vk_stage1_links[1].verified_collision);
    assert(!vk_room_try_edge(&room,&p,1));
    p.x=8; p.facing=1;
    assert(vk_room_try_edge(&room,&p,-1));
    assert(room == 0 && p.x == 246 && p.facing == 1);
    assert(!vk_room_try_edge(&room,&p,1));
    room=7;p.x=248;
    assert(!vk_room_try_edge(&room,&p,1));
    room=1;p.x=248;p.on_stairs=1;
    assert(!vk_room_try_edge(&room,&p,1));
    return 0;
}
