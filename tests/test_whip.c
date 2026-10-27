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
    assert(vk_whip_visual_step(&w)==0);
    assert(vk_whip_phase(&w)==0 && vk_whip_segments(&w)==0);
    for(i=0;i<4;i++)vk_whip_tick(&w,&p,0);
    assert(vk_whip_active(&w));
    assert(vk_whip_phase(&w)==1 && vk_whip_segments(&w)==2);
    assert(vk_whip_arm_frame(&w)==1);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(l>=p.x && r>l && t<b);
    for(i=0;i<3;i++) vk_whip_tick(&w,&p,0);
    assert(vk_whip_phase(&w)==2 && vk_whip_segments(&w)==3);
    assert(vk_whip_arm_frame(&w)==1);
    assert(vk_whip_visual_step(&w)==2);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(r-l==24); /* Visible standing lash box: x+8..x+32. */
    while(w.timer) vk_whip_tick(&w,&p,0);
    assert(vk_whip_segments(&w)==0);
    assert(vk_whip_arm_frame(&w)==0);
    assert(vk_whip_visual_step(&w)==0);
    p.facing=1;vk_whip_tick(&w,&p,1);
    for(i=0;i<4;i++)vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(r<=p.x && l<r);
    { int ax,ay; vk_whip_arm_position(&w,&ax,&ay); assert(ax < p.x && ay < p.y); }
    /* Crouched attacks use a lower hit plane, locked at attack start. */
    vk_whip_reset(&w);
    p.facing=0; p.crouched=1;
    vk_whip_tick(&w,&p,1);
    assert(w.crouched==1);
    for(i=0;i<4;i++) vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(t==p.y-10 && b==p.y-7);
    p.crouched=0; /* mid-attack changes must not move the hitbox */
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(t==p.y-10 && b==p.y-7);
    /* Exhaustive attack timeline: all visual frame indices remain in their
     * generated-resource ranges, and active hitboxes have visible lash. */
    {
        static const int expected_arm[15] = {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1};
        static const int expected_standing[15] = {0,0,0,2,2,2,2,2,2,2,2,1,1,0,0};
        static const int expected_crouched[15] = {0,0,0,3,3,3,3,3,3,3,3,2,2,1,1};
        for (i=0;i<15;i++) {
            w.timer=(VKByte)i;
            assert(vk_whip_arm_frame(&w)==expected_arm[i]);
            assert(vk_whip_visual_step(&w)==expected_standing[i]);
            assert(vk_whip_crouch_visual_step(&w)==expected_crouched[i]);
            assert(vk_whip_visual_step(&w)>=0 && vk_whip_visual_step(&w)<3);
            assert(vk_whip_crouch_visual_step(&w)>=0 && vk_whip_crouch_visual_step(&w)<4);
            if (vk_whip_active(&w)) assert(vk_whip_segments(&w)>0);
            else assert(vk_whip_segments(&w)==0);
        }
    }
    return 0;
}
