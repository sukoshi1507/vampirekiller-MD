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
    for(i=0;i<4;i++) vk_whip_tick(&w,&p,0);
    assert(vk_target_tick(&t,&w));
    assert(!vk_target_tick(&t,&w));
    assert(!t.active);
    vk_target_init(&t,10,10,8,8);
    assert(!vk_target_tick(&t,&w));
    p.facing=1; vk_whip_reset(&w);
    vk_target_init(&t,120,136,16,16);
    vk_whip_tick(&w,&p,1);
    for(i=0;i<4;i++) vk_whip_tick(&w,&p,0);
    assert(vk_target_tick(&t,&w));
    vk_target_init(&t,124,120,16,8); /* no vertical overlap */
    assert(!vk_target_tick(&t,&w));
    return 0;
}
