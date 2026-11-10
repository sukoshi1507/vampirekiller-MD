#include <string.h>
#include "gameplay.h"
#include "room_manager.h"
#include "candles.h"
#include "items.h"
#include "item_effects.h"
static VKGame g;static VKCandles c;static VKItems items;
void late_init(int stage){memset(&g,0,sizeof g);vk_candles_begin(&c);vk_items_begin(&items);vk_game_enter_stage(&g,stage);}
void late_room(int room){g.room=room;vk_candles_enter(&c,&g);vk_items_enter_room(&items);}
int late_map(int stage,int room,unsigned char *out){const VKByte *p=vk_stage_room_map(stage,room);if(!p)return 0;memcpy(out,p,768);return 1;}
void late_info(int *out){int i;VKByte x=0,y=0;out[0]=c.count;out[1]=c.lit;out[2]=g.player.x;out[3]=g.player.y;out[4]=g.player.facing;out[5]=vk_items_key_place(&g,&x,&y);out[6]=x;out[7]=y;out[8]=vk_items_door_place(&g,&x,&y);out[9]=x;out[10]=y;out[11]=sizeof c;for(i=0;i<c.count;i++)memcpy(out+12+i*5,(int[5]){vk_candles_def(&c,i)->x,vk_candles_def(&c,i)->y,vk_candles_def(&c,i)->mode,vk_candles_def(&c,i)->kind,vk_candles_def(&c,i)->content},5*sizeof(int));}
int late_link(int stage,int room,int dir){const VKRoomLink *p=vk_stage_links(stage);return ((const VKByte*)&p[room])[dir];}
int late_hit(int i){return vk_candles_hit(&c,i);}
int late_out(void){return VK_CANDLE_OUT(&c,g.stage,g.room);}
int late_present(int i){return vk_item_block_present(&c,0,g.stage,g.room,i);}
void late_wait(void){VKWhip whip={0};int hit;g.player.x=0;g.player.y=32;vk_candles_tick(&c,&g,&whip,&hit);vk_candles_tick(&c,&g,&whip,&hit);}
void late_reset(void){vk_candles_new_life(&c,g.stage);vk_game_respawn(&g);vk_items_new_life(&items);}
int late_fixed(int *out){int i,n=0;for(i=0;i<VK_DROP_MAX;i++)if(c.drop[i].state && c.drop[i].fixed){VKDrop *d=&c.drop[i];out[4*n]=d->x;out[4*n+1]=d->y;out[4*n+2]=d->kind;out[4*n+3]=d->content;n++;}return n;}
