#include <string.h>
static unsigned char random_values[32];static int random_count,random_used;
static unsigned char twelve_r(void){int i=random_used++;return i<random_count?random_values[i]:0;}
#define VK_RANDOM_PROBE twelve_r
#include "../sgdk/src/enemies.c"
void twelve_random(const unsigned char *p,int n){memcpy(random_values,p,n);random_count=n;random_used=0;}
int twelve_random_used(void){return random_used;}
static EnemyState s;static EnemyRoomVars v;static EnemySpawnTimer t;
void twelve_init(const unsigned char *map,int stage,int room,int x,int y,int sx,int sy,int r)
{
 Enemies_init(&s);memset(&v,0,sizeof v);v.stage=(EnemyU8)stage;v.room=(EnemyU8)room;v.map=map;v.simonX=(EnemyU8)sx;v.simonY=(EnemyU8)sy;
 type12Spawn(&s,(EnemyU8)x,(EnemyU8)y);Enemies_spawnTimerInit(&t);s.type5Timer=1;s.spawnPause=255;Enemies_god=1;Enemies_s7R=(EnemyS16)r;
}
void twelve_step(void){Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);}
int twelve_record(int slot,unsigned char *b,int *sprites)
{
 const int k=slot<0?0:slot+ENEMY_MAX;Enemy *e;S8Ext *q;EnemySprite sat[ENEMY_SPRITES];int i,n;
 if(k>=ENEMY_ALL)return 0;
 e=Enemies_slot(&s,k);if(slot>=0&&(!e->active||e->type!=ENEMY_TYPE_FIREBALL))return 0;
 q=ext(&s,e);memset(b,0,32);b[0]=e->active?(slot<0?18:1):0;
 b[1]=slot<0?e->state:0;b[2]=e->sub;b[3]=e->y;b[4]=e->timer;b[5]=(EnemyU8)e->x;b[6]=slot<0?q->mobile:1;
 b[7]=(EnemyU8)e->vy;b[8]=(EnemyU8)(e->vy>>8);
 b[9]=slot<0?(EnemyU8)q->vx:e->anim;b[10]=slot<0?(EnemyU8)(q->vx>>8):e->pose;
 b[11]=slot<0?e->pose:3;
 if(slot<0){b[12]=q->c12;b[13]=e->extraPhase;b[14]=e->active?7:0;b[16]=q->c10;b[17]=q->c18;b[18]=q->c1e;b[19]=q->c11;b[20]=q->c14;}
 n=Enemies_sprites(&s,(EnemyU8)k,sat);for(i=0;i<n;i++){sprites[3*i]=(EnemyU8)(sat[i].y-1);sprites[3*i+1]=(EnemyU8)sat[i].x;sprites[3*i+2]=sat[i].pattern;}
 return n;
}
