#include <assert.h>
#include "targets.h"
int main(void)
{
    VKTarget t; VKWhip w; VKPlayer p = {0}; int i;
    p.x=150; p.y=160; p.facing=0;
    vk_target_init(&t,174,136,16,16);
    vk_whip_reset(&w);
    assert(!vk_target_tick(&t,&w));
    vk_whip_tick(&w,&p,1);
    assert(!vk_target_tick(&t,&w));
    for(i=0;i<3;i++) vk_whip_tick(&w,&p,0);
    assert(vk_target_tick(&t,&w));
    assert(!vk_target_tick(&t,&w));
    assert(!t.active);
    vk_target_init(&t,10,10,8,8);
    assert(!vk_target_tick(&t,&w));
    p.facing=1; vk_whip_reset(&w);
    vk_target_init(&t,120,136,16,16);
    vk_whip_tick(&w,&p,1);
    for(i=0;i<3;i++) vk_whip_tick(&w,&p,0);
    assert(vk_target_tick(&t,&w));
    vk_target_init(&t,124,120,16,8); /* no vertical overlap */
    assert(!vk_target_tick(&t,&w));
    /* Multi-target: one swing hits only the target in reach, once. */
    {
        VKTarget list[2]; VKWhip w2; VKPlayer p2 = {0}; int k;
        p2.x=150; p2.y=160; p2.facing=0;
        vk_target_init(&list[0],10,10,8,8);
        vk_target_init(&list[1],174,136,16,16);
        vk_whip_reset(&w2);
        vk_whip_tick(&w2,&p2,1);
        for(k=0;k<3;k++) vk_whip_tick(&w2,&p2,0);
        assert(vk_targets_tick(list,2,&w2)==1);
        assert(vk_targets_tick(list,2,&w2)==-1);
        assert(!list[1].active && list[0].active);
    }
    return 0;
}
