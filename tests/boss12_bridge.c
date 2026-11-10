#include <string.h>
#include "../sgdk/src/boss.c"
static VKBoss b;
void b12_init(const unsigned char *map) {vk_boss_begin_stage(&b,12);vk_boss_set_collision(&b,map);}
int b12_step(int sx,int sy,int difficulty,int r,int frozen,int energy)
{
 int ev;Enemies_s7R=(EnemyS16)r;
 vk_boss_frame(&b,sx,sy,difficulty,frozen,energy);ev=vk_boss_frame(&b,sx,sy,difficulty,frozen,energy);return ev;
}
void b12_record(int k,unsigned char *out)
{
 VKBossWalker *w=&b.walker[k];memset(out,0,32);
 out[0]=w->alive?((b.p15&(1<<k))?0x1E:k?0x18:0x15):0;out[1]=w->state;out[2]=(unsigned char)w->y;out[3]=HI(w->y);out[4]=(unsigned char)w->x;out[5]=HI(w->x);
 out[6]=w->moving;out[7]=(unsigned char)w->vy;out[8]=HI(w->vy);out[9]=(unsigned char)w->vx;out[10]=HI(w->vx);
 out[11]=w->pose;out[12]=w->idx;out[13]=w->turns;out[14]=w->alive && !(b.p15&(1<<k))?(unsigned char)(7 & ~(w->armed?1:0)):0;
 out[16]=w->wait;out[17]=w->timer;out[18]=w->facing;
}
int b12_shot(int k,unsigned char *out)
{
 VKBossShot *s;if(k>=VK_BOSS_SHOTS)return 0;s=&b.shot[k];if(!s->active)return 0;memset(out,0,32);
 out[0]=s->active==2?12:10;out[2]=(unsigned char)s->y;out[3]=HI(s->y);out[4]=(unsigned char)s->x;out[5]=HI(s->x);out[6]=1;
 out[7]=(unsigned char)s->vy;out[8]=HI(s->vy);out[9]=(unsigned char)s->vx;out[10]=HI(s->vx);out[11]=s->active==2?(((s->anim+1)&4)?0x86:0x85):3;out[12]=s->anim;return 1;
}
int b12_signal(void){return b.p14;}
int b12_direction(void){return (unsigned short)b.vx;}
int b12_sprites(int *out)
{
 VKBossSprite sprites[VK_BOSS_MAX_SPRITES];int i,n=vk_boss_sprites(&b,sprites);
 for(i=0;i<n;i++) {
  const int tile=sprites[i].tile;
  out[3*i]=(unsigned char)(sprites[i].y-1);out[3*i+1]=(unsigned char)sprites[i].x;
  out[3*i+2]=tile==60?0xF0:tile>=64?0xF4+(tile-64):tile<36?0x40+2*tile:0xA8+2*(tile-36);
 }
 return n;
}
int b12_health(void){return b.energy;}
int b12_hit(int weapon){return vk_boss_hit(&b,weapon);}
int b12_script(int *out)
{
 out[0]=b.ending;out[1]=b.script;out[2]=b.script_wait;out[3]=b.orb;out[4]=b.orb_state;
 out[5]=b.orb_pose;out[6]=b.orb_anim;out[7]=b.orb_wait;out[8]=b.orb_y;
 out[9]=b.orb_landed;out[10]=b.orb_taken;return 11;
}
int b12_orb_touch(int sx,int sy,int state){return vk_boss_orb_touch(&b,sx,sy,state);}
void b12_place(int part,int x,int y)
{
 b.walker[part].x=(unsigned short)(x<<8);b.walker[part].y=(unsigned short)(y<<8);
}
int b12_contact(int sx,int sy,int state){return vk_boss_touch_x(&b,sx,sy,state)>=0;}
int b12_whip(int sx,int sy,int left,int state,int chain){return vk_boss_whip_touches(&b,sx,sy,left,state,chain);}
int b12_rider_whip(int sx,int sy,int left,int state,int chain){return vk_boss_rider_whip(&b,sx,sy,left,state,chain);}

int b12_rider_hit(int power){return vk_boss_rider_hit(&b,power);}
int b12_orb_record(unsigned char *out)
{
 VKBossWalker *w=&b.walker[0];b12_record(0,out);
 if(b.script<3)return 0;
 out[0]=b.orb?0x22:0;out[1]=b.orb_state;out[2]=0;out[3]=b.orb_y;out[4]=0;out[5]=0x78;
 out[6]=(unsigned char)(b.orb_state==1);out[7]=0;out[8]=1;out[9]=0;out[10]=0;out[11]=b.orb_pose;
 out[12]=b.orb_anim;out[13]=1;out[14]=b.orb&&b.orb_landed?2:0;out[16]=b.orb_wait;
 out[17]=w->timer;out[18]=w->facing;return 1;
}
void b12_rearm(void){vk_boss_rearm(&b);}
int b12_score(void){return vk_boss_score(&b);}
int b12_actor_contact(int sx,int sy,int posture,int facing,int armor,int *out)
{
 EnemyState es;EnemyRoomVars v;int x=vk_boss_touch_x(&b,sx,sy,posture),hit;
 Enemies_init(&es);memset(&v,0,sizeof v);v.simonX=(EnemyU8)sx;v.simonY=(EnemyU8)sy;v.simonState=(EnemyU8)posture;
 v.facingLeft=(EnemyU8)facing;v.armorOn=(EnemyU8)armor;es.armor=16;Enemies_god=0;
 hit=Enemies_contact(&es,&v,(EnemyU8)vk_boss_touch_damage(&b,sx,sy,posture),x,1);
 out[0]=es.energy;out[1]=es.armor;return hit;
}
void b12_navigation(int *out)
{
 out[0]=vk_boss_door(12,5);out[1]=vk_boss_room_of(12);out[2]=vk_boss_room(12,6);out[3]=vk_boss_entry_x(12);out[4]=vk_boss_entry_y(12);
}
void b12_place_shot(void){b.shot[0].active=1;b.shot[0].x=0x8000;b.shot[0].y=0xA000;}
int b12_shot_contact(int sx,int sy,int state){return vk_boss_shot_touches(&b,sx,sy,state)==0;}
int b12_shot_whip(int sx,int sy,int left,int state,int chain){return vk_boss_shot_whip_touches(&b,sx,sy,left,state,chain)==0;}
void b12_place_shot_slot(int slot)
{
 int i;for(i=0;i<VK_BOSS_SHOTS;i++)b.shot[i].active=0;
 b.shot[slot].active=1;b.shot[slot].x=0x8000;b.shot[slot].y=0xA000;
}
void b12_shot_kill(int slot){vk_boss_shot_kill(&b,slot);}
