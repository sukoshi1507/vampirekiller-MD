#include <string.h>
#include "loot.h"
#include "../sgdk/src/loot.inc"
static VKLoot l;static unsigned char mapdata[768];static VKByte stage,event[3];static int emitted;
void loot_begin(const unsigned char *retained,const unsigned char *map,int s,int type,int x,int y,int kind){memcpy(l.raw,retained,32);memcpy(mapdata,map,768);stage=s;emitted=0;vk_loot_init(&l,type,x,y,kind);}
void loot_step(int clock){emitted=vk_loot_step(&l,mapdata,stage,clock,event);}
int loot_record(unsigned char *out,int *rest){memcpy(out,l.raw,32);rest[0]=emitted;rest[1]=event[0];rest[2]=event[1];rest[3]=event[2];rest[4]=l.colour;return l.raw[0];}
