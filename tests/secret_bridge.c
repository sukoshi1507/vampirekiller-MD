#include <string.h>
#include "secret.h"
#include "enemies.h"
static VKSecret s;
static EnemyState e;
static EnemySpawnTimer t;
static EnemyRoomVars v;
static unsigned char player[32],room;
void secret_init(int stage,int r,int x,int y,int state,int tick,int invuln,int platform)
{
    vk_secret_enter(&s,stage,r);Enemies_init(&e);Enemies_spawnTimerInit(&t);
    memset(&v,0,sizeof v);v.simonX=x;v.simonY=y;v.simonState=state;
    memset(player,0,sizeof player);player[0]=state;player[1]=19;player[5]=y;player[7]=x;
    player[14]=6;player[15]=0;player[29]=platform;
    s.phase=19;s.platform=platform;e.itemPhase=1;e.tick=tick;e.invuln=invuln;e.invulnPhase=0;room=r;
}
int secret_near(int x,int y){return vk_secret_near(&s,y,x);}
int secret_begin(int up)
{
    int sound=vk_secret_begin(&s,player[0],player[5],player[7],up,&e.invuln);
    if(sound){player[0]=7;player[1]=0;e.invulnPhase=0;}
    return sound;
}
int secret_poll(void)
{
    int event=vk_secret_poll(&s,e.invuln);
    if(event){player[0]=0;player[29]=0;}
    return event;
}
void secret_timers(void)
{
    v.simonState=player[0];v.simonX=player[7];v.simonY=player[5];
    Enemies_frame(&e,&t,&v);Enemies_frame(&e,&t,&v);
}
void secret_record(unsigned char *out)
{
    memcpy(out,player,32);out[13]=e.invuln;
}
void secret_values(int *out)
{
    int i;for(i=0;i<4;i++)out[i]=s.point[i];out[4]=s.route;out[5]=room;out[6]=e.tick;out[7]=Enemies_simonHidden(&e);
}
int secret_route(void){return vk_secret_route(&s,&room);}
#include "generated/simon_crouch_md.h"
void secret_face(int left){player[12]=left;player[14]=left?16:6;player[15]=left?15:0;}
void secret_image(int left,unsigned char *out)
{
    unsigned y,x;memset(out,0,2048);
    if(Enemies_simonHidden(&e))return;
    for(y=0;y<32;y++)for(x=0;x<64;x++){
        unsigned sx=left?63-x:x;unsigned half=sx/32;unsigned tile=(sx%32/8)*4+y/8;
        unsigned long word=vk_simon_crouch_tiles[half*128+tile*8+y%8];
        out[y*64+x]=(word>>(28-4*(sx%8)))&15;
    }
}

int secret_energy(void){return e.energy;}
int secret_frame_contact(int state){
    Enemies_init(&e);Enemies_spawnTimerInit(&t);memset(&v,0,sizeof v);
    v.simonX=88;v.simonY=172;v.simonState=state;
    e.enemy[0].active=1;e.enemy[0].type=1;e.enemy[0].x=80;e.enemy[0].y=156;e.enemy[0].dx=1;
    Enemies_god=0;return Enemies_frame(&e,&t,&v);
}

int secret_crusher(void){int hit;e.invuln=e.guardTimer=e.killTimer=e.recoil=0;e.energy=32;v.simonState=7;Enemies_god=0;hit=Enemies_contact(&e,&v,8,80,0);if(hit)vk_secret_interrupt(&s);return hit;}
