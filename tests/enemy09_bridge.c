#include <string.h>
#include "../sgdk/src/enemies.c"
static EnemyState s;
static EnemyRoomVars v;
static EnemySpawnTimer t;
void nine_init(const unsigned char *map,int stage,int room,int x,int y,int sx,int difficulty)
{
 Enemies_init(&s);memset(&v,0,sizeof v);v.stage=(EnemyU8)stage;v.room=(EnemyU8)room;v.map=map;
 v.simonX=(EnemyU8)sx;v.simonY=160;v.difficulty=(EnemyU8)difficulty;
 type09Spawn(&s,(EnemyU8)x,(EnemyU8)y);Enemies_spawnTimerInit(&t);
 s.type5Timer=ENEMY_TYPE5_DELAY;s.spawnPause=255;Enemies_god=1;
}
void nine_step(int sx) {v.simonX=(EnemyU8)sx;Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);}
int nine_record(unsigned char *b,int *sprites)
{
 Enemy *e=&s.enemy[0];S8Ext *q=ext(&s,e);EnemySprite sat[ENEMY_SPRITES];int i,n;
 memset(b,0,32);b[0]=e->active?9:0;b[1]=e->state;b[2]=e->sub;b[3]=e->y;
 b[4]=e->timer;b[5]=(EnemyU8)e->x;b[6]=q->mobile;b[7]=(EnemyU8)e->vy;b[8]=(EnemyU8)(e->vy>>8);
 b[9]=(EnemyU8)q->vx;b[10]=(EnemyU8)(q->vx>>8);b[11]=e->pose;b[12]=q->c12;
 b[13]=e->extraPhase;b[14]=e->active&&e->state?7:0;b[17]=q->c11;b[18]=q->c18;
 n=Enemies_sprites(&s,0,sat);
 for(i=0;i<n;i++){sprites[3*i]=(EnemyU8)(sat[i].y-1);sprites[3*i+1]=(EnemyU8)sat[i].x;sprites[3*i+2]=sat[i].pattern;}
 return n;
}
