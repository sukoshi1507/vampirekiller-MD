#include <string.h>
#include "../sgdk/src/enemies.c"
#include "item_effects.h"
#include "stage_map.h"
static EnemyState s;static EnemyRoomVars v;static EnemySpawnTimer t;
static unsigned char shot_metadata[7][32];
static int timer_sound;
static VKCandles c;static VKWall w;static VKItemMapState ms;

void item_init(int stage,int room,int flags){Enemies_init(&s);Enemies_spawnTimerInit(&t);memset(&v,0,sizeof v);s.itemPhase=1;v.simonX=88;v.simonY=172;v.stage=stage;v.room=room;s.type5Timer=18;Enemies_god=1;vk_candles_begin(&c);vk_wall_begin(&w);c.stage=stage;c.room=room;c.c431=flags;c.hearts=50;s.energy=32;ms.phase=0;}
int item_give(int kind){int events=vk_candles_give(&c,kind);if(events&VK_CANDLE_EVENT_PICK)Enemies_item(&s,kind,c.c431);return vk_candles_sound(kind);}
void item_timers(void){Enemies_frame(&s,&t,&v);timer_sound=s.itemSound;Enemies_frame(&s,&t,&v);if(s.itemSound)timer_sound=s.itemSound;}
void item_values(int *o){o[0]=c.picked;o[1]=c.c431;o[2]=c.c701|(c.gold_keys?(c.triple_key?4:2):0);o[3]=c.c702;o[4]=c.hearts;o[5]=c.gold_keys;o[6]=c.weapon;o[7]=c.c70f;o[8]=s.energy;o[9]=s.armor;o[10]=s.spawnPause;o[11]=s.guardTimer;o[12]=s.killTimer;o[13]=s.flash;o[14]=Enemies_simonColor(&s);o[15]=timer_sound;}
int item_box(int type,int sx,int sy,int left,int x,int y){return vk_item_shield_touch(type,sx,sy,left,x,y);}
int item_outline(int mode,int x,int y){return vk_item_outline_pixel(mode,x,y);}
void item_map(int stage,int room,int x,int y,unsigned char *out){vk_stage_map_render(out,&c,stage,room,x,y);}
int item_map_step(int flags,int boss,int press){return vk_item_map_step(&ms,&c,flags,boss,press);}
int item_map_state(void){return ms.phase;}
void item_shot(int type,int x,int y,int slot){Enemy *e=&s.enemy[slot];S8Ext *q=ext(&s,e);memset(e,0,sizeof *e);memset(q,0,sizeof *q);e->active=1;e->type=type==4?ENEMY_TYPE_BONE:type==9?ENEMY_TYPE_10_SHOT:ENEMY_TYPE_FIREBALL;e->x=x;e->y=y;e->pose=3;e->age=1;q->c10=slot;q->mobile=1;}
void item_shield_config(int sx,int sy,int left,int charges,int inventory){v.simonX=sx;v.simonY=sy;v.facingLeft=left;s.armor=charges;c.c701=inventory;}
int item_shield(void){return Enemies_shield(&s,&v,&c.c701);}
void item_shot_record(int slot,unsigned char *out,int *sprite){Enemy *e=&s.enemy[slot];S8Ext *q=ext(&s,e);EnemySprite sat[ENEMY_SPRITES];int i,n;memcpy(out,shot_metadata[slot],32);out[0]=e->active?(e->type==ENEMY_TYPE_SHIELD_SPARK?12:e->type==ENEMY_TYPE_BONE?4:e->type==ENEMY_TYPE_10_SHOT?9:3):0;out[1]=e->state;out[3]=e->y;out[5]=(VKByte)e->x;out[6]=q->mobile;out[7]=e->vy;out[8]=(VKByte)(e->vy>>8);out[9]=q->vx;out[10]=(VKByte)(q->vx>>8);out[11]=e->pose;out[12]=e->anim;out[14]=e->extraPhase;out[31]=0;n=Enemies_sprites(&s,slot,sat);for(i=0;i<n;i++){sprite[3*i]=sat[i].y-1;sprite[3*i+1]=sat[i].x;sprite[3*i+2]=sat[i].pattern;}sprite[12]=n;}

