#include <assert.h>
#include <stdio.h>
#include "enemies.h"
#include "gameplay.h"
#include "room_manager.h"
#include "generated/enemy_rooms.h"
static int always_touch(const void *ctx,int slot,int x,int y,int h,int l)
{ (void)ctx;(void)x;(void)y;(void)h;(void)l;return slot==0; }

int main(void) {
 int st,room,i,frame,panthers=0,flies=0;
 for(st=1;st<=11;st++) for(room=0;room<vk_stage_room_count(st);room++) {
  EnemyState s;EnemySpawnTimer t;EnemyRoomVars v={0};VKGame g={0};int expected=0,actual=0,seen4=0;
  assert(vk_game_debug_start(&g,st,room));vk_game_room_map(&g);
  Enemies_init(&s);Enemies_spawnTimerInit(&t);
  v.stage=st;v.room=room;v.simonX=128;v.simonY=32;v.map=vk_game_collision(&g);
  assert(Enemies_roomFlags(st,room)==vk_enemy_room_flags[st][room]);
  for(i=0;i<4;i++) expected+=(vk_fixed_enemies[st][room][i].raw_type&127)==5;
  Enemies_frame(&s,&t,&v);
  for(i=0;i<ENEMY_MAX;i++) if(s.enemy[i].active&&s.enemy[i].type==ENEMY_TYPE_5) actual++;
  assert(actual==expected);panthers+=actual;
  for(i=0;i<4;i++) if((vk_fixed_enemies[st][room][i].raw_type&127)==5) {
   int j,matched=0;
   for(j=0;j<ENEMY_MAX;j++) if(s.enemy[j].active&&s.enemy[j].type==ENEMY_TYPE_5 &&
      s.enemy[j].x==vk_fixed_enemies[st][room][i].x&&s.enemy[j].y==vk_fixed_enemies[st][room][i].y) matched++;
   assert(matched==1);
  }
  for(frame=1;frame<600;frame++) {
   EnemySprite sprites[ENEMY_SPRITES];Enemies_frame(&s,&t,&v);
   for(i=0;i<ENEMY_MAX;i++) if(s.enemy[i].active) {
    assert(s.enemy[i].type<=ENEMY_TYPE_FLEA);
    Enemies_sprites(&s,i,sprites);
    if(s.enemy[i].type==ENEMY_TYPE_4) seen4=1;
   }
  }
  if(vk_enemy_room_flags[st][room]&8) {assert(seen4);flies++;}
  Enemies_setRoom(&s);
  for(i=0;i<ENEMY_MAX;i++) assert(!s.enemy[i].active);
 }
 assert(panthers==6);
 /* Stage 4, objects 06 (trace, research/stage4-enemies.md). Room 0: both at their list place,
  * 4 hit points, X of the top row 60 frames later as in the trace. */
 {
  EnemyState s;EnemySpawnTimer t;EnemyRoomVars v={0};VKGame g={0};int hits,k,found=-1;
  assert(vk_game_debug_start(&g,4,0));vk_game_room_map(&g);
  Enemies_init(&s);Enemies_spawnTimerInit(&t);
  v.stage=4;v.room=0;v.simonX=16;v.simonY=192;v.map=vk_game_collision(&g);
  Enemies_frame(&s,&t,&v);
  for(i=0;i<ENEMY_MAX;i++) if(s.enemy[i].active&&s.enemy[i].type==ENEMY_TYPE_6&&s.enemy[i].y==192) {found=i;assert(s.enemy[i].x==224);}
  assert(found>=0);
  for(k=0;k<60;k++) Enemies_frame(&s,&t,&v);
  assert(s.enemy[found].x==215);
  /* Whip: 4 hits of 1, the last kills; chain: 2 and 2. */
  for(k=0;k<3;k++) {
   assert(Enemies_whipHitStage4(&s,s.enemy[found].x-20,192+12,0,0,0,&hits)==0&&hits==1);
   assert(s.enemy[found].active);
  }
  assert(Enemies_whipHitStage4(&s,s.enemy[found].x-20,192+12,0,0,0,&hits)==1&&hits==1);
  assert(!Enemy_alive(s.enemy[found]));
  /* A thrown weapon touches once until 999F: 8083, 7356. */
  {
   int slot=0,score=0;
   Enemies_setRoom(&s);
   Enemies_frame(&s,&t,&v);
   found=-1;
   for(i=0;i<ENEMY_MAX;i++) if(s.enemy[i].active&&s.enemy[i].type==ENEMY_TYPE_6) {found=i;break;}
   assert(found>=0);
   Enemies_shotPower[0]=Enemies_shotPower[1]=1;
   assert(Enemies_shotHit(&s,always_touch,0,&slot,&score)==1&&score==0);
   for(i=0;i<ENEMY_MAX;i++) if(i!=found) s.enemy[i].active=0;
   assert(Enemies_shotHit(&s,always_touch,0,&slot,&score)==0);
   Enemies_rearm(&s);
   assert(Enemies_shotHit(&s,always_touch,0,&slot,&score)==1&&score==0);
   Enemies_shotPower[0]=4;Enemies_rearm(&s);
   assert(Enemies_shotHit(&s,always_touch,0,&slot,&score)==1&&score==2&&!Enemy_alive(s.enemy[found]));
  }
 }
 /* Stage 6, object 07 (bat), research/stage6-enemies.md. Positions of an openMSX trace, pass by pass (2 frames).
  * Simon (240, 112): above the bat, vy = -100h at the retarget; then Simon (240, 128): vy = +100h. */
 {
  static const int y1[7]={119,118,117,116,115,114,113}, x1[7]={233,234,235,236,237,238,239}, f1[7]={32,64,96,128,160,192,224};
  EnemyState s;EnemySpawnTimer t;EnemyRoomVars v={0};
  Enemies_init(&s);Enemies_spawnTimerInit(&t);
  v.stage=6;v.room=1;v.simonX=240;v.simonY=112;
  s.enemy[0]=(Enemy){0};
  s.enemy[0].active=1;s.enemy[0].type=ENEMY_TYPE_7;s.enemy[0].x=232;s.enemy[0].y=120;
  s.enemy[0].dx=1;s.enemy[0].anim=1;s.enemy[0].pose=0x18;s.enemy[0].extraPhase=2;
  Enemies_frame(&s,&t,&v);
  for(i=0;i<7;i++) {
   assert(s.enemy[0].y==y1[i]&&s.enemy[0].sub==0&&s.enemy[0].x==x1[i]&&s.enemy[0].timer==f1[i]);
   Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);
  }
  assert(s.enemy[0].anim==9);   /* trace: c = 9 at the 8th pass */
  Enemies_init(&s);v.simonY=128;
  s.enemy[0]=(Enemy){0};
  s.enemy[0].active=1;s.enemy[0].type=ENEMY_TYPE_7;s.enemy[0].x=124;s.enemy[0].y=104;
  s.enemy[0].dx=1;s.enemy[0].anim=1;s.enemy[0].vy=-256;s.enemy[0].pose=0x18;s.enemy[0].extraPhase=2;
  Enemies_frame(&s,&t,&v);
  assert(s.enemy[0].vy==256&&s.enemy[0].y==105&&s.enemy[0].x==125&&s.enemy[0].timer==32&&s.enemy[0].anim==16);
  assert(s.enemy[0].pose==0x18);
 }
 /* Stage 6 room 2, object 0A and its ball: trace 1845-1944 (pass 8 of the wait, 24 of the flashing, 18 of the
  * second shot), ball X 196 + 4 per pass up to 240. */
 {
  EnemyState s;EnemySpawnTimer t;EnemyRoomVars v={0};int n,k,ball=-1,statue=-1,hits;
  Enemies_init(&s);Enemies_spawnTimerInit(&t);
  v.stage=6;v.room=2;v.simonX=232;v.simonY=160;
  for(n=1;n<=100;n++) {
   Enemies_frame(&s,&t,&v);
   for(k=0;k<ENEMY_MAX;k++) {
    if(s.enemy[k].active&&s.enemy[k].type==ENEMY_TYPE_STATUE) statue=k;
    if(s.enemy[k].active&&s.enemy[k].type==ENEMY_TYPE_FIREBALL) ball=k;
   }
   if(n==2) {assert(statue>=0&&s.enemy[statue].x==192&&s.enemy[statue].y==80&&s.enemy[statue].timer==7);}
   if(n==62) {assert(s.enemy[statue].state==1&&s.enemy[statue].anim==1);assert(ball<0||!s.enemy[ball].active);}
   if(n==64) {assert(s.enemy[statue].state==2&&s.enemy[statue].sub==18&&ball>=0&&s.enemy[ball].active&&s.enemy[ball].x==196&&s.enemy[ball].y==60);}
   if(n==66) {assert(s.enemy[ball].x==200);}
   if(n==86) {assert(s.enemy[ball].x==240);}
   if(n==88) {assert(!s.enemy[ball].active);}
   if(n==98) {assert(s.enemy[statue].state==2&&s.enemy[statue].sub==1);}
   if(n==100) {assert(s.enemy[statue].state==0&&s.enemy[statue].timer==40&&s.enemy[ball].active&&s.enemy[ball].x==196);}
  }
  /* 847A with L = 3, H = 6 for the ball (class 0 of D700, 8318): Simon within 8 px in X and Y 56..87 (code only, not traced). */
  {
   Enemy b=(Enemy){0};
   b.active=1;b.type=ENEMY_TYPE_FIREBALL;b.x=228;b.y=60;
   v.simonX=232;v.simonY=70;assert(Enemies_touches(&b,&v));
   v.simonY=100;assert(!Enemies_touches(&b,&v));
   v.simonY=70;v.simonX=237;assert(!Enemies_touches(&b,&v));
  }
  /* The whip: 8 hit points (1 each), 300 points; the ball dies at once for 100. */
  for(k=0;k<7;k++) {
   assert(Enemies_whipHitStage4(&s,s.enemy[statue].x-20,80+12,0,0,0,&hits)==0&&hits==1);
   Enemies_rearm(&s);
  }
  assert(Enemies_whipHitStage4(&s,s.enemy[statue].x-20,80+12,0,0,0,&hits)==1&&Enemies_killScore==3&&!Enemy_alive(s.enemy[statue]));
  assert(Enemies_whipHitStage4(&s,s.enemy[ball].x-20,s.enemy[ball].y+12,0,0,0,&hits)==1&&Enemies_killScore==1&&!s.enemy[ball].active);
 }
 /* Stage 6 room 0: bat channel 9DC3 (table 9DCD), rooms 3 and 4: object 08 (9DD5) through the same code. */
 {
  EnemyState s;EnemySpawnTimer t;EnemyRoomVars v={0};int n,k,seen7=0,seen8=0;
  Enemies_init(&s);Enemies_spawnTimerInit(&t);
  v.stage=6;v.room=0;v.simonX=232;v.simonY=128;
  for(n=0;n<400;n++) {
   Enemies_frame(&s,&t,&v);
   for(k=0;k<ENEMY_MAX;k++) if(s.enemy[k].active&&s.enemy[k].type==ENEMY_TYPE_7) {seen7=1;assert(s.enemy[k].y>=119&&s.enemy[k].y<=121);}
  }
  assert(seen7);
  Enemies_init(&s);Enemies_spawnTimerInit(&t);v.room=3;
  for(n=0;n<400;n++) {
   Enemies_frame(&s,&t,&v);
   for(k=0;k<ENEMY_MAX;k++) if(s.enemy[k].active&&s.enemy[k].type==ENEMY_TYPE_8) seen8=1;
  }
  assert(seen8);
 }
 assert(Enemies_roomFlags(11,0)==0&&Enemies_roomFlags(3,9)==0);
 /* Stage 3 has E=C0, unlike stage 2's E=A0 (original 9D17/9D1B). */
 {
  EnemyState s;EnemySpawnTimer t;EnemyRoomVars v={0};int fired=0;
  Enemies_init(&s);Enemies_spawnTimerInit(&t);v.stage=3;v.room=3;v.simonX=128;
  for(i=0;i<100;i++)if(Enemies_spawnTimerTick(&t,&v,&s)){fired=1;break;}
  assert(fired&&s.enemy[0].y==0xA0);
 }
 printf("73 rooms x 600 frames; %d fixed panthers; %d flying-enemy rooms; reset, sprites and stage-3 spawn height PASS\n",panthers,flies);
 return 0;
}
