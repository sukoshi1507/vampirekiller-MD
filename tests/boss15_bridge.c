#include <string.h>
#include "../sgdk/src/boss.c"
static VKBoss b;
void b15_init(const unsigned char *map){vk_boss_clock_init();vk_boss_begin_stage(&b,15);vk_boss_set_collision(&b,map);}
int b15_step(int sx,int sy,int difficulty,int frozen,int energy)
{
 vk_boss_frame(&b,sx,sy,difficulty,frozen,energy);
 return vk_boss_frame(&b,sx,sy,difficulty,frozen,energy);
}
void b15_record(unsigned char *out)
{
 VKBossWalker *w=&b.walker[0];memset(out,0,32);
 out[0]=w->alive?((b.p15&1)?30:22):0;out[1]=w->state;out[2]=(unsigned char)w->y;out[3]=HI(w->y);out[4]=(unsigned char)w->x;out[5]=HI(w->x);
 out[6]=w->moving;out[7]=(unsigned char)w->vy;out[8]=HI(w->vy);out[9]=(unsigned char)w->vx;out[10]=HI(w->vx);
 out[11]=w->pose;out[12]=w->idx;out[13]=w->turns;out[14]=w->alive && !(b.p15&1)?(unsigned char)(7&~(w->armed?1:0)):0;
}
int b15_shot(int k,unsigned char *out)
{
 const VKBossShot *s=vk_boss_shot_data(&b,k);if(!s||!s->active)return 0;memset(out,0,32);
 out[0]=s->active==2?12:8;out[1]=b.shot_state[k];out[2]=(unsigned char)s->y;out[3]=HI(s->y);out[4]=(unsigned char)s->x;out[5]=HI(s->x);out[6]=1;
 out[7]=(unsigned char)s->vy;out[8]=HI(s->vy);out[9]=(unsigned char)s->vx;out[10]=HI(s->vx);out[11]=b.shot_pose[k];out[12]=s->anim;return 1;
}
int b15_sprites(int *out)
{
 VKBossSprite sprites[VK_BOSS_MAX_SPRITES];int i,n=vk_boss_sprites(&b,sprites);
 for(i=0;i<n;i++){out[3*i]=(unsigned char)(sprites[i].y-1);out[3*i+1]=(unsigned char)sprites[i].x;out[3*i+2]=sprites[i].tile>=44?0xF4+sprites[i].tile-44:sprites[i].tile==40?0xF0:0x40+2*sprites[i].tile;}
 return n;
}
void b15_room(int *out){out[0]=b.room_step;out[1]=b.room_wait;out[2]=b.p12;}
int b15_health(void){return b.energy;}
int b15_hit(int weapon){return vk_boss_hit(&b,weapon);}
void b15_script(int *out){out[0]=b.ending;out[1]=b.script;out[2]=b.script_wait;out[3]=b.orb;out[4]=b.orb_y;}
int b15_orb_touch(int sx,int sy,int state){return vk_boss_orb_touch(&b,sx,sy,state);}
int b15_orb_record(unsigned char *out)
{
 b15_record(out);if(b.script<3)return 0;
 out[0]=b.orb?34:0;out[1]=b.orb_state;out[2]=0;out[3]=b.orb_y;out[4]=0;out[5]=120;out[6]=b.orb_state==1;out[7]=0;out[8]=1;out[9]=0;out[10]=0;
 out[11]=b.orb_pose;out[12]=b.orb_anim;out[13]=1;out[14]=b.orb&&b.orb_landed?2:0;out[16]=b.orb_wait;return 1;
}

int b15_score(void){return vk_boss_score(&b);}
int b15_actor_contact(int sx,int sy,int posture,int facing,int armor,int *out)
{
 EnemyState es;EnemyRoomVars v;int x=vk_boss_touch_x(&b,sx,sy,posture),hit;
 Enemies_init(&es);memset(&v,0,sizeof v);v.simonX=(EnemyU8)sx;v.simonY=(EnemyU8)sy;v.simonState=(EnemyU8)posture;
 v.facingLeft=(EnemyU8)facing;v.armorOn=(EnemyU8)armor;es.armor=16;Enemies_god=0;
 hit=Enemies_contact(&es,&v,(EnemyU8)vk_boss_touch_damage(&b,sx,sy,posture),x,1);
 out[0]=es.energy;out[1]=es.armor;return hit;
}

void b15_place(int x,int y){b.walker[0].x=(unsigned short)(x<<8);b.walker[0].y=(unsigned short)(y<<8);}
int b15_contact(int x,int y,int state){return vk_boss_touch_x(&b,x,y,state)>=0;}
int b15_whip(int x,int y,int left,int state,int chain){return vk_boss_whip_touches(&b,x,y,left,state,chain);}
void b15_place_shot(void){int i;for(i=0;i<8;i++)boss_shot(&b,i)->active=0;b.shot[0].active=1;b.shot[0].x=0x8000;b.shot[0].y=0xA000;}
int b15_shot_contact(int x,int y,int state){return vk_boss_shot_touches(&b,x,y,state)==0;}
int b15_shot_whip(int x,int y,int left,int state,int chain){return vk_boss_shot_whip_touches(&b,x,y,left,state,chain)==0;}
void b15_reenter(const unsigned char *map){vk_boss_begin_stage(&b,15);vk_boss_set_collision(&b,map);}

void b15_shot_kill(int slot){vk_boss_shot_kill(&b,slot);}

void b15_navigation(int *out){out[0]=vk_boss_door(15,8);out[1]=vk_boss_room_of(15);out[2]=vk_boss_room(15,9);out[3]=vk_boss_entry_x(15);out[4]=vk_boss_entry_y(15);}
