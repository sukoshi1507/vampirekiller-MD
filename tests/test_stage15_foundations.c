/* Static ROM tables only; this is NOT a stage 15 gameplay test. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage15_foundations.h"
int main(void) {
    static const VKByte flags[10] = {0,128,128,128,0,0,0,128,0,0};
    unsigned int r,t;
    assert(sizeof vk_stage15_tiles == 10u*768u);
    assert(sizeof vk_stage15_raw_spawn_flags == 10u);
    assert(sizeof vk_stage15_raw_links / sizeof vk_stage15_raw_links[0] == 10u);
    for(r=0;r<10;r++) {
        const VKRoomLink *l=&vk_stage15_raw_links[r];
        assert(l->up==VK_ROOM_NONE || l->up<10);
        assert(l->down==VK_ROOM_NONE || l->down<10);
        assert(l->left==VK_ROOM_NONE || l->left<10);
        assert(l->right==VK_ROOM_NONE || l->right<10);
        assert(vk_stage15_raw_spawn_flags[r]==flags[r]);
        for(t=0;t<768;t++) assert(vk_stage15_tiles[r][t]<192);
    }
    assert(vk_stage15_raw_links[5].down==3 && vk_stage15_raw_links[3].up==VK_ROOM_NONE);
    assert(vk_stage15_raw_links[8].down==5 && vk_stage15_raw_links[5].up==VK_ROOM_NONE);
    assert(vk_stage15_raw_links[8].left==9 && vk_stage15_raw_links[9].right==VK_ROOM_NONE);
    assert(vk_stage15_raw_links[9].up==VK_ROOM_NONE && vk_stage15_raw_links[9].down==VK_ROOM_NONE);
    assert(VK_STAGE15_DOOR_RAW_FLAGS==136);
    assert(VK_STAGE15_DOOR_ROOM==8 && VK_STAGE15_DOOR_X==12 && VK_STAGE15_DOOR_Y==128);
    puts("stage15 static ROM tables: PASS (not a gameplay validation)");
    return 0;
}
