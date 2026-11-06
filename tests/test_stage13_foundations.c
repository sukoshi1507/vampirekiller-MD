/* ROM-derived stage 13 STATIC resources. Not a gameplay test. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage13_foundations.h"

int main(void)
{
    static const unsigned char expected_flags[12] = {0,0,128,0,0,0,128,128,128,128,128,128};
    unsigned int r,j;
    assert(sizeof vk_stage13_tiles == 12u*768u);
    assert(sizeof vk_stage13_raw_links / sizeof vk_stage13_raw_links[0] == 12);
    assert(sizeof vk_stage13_raw_spawn_flags == 12);
    for (r=0;r<12;r++) {
        const VKRoomLink *l=&vk_stage13_raw_links[r];
        assert(l->up==VK_ROOM_NONE || l->up<12);
        assert(l->down==VK_ROOM_NONE || l->down<12);
        assert(l->left==VK_ROOM_NONE || l->left<12);
        assert(l->right==VK_ROOM_NONE || l->right<12);
        assert(vk_stage13_raw_spawn_flags[r]==expected_flags[r]);
        for(j=0;j<768;j++) assert(vk_stage13_tiles[r][j]<192);
    }
    /* Preserve asymmetry in original byte table instead of normalising it. */
    assert(vk_stage13_raw_links[11].left==9);
    assert(vk_stage13_raw_links[9].right==10);
    assert(vk_stage13_raw_links[0].up==3);
    assert(vk_stage13_raw_links[2].down==7);
    assert(vk_stage13_raw_links[6].up==11);
    puts("stage13 static ROM tables: PASS (not gameplay validation)");
    return 0;
}