void item_shot_metadata(int slot,const unsigned char *p){memcpy(shot_metadata[slot],p,32);}
void item_freeze(int flag){s.frozen=flag;}
static VKGame item_game;
void item_recoil_init(int flags,int direction){vk_game_begin(&item_game);item_game.area=VK_AREA_INTERIOR;item_game.stage=1;item_game.room=0;item_game.player.x=96;item_game.player.y=192;item_game.items=flags;vk_game_hit(&item_game,direction,0);}
void item_recoil_step(int *out){vk_game_tick_controls(&item_game,0,0,0,0);vk_game_tick_controls(&item_game,0,0,0,0);out[0]=item_game.player.x;out[1]=item_game.player.y;out[2]=item_game.x_frac;out[3]=item_game.recoil_count;out[4]=item_game.recoil;}
void item_enemy(int type,int x,int y){
 switch(type){case 9:type09Spawn(&s,x,y);break;case 18:type12Spawn(&s,x,y);break;case 11:hopperSpawn(&s,x,y,&v);break;case 13:fleaSpawn(&s,x,y);break;case 35:flea23Spawn(&s,x,y);break;case 14:dragonSpawn(&s,x,y);break;case 16:type10Spawn(&s,x,y);break;case 15:{Enemy *flea;eagleSpawnRaw(&s,x,y,&flea);break;}}
}
void item_enemy_active09(int x,int fraction){s.enemy[0].x=x;s.enemy[0].timer=fraction;s.enemy[0].state=1;ext(&s,&s.enemy[0])->c11=0;ext(&s,&s.enemy[0])->mobile=1;ext(&s,&s.enemy[0])->c12=16;ext(&s,&s.enemy[0])->c18=0;s.enemy[0].pose=0x21;ext(&s,&s.enemy[0])->vx=0x220;}
void item_contact_once(int kind,int random){(void)kind;s.frozen=1;Enemies_god=0;Enemies_s7R=random;Enemies_frame(&s,&t,&v);}
void item_enemy_result(int *o){int i;o[0]=s.energy;o[1]=s.contactScore;o[2]=s.itemSound;for(i=0;i<7;i++){Enemy *e=&s.enemy[i];S8Ext *q=ext(&s,e);o[3+6*i]=e->active?(e->type==ENEMY_TYPE_DEATH?30:e->type==ENEMY_TYPE_09?9:e->type==ENEMY_TYPE_12?18:e->type==ENEMY_TYPE_HOPPER?11:e->type==ENEMY_TYPE_DRAGON?14:e->type==ENEMY_TYPE_10?16:e->type==ENEMY_TYPE_EAGLE?15:13):0;o[4+6*i]=e->x;o[5+6*i]=e->y;o[6+6*i]=e->pose;o[7+6*i]=e->type==ENEMY_TYPE_09?q->c11:e->anim;o[8+6*i]=e->type==ENEMY_TYPE_09?e->state:e->state;}}

