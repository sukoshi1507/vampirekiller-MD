/* Stage12: room BA26, body 15h BA4F/BA64 and rider 18h BADD/BAF6.
 * Existing walker storage preserves the public prefix of VKBoss.
 * idx = +0C, wait=+10, timer=+11, facing=+12. p14=CE07, b->vx=CE09. */
static int boss12_floor(const VKBoss *b, int x, int y)
{
    if (!b->collision) return 0;
    return vk_tile_solid(vk_sample_tile(b->collision,(VKByte)x,(VKByte)y),12,0) ||
           vk_tile_solid(vk_sample_tile(b->collision,(VKByte)(x-10),(VKByte)y),12,0);
}
static void boss12_init(VKBoss *b)
{
    VKBossWalker *body=&b->walker[0],*rider=&b->walker[1];
    body->alive=body->moving=1;body->x=0xD000;body->y=0xC000;body->wait=0x30;body->turns=32;
    rider->alive=rider->moving=1;rider->x=0xD000;rider->y=0xA000;rider->facing=0x30;rider->turns=1;
    b->room_step=1;b->alive=1;
}
static void boss12_body(VKBoss *b,int sx)
{
    static const VKBossU8 poses[4]={0x79,0x7A,0x7B,0x7A};
    VKBossWalker *w=&b->walker[0];const int x=HI(w->x);
    if(w->wait && --w->wait==0)b->p14=1;
    w->timer++;w->pose=poses[(w->timer>>3)&3];
    if(w->state==1) {
        if(x>=0xE0)w->vx=-0x220;else if(x<0x10)w->vx=0x220;
        if(--w->idx==0)w->state=0;
    } else {
        w->idx=0x20;
        b->vx=(short)(sx>=x || x-sx<=0x40 ? 0x220:-0x220);
        w->vx=b->vx;w->state++;
    }
}
static void boss12_fire(VKBoss *b,const VKBossWalker *w,int sx,int sy,int difficulty)
{
    int i;
    for(i=0;i<VK_BOSS_SHOTS;i++)if(!b->shot[i].active) {
        VKBossShot *s=&b->shot[i];s->active=1;
        s->x=(unsigned short)(w->x&0xFF00);s->y=(unsigned short)((VKBossU8)(HI(w->y)-16)<<8);
        aim(HI(w->y),HI(w->x),sy,sx,difficulty,&s->vy,&s->vx);
        /* 9F61 has no 509F request here: unlike BF44, this shot is silent. */
        return;
    }
}
static void boss12_rider(VKBoss *b,int sx,int sy,int difficulty)
{
    VKBossWalker *w=&b->walker[1];
    if(w->wait) {w->wait--;w->moving=0;return;}
    w->moving=1;
    if(w->state==0) {
        if(b->p14) {w->timer=0;w->vy=-0x600;w->state=1;}
        else {w->pose=0x67;w->vx=b->vx;}
    } else if(w->state==1) {
        w->timer++;
        w->pose=(VKBossU8)((w->timer<4 ? 0x67:0x68)+(w->vx<0 ? 0:3));
        if(--w->facing==0){w->facing=0x30;boss12_fire(b,w,sx,sy,difficulty);}
        w->vy=add_vy(w->vy,0x60);
        if((HI(w->x)>=0xE8 && w->vx>=0)||(HI(w->x)<0x14&&w->vx<0))w->vx=(short)-w->vx;
        if(w->vy>=0 && boss12_floor(b,HI(w->x),HI(w->y))) {
            w->pose=(VKBossU8)(0x69+(w->vx<0 ? 0:3));w->moving=0;
            w->y=(unsigned short)((w->y&0x00FF)|(HI(w->y)&0xF8)<<8);w->idx=0x10;w->state++;
        }
    } else {
        w->moving=0;
        if(--w->idx==0) {
            const int base=HI(w->y)>=0x80 ? -0x780:-0x580;
            /* BBA8 computes HL += DE but A55D stores DE, not HL: R is consumed,
             * its arithmetic has no effect on the velocity (executed trace). */
            (void)Enemies_randomR();w->vy=(short)base;
            w->moving=1;w->timer=0;w->state=0;
        }
    }
}
static int room12_pass(VKBoss *b,int sx,int sy,int difficulty)
{
    int i;
    if(!b->room_step)boss12_init(b);
    if(b->walker[0].alive==1)boss12_body(b,sx);
    if(b->walker[1].alive==1)boss12_rider(b,sx,sy,difficulty);
    for(i=0;i<2;i++) {
        VKBossWalker *w=&b->walker[i];
        if(!w->alive)continue;
        if(w->moving){w->x=(unsigned short)(w->x+w->vx);w->y=(unsigned short)(w->y+w->vy);}
        if(HI(w->y)>=0xE4 || HI(w->x)>=0xF1 || HI(w->x)<7)w->alive=0;
    }
    b->alive=b->walker[0].alive;
    b->x=b->walker[0].x;b->y=b->walker[0].y;
    return 0;
}
static void boss12_flame(VKBoss *b,int k,int offset)
{
    VKBossWalker *w=&b->walker[k];
    if(!w->alive)return;
    w->alive=2;w->armed=0;w->state=0;w->moving=0;w->pose=0x85;w->idx=16;w->turns=1;
    w->x=(unsigned short)(w->x&0xFF00);w->y=(unsigned short)((VKBossU8)(HI(w->y)-offset)<<8);
    /* 5F1D clears +6, but the unused velocity bytes survive in the slot. */
    b->p15|=(VKBossU8)(1<<k);
}
static void boss12_flames_pass(VKBoss *b)
{
    int i;for(i=0;i<2;i++) {
        VKBossWalker *w=&b->walker[i];if(w->alive!=2)continue;
        w->pose=(w->idx&4)?0x86:0x85;
        if(--w->idx==0)w->alive=0;
    }
}

