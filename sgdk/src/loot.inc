#include "loot.h"
#include "generated/loot_data.inc"
static unsigned word(const VKByte *p){return p[0]|((unsigned)p[1]<<8);}
static void put(VKByte *p,unsigned v){p[0]=(VKByte)v;p[1]=(VKByte)(v>>8);}
void vk_loot_init(VKLoot *l,VKByte type,VKByte x,VKByte y,VKByte kind)
{
 int i,k=type==30?2:type==38;for(i=0;i<32;i++)if(loot_write[k][i])l->raw[i]=loot_init[k][i];
 l->raw[5]=x;l->raw[3]=y;if(type!=36)l->raw[31]=kind;l->colour=type==38?14:8;
}
void vk_loot_pick(VKLoot *l,VKByte *kind){*kind=l->raw[0]==36?1:l->raw[31];l->raw[0]=l->raw[14]=0;}
int vk_loot_step(VKLoot *l,const VKByte *map,VKByte stage,VKByte clock,VKByte out[3])
{
 VKByte *b=l->raw;int landed=0;
 if(!b[0])return 0;
 if(b[0]==36){
  landed=map && vk_grounded(map,(VKByte)(b[5]-5),b[3],stage,0);
  if(!landed){
   b[12]++;put(b+9,word(b+9)+word(b+16));
   if(b[12]==32){b[12]=0;put(b+16,0-word(b+16));}
  }
 }else{
  static const VKByte colours[4]={8,1,14,1};l->colour=colours[clock&3];
  if(!b[1]){if(--b[12]==0){b[1]++;put(b+9,0);put(b+7,0x800);}}
  else{b[6]=1;landed=map && vk_grounded(map,(VKByte)(b[5]-5),b[3],stage,0);}
 }
 if(landed){out[0]=b[0]==36?1:b[31];out[1]=(VKByte)((b[5]-(b[0]==36?0:4))&0xF8);out[2]=(VKByte)((b[3]-16)&0xF8);b[0]=b[14]=0;}
 vk_loot_advance(l);
 if(!landed && !b[0])return 2;
 return landed?1:0;
}

void vk_loot_advance(VKLoot *l){VKByte *b=l->raw;unsigned v;
 if(b[6]){v=word(b+2)+word(b+7);put(b+2,v);v=word(b+4)+word(b+9);put(b+4,v);}
 if(b[3]>=0xE4 || b[5]>=0xF1 || b[5]<7)b[0]=b[14]=0;
}
