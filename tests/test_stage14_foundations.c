/* Test only ROM raw static stage 14 data, NOT collision, enemy, or gameplay behavior. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage14_foundations.h"
int main(void) {
    unsigned int r,t;
    assert(sizeof vk_stage14_tiles == 8u*768u);
    assert(sizeof vk_stage14_raw_spawn_flags == 8u);
    assert(sizeof vk_stage14_raw_links / sizeof vk_stage14_raw_links[0] == 8u);
    for(r=0;r<8;r++) {
        const VKRoomLink *l=&vk_stage14_raw_links[r];
        assert(l->up==VK_ROOM_NONE || l->up<8);
        assert(l->down==VK_ROOM_NONE || l->down<8);
        assert(l->left==VK_ROOM_NONE || l->left<8);
        assert(l->right==VK_ROOM_NONE || l->right<8);
        assert(vk_stage14_raw_spawn_flags[r]==128);
        for(t=0;t<768;t++) assert(vk_stage14_tiles[r][t]<192);
    }
    assert(vk_stage14_raw_links[2].down==5);
    assert(vk_stage14_raw_links[5].up==VK_ROOM_NONE);
    assert(vk_stage14_raw_links[4].down==1);
    assert(vk_stage14_raw_links[1].up==VK_ROOM_NONE);
    assert(vk_stage14_raw_links[0].up==3);
    assert(vk_stage14_raw_links[7].right==6);
    assert(VK_STAGE14_DOOR_RAW_FLAGS==135);
    assert(VK_STAGE14_DOOR_ROOM==7);
    assert(VK_STAGE14_DOOR_X==12);
    assert(VK_STAGE14_DOOR_Y==128);
    puts("stage14 raw tables test: PASS (NOT gameplay validation)");
    return 0;
}
