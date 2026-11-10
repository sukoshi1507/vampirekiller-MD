#include "secret.h"
VKSecret vk_secret;
void vk_secret_enter(VKSecret *s,VKByte stage,VKByte room)
{
    unsigned i;
    for(i=0;i<4;i++)s->point[i]=stage<VK_SECRET_STAGES && room<vk_secret_room_counts[stage] ? vk_secret_points[stage][room][i]:0;
    s->active=s->phase=s->route=s->platform=0;
}
int vk_secret_near(const VKSecret *s,VKByte y,VKByte x)
{
    return s->point[0] && (VKByte)(y-4-s->point[1])<16 && (VKByte)(x-s->point[2])<16;
}
VKByte vk_secret_begin(VKSecret *s,VKByte state,VKByte y,VKByte x,VKByte up,VKByte *invuln)
{
    if(s->active || state!=2 || !up || !vk_secret_near(s,y,x))return 0;
    s->active=7;s->phase=0;*invuln=VK_SECRET_DELAY;
    return VK_SECRET_EFFECT;
}
int vk_secret_poll(VKSecret *s,VKByte invuln)
{
    if(!s->active || invuln)return 0;
    s->active=0;s->platform=0;s->route=255;
    return 1;
}
int vk_secret_route(VKSecret *s,VKByte *room)
{
    if(s->route!=255)return 0;
    s->route=0;*room=s->point[3];return 1;
}

void vk_secret_interrupt(VKSecret *s)
{
    s->active=0; /* another Simon state has replaced7; never request B98C */
}
