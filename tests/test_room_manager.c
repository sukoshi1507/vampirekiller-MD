#include <assert.h>
#include "room_manager.h"

/* Hall left/right links proven by openMSX traces (research/room-links.md 3.2). */
static void check_edge(VKByte from, int direction, int expect_ok, VKByte expect_room)
{
    VKPlayer p = {0}; VKByte room = from;
    p.x = direction > 0 ? 248 : 8; p.y = 192; p.mode = 0;
    assert(vk_room_try_edge(&room, &p, direction) == expect_ok);
    assert(room == expect_room);
    if (expect_ok)
        assert(p.x == (direction > 0 ? 10 : 246) && p.y == 192);
}

int main(void)
{
    VKPlayer p = {0}; VKByte room = 0;
    /* Entry x is 10 from the left, 246 from the right; height is kept. */
    p.x = 248; p.y = 176; p.mode = 1; p.phase = 7; p.facing = 0;
    assert(vk_room_try_edge(&room,&p,1));
    assert(room == 1 && p.x == 10 && p.y == 176 && p.mode == 1 && p.phase == 7);
    assert(!vk_stage1_links[1].verified_collision);
    assert(!vk_room_try_edge(&room,&p,1));

    /* Full left/right table. NONE means no exit. */
    check_edge(0, 1, 1, 1);    check_edge(0, -1, 1, 3);
    check_edge(1, 1, 1, 2);    check_edge(1, -1, 1, 0);
    check_edge(2, 1, 1, 3);    check_edge(2, -1, 1, 1);
    check_edge(3, 1, 1, 0);    check_edge(3, -1, 1, 2);
    check_edge(4, 1, 1, 5);    check_edge(4, -1, 0, 4);
    check_edge(5, 1, 1, 6);    check_edge(5, -1, 1, 4);
    check_edge(6, 1, 1, 7);    check_edge(6, -1, 1, 5);
    check_edge(7, 1, 0, 7);    check_edge(7, -1, 1, 6);

    /* Edge needs the boundary: x=200 does not leave the room. */
    room = 1; p.x = 200; p.on_stairs = 0;
    assert(!vk_room_try_edge(&room,&p,1) && room == 1);
    p.x = 248; p.on_stairs = 1;
    assert(!vk_room_try_edge(&room,&p,1) && room == 1);
    return 0;
}
