/* Exact ROM static tables only. Does NOT test gameplay/boss/collisions. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage17_foundations.h"
#include "generated/stage18_foundations.h"
static void test_17(void) {
    unsigned int r,i;
    static const VKByte expected[] = {0,0,0,128,0,128,0,64,0,0,64,0};
    assert(sizeof vk_stage17_tiles == 12u*768u);
    assert(sizeof vk_stage17_raw_spawn_flags == 12u);
    assert(sizeof vk_stage17_raw_links/sizeof vk_stage17_raw_links[0] == 12u);
    for (r=0;r<12;r++) {
        const VKRoomLink *l=&vk_stage17_raw_links[r];
        assert(l->up==VK_ROOM_NONE || l->up<12);
        assert(l->down==VK_ROOM_NONE || l->down<12);
        assert(l->left==VK_ROOM_NONE || l->left<12);
        assert(l->right==VK_ROOM_NONE || l->right<12);
        assert(vk_stage17_raw_spawn_flags[r]==expected[r]);
        for (i=0;i<768;i++) assert(vk_stage17_tiles[r][i]<192);
    }
    assert(vk_stage17_raw_links[10].left==11);
    assert(vk_stage17_raw_links[11].right==VK_ROOM_NONE);
    assert(VK_STAGE17_DOOR_RAW_FLAGS==0x8B);
    assert(VK_STAGE17_DOOR_ROOM==11 && VK_STAGE17_DOOR_X==12 && VK_STAGE17_DOOR_Y==64);
}
static void test_18(void) {
    unsigned int r,i,unresolved=0;
    static const VKByte expected[] = {0,128,0,0,0,0,0,128,128,0};
    assert(sizeof vk_stage18_tiles == 10u*768u);
    assert(sizeof vk_stage18_raw_spawn_flags == 10u);
    assert(sizeof vk_stage18_raw_links/sizeof vk_stage18_raw_links[0] == 10u);
    for (r=0;r<10;r++) {
        const VKRoomLink *l=&vk_stage18_raw_links[r];
        assert(l->up==VK_ROOM_NONE || l->up<10);
        assert(l->down==VK_ROOM_NONE || l->down<10);
        assert(l->left==VK_ROOM_NONE || l->left<10);
        assert(l->right==VK_ROOM_NONE || l->right<10);
        assert(vk_stage18_raw_spawn_flags[r]==expected[r]);
        for (i=0;i<768;i++) {
            if (vk_stage18_tiles[r][i]>=192) {
                assert(r==9); /* ROM contains these IDs; current renderer cannot display them. */
                unresolved++;
            }
        }
    }
    assert(unresolved>0);
    assert(vk_stage18_raw_links[8].left==9);
    assert(vk_stage18_raw_links[9].right==VK_ROOM_NONE);
    assert(VK_STAGE18_DOOR_RAW_FLAGS==0x88);
    assert(VK_STAGE18_DOOR_ROOM==8 && VK_STAGE18_DOOR_X==12 && VK_STAGE18_DOOR_Y==128);
}
int main(void) {
    test_17(); test_18();
    puts("stages 17 and 18 static ROM tables: PASS (NOT gameplay, boss or ending)");
    return 0;
}