void item_drop(int *out){out[0]=Enemies_dropCount;if(Enemies_dropCount){out[1]=Enemies_drops[0].kind;out[2]=Enemies_drops[0].x;out[3]=Enemies_drops[0].y;}}
#include "weapons.h"
static VKWeapons holy_water;
static VKByte water_clock;
static VKWeaponSimon water_simon;
static VKWeaponEvents water_event;
void item_water_init(int state,int hearts,int inventory){vk_weapons_reset(&holy_water);water_clock=0;water_simon=(VKWeaponSimon){88,112,state,0};c.hearts=hearts;c.c701=inventory;}
void item_water_begin(int left,int right,int whip){if(!whip&&(left||right)&&water_simon.state==1&&(c.c701&8))vk_weapons_water(&holy_water,&c.hearts,left);}
void item_water_step(void){vk_weapons_frame_clocked(&holy_water,&water_simon,0,vk_stage_room_map(1,0),1,&water_event,water_clock,1);water_clock++;vk_weapons_frame_clocked(&holy_water,&water_simon,0,vk_stage_room_map(1,0),1,&water_event,water_clock,0);}
void item_water_record(unsigned char *out){memset(out,0,32);memcpy(out,&holy_water.shot[0],9);memcpy(out+16,&holy_water.shot[1],9);}
void item_map_inventory(int inventory,int uses){c.c701=inventory;c.c70f=uses;}
#include "boss.h"
static VKBoss item_boss;
static unsigned char boss_metadata[32];
void item_boss_init(int kind,const unsigned char *raw){vk_boss_reset(&item_boss);item_boss.in_room=1;item_boss.kind=kind;item_boss.energy=128;item_boss.alive=1;item_boss.room_step=1;item_boss.room_wait=255;item_boss.shot[0].active=1;item_boss.shot[0].x=96<<8;item_boss.shot[0].y=144<<8;memcpy(boss_metadata,raw,32);}
void item_boss_shield(void){vk_boss_shot_shield(&item_boss,0);}
void item_boss_step(void){vk_boss_frame(&item_boss,88,172,0,1,32);vk_boss_frame(&item_boss,88,172,0,1,32);}
void item_boss_record(unsigned char *out){VKBossShot *q=&item_boss.shot[0];memcpy(out,boss_metadata,32);out[0]=q->active?12:0;out[1]=out[2]=out[4]=out[7]=out[8]=out[9]=out[10]=out[14]=out[31]=0;out[3]=q->y>>8;out[5]=q->x>>8;out[6]=1;out[11]=((q->anim+1)&4)?0x86:0x85;out[12]=q->anim;}
/* Retained bytes belong to the physical D700 slot, independently of native index. */
static unsigned char physical_metadata[8][32];
void item_pool_metadata(int slot,const unsigned char *raw){memcpy(physical_metadata[slot],raw,32);}
void item_pool_record(int slot,unsigned char *out){int i,sat[13];memcpy(out,physical_metadata[slot],32);out[0]=0;for(i=0;i<7;i++)if(!s.enemy[i].active&&s.enemy[i].type==ENEMY_TYPE_SHIELD_SPARK&&ext(&s,&s.enemy[i])->c10==slot)out[12]=s.enemy[i].anim;for(i=0;i<7;i++)if(s.enemy[i].active&&isProjectile(s.enemy[i].type)&&ext(&s,&s.enemy[i])->c10==slot){memcpy(shot_metadata[i],physical_metadata[slot],32);item_shot_record(i,out,sat);break;}memcpy(physical_metadata[slot],out,32);}
void item_pool_free(int i){s.enemy[i].active=0;}
static unsigned char enemy_metadata[7][32];
void item_enemy_metadata(int slot,const unsigned char *raw){memcpy(enemy_metadata[slot],raw,32);memcpy(enemyLoot[slot].raw,raw,32);}
void item_enemy_prefix(int slot,unsigned char *b){Enemy *e=&s.enemy[slot];S8Ext *q=ext(&s,e);memcpy(b,enemy_metadata[slot],32);b[0]=e->active?b[0]:0;
 if(isLoot(e->type) || (!e->active && (enemy_metadata[slot][0]==36 || enemy_metadata[slot][0]==38) && !enemyLoot[slot].raw[0])){memcpy(b,enemyLoot[slot].raw,32);}
 else if(e->type==ENEMY_TYPE_09){b[0]=e->active?9:0;b[1]=e->state;b[2]=e->sub;b[3]=e->y;b[4]=e->timer;b[5]=e->x;b[6]=q->mobile;b[7]=e->vy;b[8]=e->vy>>8;b[9]=q->vx;b[10]=q->vx>>8;b[11]=e->pose;b[12]=q->c12;b[13]=e->extraPhase;b[14]=e->active&&e->state?7:0;b[17]=q->c11;b[18]=q->c18;}
 else if(e->type==ENEMY_TYPE_DEATH){int k;for(k=0;k<32;k++)if(vk_item_death_mask[k])b[k]=vk_item_death_prefix[k];b[0]=e->active?30:0;b[3]=e->y;b[5]=e->x;b[11]=e->pose;b[12]=e->anim;b[31]=e->state;}
 memcpy(enemy_metadata[slot],b,32);
}
void item_long_step(void){Enemies_god=0;s.invuln=0;s.recoil=0;Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);}
void item_ghost_init(const unsigned char *map,int stage,int room,int x,int y,int sx,int sy){item_init(stage,room,0);v.stage=stage;v.room=room;v.map=map;v.simonX=sx;v.simonY=sy;s.type5Timer=ENEMY_TYPE5_DELAY;s.spawnPause=255;Enemies_candleObject(&s,stage,room,x,y);}
void item_ghost_step(void){s.spawnPause=255;Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);}
void item_ghost_prefix(unsigned char *b,int *sp){Enemy *e=&s.enemy[0];S8Ext *q=ext(&s,e);EnemySprite sat[8];int i,n;memset(b,0,32);b[0]=e->active?27:0;b[1]=e->state;b[2]=e->sub;b[3]=e->y;b[4]=e->timer;b[5]=e->x;b[6]=q->mobile;b[7]=e->vy;b[8]=e->vy>>8;b[9]=q->vx;b[10]=q->vx>>8;b[11]=e->pose;b[12]=q->c12;b[13]=e->extraPhase;b[14]=e->active&&e->state?7:0;b[16]=q->c10;n=Enemies_sprites(&s,0,sat);for(i=0;i<n;i++){sp[3*i]=sat[i].y-1;sp[3*i+1]=sat[i].x;sp[3*i+2]=sat[i].pattern;}sp[24]=n;}

