/* BBF3/BC54/BC6D and BC29: stage15 body16h and D700 projectile08h.
 * Walker0: idx=+C, turns=+D. room_wait=CE02, p12=CF20. */
static int boss15_floor(const VKBoss *b,int x,int y)
{
    if(!b->collision)return 0;
    return vk_tile_solid(vk_sample_tile(b->collision,(VKByte)x,(VKByte)y),15,0) ||
           vk_tile_solid(vk_sample_tile(b->collision,(VKByte)(x-10),(VKByte)y),15,0);
}
static void boss15_spawn_shot(VKBoss *b)
{
    static const VKBossU8 xy[8][2]={{48,48},{192,128},{192,48},{48,192},{128,72},{128,160},{48,104},{192,104}};
    const int row=b->p12&7;
    int i,used=0;
    b->p12++; boss15_clock=b->p12; /* BC29 advances CF20 even when allocation fails. */
    for(i=0;i<VK_BOSS_MAX_SHOTS;i++)if(boss_shot(b,i)->active)used+=2;
    /* D700 allocates in D660..D67C. The body's twelve SAT entries occupy
     * D660/D664 too, leaving six entries (three projectiles), not eight. */
    if(used+2 > (b->walker[0].alive==1 ? 6:8))return;
    for(i=0;i<VK_BOSS_MAX_SHOTS;i++)if(!boss_shot(b,i)->active) {
        VKBossShot *s=boss_shot(b,i);
        if(vk_boss15_pool){int reserved=vk_boss15_pool(i+1,2);if(reserved<0)continue;if(!reserved)return;}
        s->active=1;s->x=(unsigned short)(xy[row][0]<<8);s->y=(unsigned short)(xy[row][1]<<8);
        s->vx=s->vy=0;s->anim=60;b->shot_state[i]=0;b->shot_pose[i]=3;return;
    }
}
static void boss15_room(VKBoss *b)
{
    if(!b->room_step) {
        VKBossWalker *w=&b->walker[0];int i;
        if(vk_boss15_pool && vk_boss15_pool(0,12)<=0)return;
        w->alive=1;w->x=0xA000;w->y=0x9000;w->vx=-0x280;w->vy=-0x200;w->idx=30;w->pose=0x7C;w->turns=32;
        b->alive=1;b->room_step=1;b->room_wait=0;
        for(i=0;i<4;i++)boss15_spawn_shot(b);
    } else if(--b->room_wait==0) {b->room_wait=128;boss15_spawn_shot(b);}
}
static void boss15_body(VKBoss *b)
{
    VKBossWalker *w=&b->walker[0];
    if(!w->alive)return;
    if(w->state!=1) {if(--w->idx==0){w->state++;w->moving=1;}}
    else {
        w->vy=(short)(w->vy+8); /* BC8A writes DE directly, no gravity clamp. */
        if((HI(w->x)>=0xE8 && w->vx>=0)||(HI(w->x)<0x18 && w->vx<0))w->vx=(short)-w->vx;
        if(HI(w->y)<0x40 && w->vy<0)w->vy=(short)-w->vy;
        if(boss15_floor(b,HI(w->x),HI(w->y)))w->vy=-0x300;
    }
    if(w->moving){w->x=(unsigned short)(w->x+w->vx);w->y=(unsigned short)(w->y+w->vy);}
    if(HI(w->y)>=0xE4 || HI(w->x)>=0xF1 || HI(w->x)<7)w->alive=0;
    b->alive=w->alive;b->x=w->x;b->y=w->y;
}
static void boss15_shots_pass(VKBoss *b,int sx,int sy,int difficulty)
{
    int i;
    for(i=0;i<VK_BOSS_MAX_SHOTS;i++) {
        VKBossShot *s=boss_shot(b,i);if(s->active!=1)continue;
        b->shot_pose[i]=(VKBossU8)(0x7D+((s->anim>>2)&3));
        if(--s->anim==0) {
            if(b->shot_state[i]==1){s->vx=s->vy=0;s->anim=60;b->shot_state[i]=0;}
            else {aim_speed(HI(s->y),HI(s->x),sy,sx,0x40,difficulty,&s->vy,&s->vx);s->anim=30;b->shot_state[i]++;}
        }
        s->x=(unsigned short)(s->x+s->vx);s->y=(unsigned short)(s->y+s->vy);
        if(HI(s->y)>=0xE4 || HI(s->x)>=0xF1 || HI(s->x)<7)s->active=0;
    }
}
#include "generated/boss15_poses.h"
static int boss15_sprites(const VKBoss *b,VKBossSprite *out)
{
    const VKBossWalker *w=&b->walker[0];int n=0,i;
    if(w->alive==2) {
        out[n].x=(short)(HI(w->x)-8);out[n].y=(short)(HI(w->y)-(w->pose==0x86?14:15));
        out[n].tile=(unsigned short)(44+(w->pose==0x86?4:0));out[n].size=2;n++;
    } else if(w->alive)for(i=0;i<6;i++) {
        const VKBoss15Cell *c=&vk_boss15_cells[i];
        out[n].x=(short)((VKBossU8)(HI(w->x)+c->dx));out[n].y=(short)((VKBossU8)(HI(w->y)+c->dy)+1);
        out[n].tile=c->tile;out[n].size=2;n++;
    }
    for(i=0;i<VK_BOSS_MAX_SHOTS;i++) {
        const VKBossShot *s=vk_boss_shot_data(b,i);if(!s->active)continue;
        if(s->active==2) {
            const int alt=b->shot_pose[i]==0x86;
            out[n].x=(short)(HI(s->x)-8);out[n].y=(short)(HI(s->y)-(alt?14:15));out[n].tile=(unsigned short)(44+(alt?4:0));
        } else {
            const VKBoss15Cell *c=&vk_boss15_cells[b->shot_pose[i]==3?10:6+b->shot_pose[i]-0x7D];
            out[n].x=(short)((VKBossU8)(HI(s->x)+c->dx));out[n].y=(short)((VKBossU8)(HI(s->y)+c->dy)+1);out[n].tile=c->tile;
        }
        out[n].size=2;n++;
    }
    return n;
}