#include "generated/boss12_poses.h"
static const VKBoss12Pose *boss12_pose(unsigned char pose)
{
    int i;for(i=0;i<9;i++)if(vk_boss12_poses[i].pose==pose)return &vk_boss12_poses[i];
    return 0;
}
/* D700 0Ch (9A1A/9B60/9B71): harmless sixteen-pass disappearance flame. */
static void boss12_burst(VKBossShot *s)
{
    if(!s->active)return;
    s->active=2;s->anim=16;s->x&=0xFF00;s->y&=0xFF00;s->vx=s->vy=0;
}
static void boss12_shot_disappear(VKBoss *b,int index)
{
    VKBossShot copy=*boss_shot(b,index);int i;
    if(!copy.active)return;
    boss_shot(b,index)->active=0;
    for(i=0;i<vk_boss_shot_count(b);i++)if(!boss_shot(b,i)->active){
        *boss_shot(b,i)=copy;boss12_burst(boss_shot(b,i));
        if(b->kind==4){b->shot_state[i]=0;b->shot_pose[i]=0x85;}
        return;
    }
}
static void boss12_bursts_pass(VKBoss *b)
{
    int i;for(i=0;i<vk_boss_shot_count(b);i++) {
        VKBossShot *s=boss_shot(b,i);if(s->active!=2)continue;
        if(b->kind==4)b->shot_pose[i]=(s->anim&4)?0x86:0x85;
        if(--s->anim==0)s->active=0;
    }
}
static int boss12_sprites(const VKBoss *b,VKBossSprite *out)
{
    int k,i,n=0;
    for(k=0;k<2;k++) {
        const VKBossWalker *w=&b->walker[k];
        const VKBoss12Pose *p=boss12_pose(w->pose);
        if(w->alive==2) {
            out[n].x=(short)(HI(w->x)-8);out[n].y=(short)(HI(w->y)-(w->pose==0x86?14:15));
            out[n].tile=(unsigned short)(64+(w->pose==0x86?4:0));out[n].size=2;n++;continue;
        }
        if(!w->alive || !p)continue;
        for(i=0;i<p->count;i++) {
            out[n].x=(short)((unsigned char)(HI(w->x)+p->cell[i].dx));
            out[n].y=(short)((unsigned char)(HI(w->y)+p->cell[i].dy)+1);
            out[n].tile=p->cell[i].tile;out[n].size=2;n++;
        }
    }
    for(i=0;i<VK_BOSS_SHOTS;i++)if(b->shot[i].active) {
        const int burst=b->shot[i].active==2;
        const int alt=burst && ((b->shot[i].anim+1)&4);
        out[n].x=(short)((unsigned char)(HI(b->shot[i].x)-8));
        out[n].y=burst?(short)(HI(b->shot[i].y)-(alt?14:15)):(short)((unsigned char)(HI(b->shot[i].y)-12)+1);
        out[n].tile=(unsigned short)(burst?64+(alt?4:0):60);out[n].size=2;n++;
    }
    return n;
}
