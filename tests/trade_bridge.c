#include <string.h>
#include "wall.h"
static unsigned char random_value;
unsigned char Enemies_randomR(void){return random_value;}
#include "../sgdk/src/wall.c"
const VKByte vk_courtyard_maps[3][768]={{0}};
const VKByte vk_courtyard_neighbors[3][4]={{0}};
static VKWall w;
static VKCandles c;
static VKWallEvents ev;
static unsigned short flag_pointer[2];
static unsigned char selector[2];
void trade_init(int stage,int room){vk_candles_begin(&c);vk_wall_begin(&w);c.stage=stage;c.room=room;vk_wall_enter(&w,stage,room);memset(flag_pointer,0,sizeof flag_pointer);memset(selector,0,sizeof selector);}
void trade_metadata(int slot,int pointer,int code){flag_pointer[slot]=pointer;selector[slot]=(code&60)>>2;}
void trade_reenter(void){vk_wall_enter(&w,w.stage,w.room);memset(flag_pointer,0,sizeof flag_pointer);memset(selector,0,sizeof selector);}
int trade_block(int index,int x,int y,int code){VKCandleDef def={0};def.x=x;def.y=y;def.mode=3;def.kind=31;def.content=code;return vk_wall_block(&w,index,&def);}
void trade_hit(int slot){vk_wall_hit(&w,slot);}
void trade_cross(void){vk_wall_cross(&w);}
void trade_pass(int r){random_value=r;w.phase=1;vk_wall_step(&w,&c,&ev);}
void trade_record(int slot,unsigned char *out){VKWallObject *o=&w.obj[slot];memset(out,0,16);out[0]=o->state;out[1]=o->y;out[2]=o->x;out[3]=o->hits;out[4]=o->flag?selector[slot]:0;out[5]=o->row;out[7]=o->flag?(unsigned char)(flag_pointer[slot]>>8):0;out[8]=o->flag?(unsigned char)flag_pointer[slot]:0;out[9]=o->flag?(unsigned char)slot:0;out[10]=o->timer;}
void trade_values(int *out){out[0]=w.c70c;out[1]=w.c70b;out[2]=c.hearts;out[3]=w.d012;out[4]=*count_of(&w,0);out[5]=*count_of(&w,1);out[6]=w.shop;out[7]=w.sale;out[8]=w.price;out[9]=w.timeout;out[10]=w.delay;out[11]=w.keys;out[12]=w.bought;out[13]=vk_wall_frame(&w,0);out[14]=vk_wall_frame(&w,1);}
int trade_sounds(unsigned char *out){memcpy(out,ev.sound,ev.sounds);return ev.sounds;}
int trade_points(void){return ev.points*5000;}
void trade_inventory(int hearts,int books){c.hearts=hearts;c.c702=books;}
void trade_clock(int clock){w.c003=clock;}
int trade_shop(int keys){return vk_shop_step(&w,&c,keys,&ev);}
void trade_given(int *out){out[0]=c.picked;out[1]=c.weapon;out[2]=c.c701;out[3]=c.c431;out[4]=c.c702;out[5]=c.gold_keys;out[6]=c.triple_key;}
int trade_present(int index){return vk_wall_block_present(&w,w.stage,w.room,index);}

void trade_reset(void){vk_wall_reset_stage(&w,w.stage);}
void trade_window(unsigned char *out){vk_shop_window(&w,out,out+1);}
int trade_whip(int x,int y,int facing,int crouched,int weapon){VKWhip whip={0};whip.timer=VK_WHIP_STRIKE_TIMER;whip.x=x;whip.y=y;whip.facing=facing;whip.crouched=crouched;whip.level=weapon;return vk_wall_whip_touch(&w,0,&whip,weapon);}
