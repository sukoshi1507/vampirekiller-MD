#include <string.h>
static unsigned char random_values[32];static int random_count,random_used;
static unsigned char channel_random(void){int i=random_used++;return i<random_count?random_values[i]:0;}
#define VK_RANDOM_PROBE channel_random
#include "../sgdk/src/enemies.c"
static EnemyState s;static EnemyRoomVars v;static EnemySpawnTimer t;
void channels_init(int stage,int room)
{
 Enemies_init(&s);Enemies_spawnTimerInit(&t);memset(&v,0,sizeof v);v.stage=(EnemyU8)stage;v.room=(EnemyU8)room;
 v.simonY=160;v.simonX=64;s.type5Timer=1;Enemies_god=1;Enemies_s7R=0;
}
int channels_step(int sx,int pause,int frozen,int *out)
{
 int i,n=0;v.simonX=(EnemyU8)sx;if(pause>=0)s.spawnPause=(EnemyU8)pause;s.frozen=(EnemyU8)frozen;
 Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);
 out[n++]=t.phase;out[n++]=s.type4Count;out[n++]=s.type4Shots;
 for(i=0;i<ENEMY_MAX;i++)if(s.enemy[i].active) {
  Enemy *e=&s.enemy[i];S8Ext *q=ext(&s,e);
  out[n++]=e->type==ENEMY_TYPE_EAGLE?15:e->type==ENEMY_TYPE_FLEA?35:255;
  out[n++]=e->state;out[n++]=e->x;out[n++]=e->y;out[n++]=e->pose;out[n++]=q->c12;out[n++]=q->c11;
  e->active=0;
 }
 return n;
}

void channels_random(const unsigned char *p,int n){memcpy(random_values,p,n);random_count=n;random_used=0;}
int channels_random_used(void){return random_used;}
void channels_item(int flags){Enemies_item(&s,6,flags);}
int channels_pause(void){return s.spawnPause;}
