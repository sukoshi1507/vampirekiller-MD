/* Stage 12 raw ROM-derived data only: NOT a gameplay/4F83 verification. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage12_foundations.h"

int main(void)
{
    static const unsigned char secret[10][4] = {
      {0,3,88,168}, {1,4,184,136}, {2,11,152,168},
      {3,0,88,168}, {4,1,184,136}, {5,8,88,168},
      {7,10,56,168}, {8,5,88,168}, {10,7,56,168}, {11,2,152,168}
    };
    unsigned int i, j;
    assert(sizeof vk_stage12_tiles == 12u * 768u);
    assert(sizeof vk_stage12_raw_links / sizeof vk_stage12_raw_links[0] == 12);
    for (i=0;i<VK_STAGE12_ROOM_COUNT;++i) {
       const VKRoomLink *l=&vk_stage12_raw_links[i];
       assert(l->up==VK_ROOM_NONE && l->down==VK_ROOM_NONE);
       assert(l->left == VK_ROOM_NONE || l->left < VK_STAGE12_ROOM_COUNT);
       assert(l->right == VK_ROOM_NONE || l->right < VK_STAGE12_ROOM_COUNT);
       for (j=0;j<768;++j) assert(vk_stage12_tiles[i][j] < 192);
    }
    /* ROM bank-14 85A6 table flags, pointer at 1C5BE / CPU 861E. */
    for (i=0;i<VK_STAGE12_ROOM_COUNT;++i)
        assert(vk_stage12_raw_spawn_flags[i]==0); /* fixed enemies NOT implied absent */
    assert(vk_stage12_raw_links[5].right==6);
    assert(vk_stage12_raw_links[6].left==VK_ROOM_NONE); /* one-way, per ROM */
    assert(vk_stage12_raw_links[0].left==2 && vk_stage12_raw_links[0].right==1);
    assert(vk_stage12_raw_links[9].left==11 && vk_stage12_raw_links[9].right==10);
    for (i=0;i<VK_STAGE12_PASSAGE_COUNT;++i) {
       const VKStage12Passage *p=&vk_stage12_raw_passages[i];
       assert(p->source==secret[i][0] && p->destination==secret[i][1]);
       assert(p->x==secret[i][2] && p->y==secret[i][3]);
    }
    puts("Stage 12 foundation data: 12 maps, 12 links, one-way 5->6, 10 secrets: PASS");
    return 0;
}