void item_contact_map(const unsigned char *map){v.map=map;}
static int actor_sound;
void loot_actor_begin(const unsigned char *map,const unsigned char *meta,int stage,int type,int x,int y,int kind){Enemy *e;item_init(stage,0,0);v.map=map;v.simonX=16;v.simonY=32;memcpy(enemyLoot[0].raw,meta,32);e=newObject(&s,ENEMY_TYPE_LOOT_HEART);e->x=x;e->y=y;lootSpawn(&s,e,type,kind);e->age=0;vk_game_enter_stage(&item_game,stage);actor_sound=0;}
void loot_actor_step(int sx,int sy,int state,int guard,int invuln,int kill){int i,j,hit;VKWhip whip={0};v.simonX=sx;v.simonY=sy;v.simonState=state;item_game.player.x=sx;item_game.player.y=sy;item_game.player.mode=state==1?1:0;
 s.guardTimer=guard;s.invuln=invuln;s.killTimer=kill;Enemies_god=0;actor_sound=0;
 for(i=0;i<2;i++){
  Enemies_frame(&s,&t,&v);
  if(Enemies_lootPick){int kind=Enemies_lootPick;vk_candles_give(&c,kind);actor_sound=vk_candles_sound(kind);Enemies_lootPick=0;}
  for(j=0;j<Enemies_dropCount;j++)vk_candles_loot_rest(&c,Enemies_drops[j].kind,Enemies_drops[j].x,Enemies_drops[j].y,s.tick);
  Enemies_dropCount=0;vk_candles_sync(s.tick,(VKByte)!s.itemPhase);if(vk_candles_tick(&c,&item_game,&whip,&hit)&(VK_CANDLE_EVENT_PICK|VK_CANDLE_EVENT_HEARTS|VK_CANDLE_EVENT_ITEM))actor_sound=vk_candles_sound(c.picked);
 }
}
int loot_actor_record(unsigned char *raw,int *o){int i;memcpy(raw,enemyLoot[0].raw,32);o[0]=c.hearts;o[1]=actor_sound;o[2]=s.enemy[0].active;o[3]=enemyLoot[0].colour;
 for(i=0;i<8;i++){VKDrop *d=&c.drop[i];o[4+i*5]=d->state!=0;o[5+i*5]=d->x;o[6+i*5]=d->y;o[7+i*5]=d->kind;o[8+i*5]=d->timer;}
 return s.enemy[0].type;
}
void loot_actor_sprites(int *out){EnemySprite sat[8];int n=Enemies_sprites(&s,0,sat),i;out[0]=n;for(i=0;i<n;i++){out[1+3*i]=sat[i].y-1;out[2+3*i]=sat[i].x;out[3+3*i]=sat[i].pattern;}}
void loot_capacity_begin(const unsigned char *map){int i;item_init(12,0,0);v.map=map;v.simonX=0;v.simonY=0;for(i=0;i<7;i++){Enemy *e=newObject(&s,ENEMY_TYPE_LOOT_HEART);e->x=24+32*i;e->y=64;lootSpawn(&s,e,36,1);e->age=0;}}
int loot_capacity_probe(void){Enemy *e=newObject(&s,ENEMY_TYPE_A);if(!e)return 0;e->active=0;return 1;}
int loot_capacity_step(void){int i,n=0;Enemies_frame(&s,&t,&v);Enemies_frame(&s,&t,&v);for(i=0;i<7;i++)if(s.enemy[i].active)n++;return n;}

