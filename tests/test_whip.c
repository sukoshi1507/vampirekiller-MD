#include <assert.h>
#include "whip.h"
int main(void)
{
    VKPlayer p = {0}; VKWhip w; int l,t,r,b; int i;
    p.x=80; p.y=160;
    vk_whip_reset(&w);
    assert(!vk_whip_active(&w));
    vk_whip_tick(&w,&p,1);
    assert(w.timer==14 && !vk_whip_active(&w));
    assert(vk_whip_arm_frame(&w)==1);
    assert(vk_whip_phase(&w)==0 && vk_whip_segments(&w)==0);
    for(i=0;i<4;i++)vk_whip_tick(&w,&p,0);
    assert(vk_whip_active(&w));
    assert(vk_whip_phase(&w)==1 && vk_whip_segments(&w)==2);
    assert(vk_whip_arm_frame(&w)==2);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(l>=p.x && r>l && t<b);
    for(i=0;i<3;i++) vk_whip_tick(&w,&p,0);
    assert(vk_whip_phase(&w)==2 && vk_whip_segments(&w)==3);
    assert(vk_whip_arm_frame(&w)==3);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(r-l==24);
    while(w.timer) vk_whip_tick(&w,&p,0);
    assert(vk_whip_segments(&w)==0);
    assert(vk_whip_arm_frame(&w)==0);
    p.facing=1;vk_whip_tick(&w,&p,1);
    for(i=0;i<4;i++)vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(r<=p.x && l<r);
    { int ax,ay; vk_whip_arm_position(&w,&ax,&ay); assert(ax < p.x && ay < p.y); }
    return 0;
}
