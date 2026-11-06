/* Static ROM tables only; this is NOT a stage 16 gameplay test. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage16_foundations.h"
int main(void) {
    static const VKByte flags[10] = {0,0,0,0,0,0,0,0,0,0};
    unsigned int r,t;
    assert(sizeof vk_stage16_tiles == 10u*768u);
    assert(sizeof vk_stage16_raw_spawn_flags == 10u);
    assert(sizeof vk_stage16_raw_links / sizeof vk_stage16_raw_links[0] == 10u);
    for(r=0;r<10;r++) {
        const VKRoomLink *l=&vk_stage16_raw_links[r];
        assert(l->up==VK_ROOM_NONE || l->up<10);
        assert(l->down==VK_ROOM_NONE || l->down<10);
        assert(l->left==VK_ROOM_NONE || l->left<10);
        assert(l->right==VK_ROOM_NONE || l->right<10);
        assert(vk_stage16_raw_spawn_flags[r]==flags[r]);
        for(t=0;t<768;t++) assert(vk_stage16_tiles[r][t]<192);
    }
    assert(vk_stage16_raw_links[1].down==7 && vk_stage16_raw_links[7].up==VK_ROOM_NONE);
    assert(vk_stage16_raw_links[2].down==8 && vk_stage16_raw_links[8].up==VK_ROOM_NONE);
    assert(vk_stage16_raw_links[3].down==9 && vk_stage16_raw_links[9].up==VK_ROOM_NONE);
    assert(VK_STAGE16_DOOR_RAW_FLAGS==133);
    assert(VK_STAGE16_DOOR_ROOM==5 && VK_STAGE16_DOOR_X==12 && VK_STAGE16_DOOR_Y==64);
    puts("stage16 static ROM tables: PASS (not a gameplay validation)");
    return 0;
}