/* LOT13 connected candle facade; standalone historical tests leave adapter off. */
static int candle_pool(VKByte kind,VKByte x,VKByte y){return Enemies_candleLoot(&s,kind,x,y);}
void candle_begin(const unsigned char *map,int stage,int room,int weapon){
 item_init(stage,room,0);if(stage)vk_game_enter_stage(&item_game,stage);else vk_game_begin(&item_game);item_game.room=room;
 c.stage=255;vk_candles_enter(&c,&item_game);memset(c.drop,0,sizeof c.drop);c.weapon=weapon;c.clock=0;
 v.map=map;v.simonX=0;v.simonY=32;s.frozen=1;s.spawnPause=255;Enemies_god=0;
 memset(enemy_metadata,0,sizeof enemy_metadata);memset(enemyLoot,0,sizeof enemyLoot);
 vk_candles_airborne(candle_pool);actor_sound=0;
}
int candle_hit(int index){return vk_candles_hit(&c,index);}
int candle_direct(int kind,int x,int y){return Enemies_candleLoot(&s,kind,x,y);}
void candle_record(int slot,unsigned char *out,int *extra){int i;item_enemy_prefix(slot,out);extra[0]=c.lit;extra[1]=VK_CANDLE_OUT(&c,c.stage,c.room);extra[2]=0;for(i=0;i<8;i++)if(c.drop[i].state)extra[2]++;extra[3]=s.enemy[slot].type;extra[4]=c.hearts;extra[5]=actor_sound;}
void candle_step(int sx,int sy,int state,int guard,int invuln,int attack){int i,j,hit,events;VKWhip whip={0};
 v.simonX=sx;v.simonY=sy;v.simonState=state;item_game.player.x=sx;item_game.player.y=sy;item_game.player.mode=state==1?1:0;item_game.player.crouched=state==2;
 s.guardTimer=guard;s.invuln=invuln;v.facingLeft=0;whip.timer=attack;actor_sound=0;
 for(i=0;i<2;i++){
  s.spawnPause=255;Enemies_frame(&s,&t,&v);
  if(Enemies_lootPick){int kind=Enemies_lootPick;int ev=vk_candles_give(&c,kind);if(ev&VK_CANDLE_EVENT_PICK)Enemies_item(&s,kind,c.c431);actor_sound=vk_candles_sound(kind);Enemies_lootPick=0;}
  for(j=0;j<Enemies_dropCount;j++)vk_candles_loot_rest(&c,Enemies_drops[j].kind,Enemies_drops[j].x,Enemies_drops[j].y,s.tick);
  Enemies_dropCount=0;
  vk_candles_sync(s.tick,(VKByte)!s.itemPhase);events=vk_candles_tick(&c,&item_game,&whip,&hit);if(events&VK_CANDLE_EVENT_PICK)Enemies_item(&s,c.picked,c.c431);if(events&(VK_CANDLE_EVENT_PICK|VK_CANDLE_EVENT_HEARTS|VK_CANDLE_EVENT_ITEM|VK_CANDLE_EVENT_BOX_ITEM|VK_CANDLE_EVENT_WEAPON|VK_CANDLE_EVENT_GOLD_KEY))actor_sound=vk_candles_sound(c.picked);
 }
}
void candle_end(void){vk_candles_airborne(0);}

void candle_busy(int slots){int i;for(i=0;i<slots;i++){Enemy *e=newObject(&s,ENEMY_TYPE_DEATH);e->x=24+24*i;e->y=64;e->anim=255;e->pose=0x85;e->state=0;e->age=1;e->extraPhase=1;}}
void candle_free(int slot){s.enemy[slot].active=0;enemyLoot[slot].raw[0]=enemyLoot[slot].raw[14]=0;}
void candle_fallback(unsigned char *out){const VKByte *p=vk_candles_fallback_record(&c,0);if(p)memcpy(out,p,16);else memset(out,0,16);}

int candle_fallback_sprite(unsigned char *out){return vk_candles_fallback_sprite(&c,0,out);}

void candle_drop(int kind,int x,int y){vk_candles_drop(&c,x,y,kind);}
void candle_sat_busy(void){Enemy *e=newObject(&s,ENEMY_TYPE_DRAGON);e->x=24;e->y=64;e->age=1;e=newObject(&s,ENEMY_TYPE_HOPPER);if(e){e->x=88;e->y=64;e->age=1;}e=newObject(&s,ENEMY_TYPE_DEATH);if(e){e->x=144;e->y=64;e->age=1;e->anim=255;e->pose=0x85;e->extraPhase=1;}}
void candle_clock(int clock){s.tick=(VKByte)clock;}
void candle_fill_floor(void){int i;for(i=0;i<8;i++){VKDrop *d=&c.drop[i];memset(d,0,sizeof *d);d->state=VK_DROP_REST;d->x=24*i;d->y=176;d->kind=23;d->timer=255;}}
