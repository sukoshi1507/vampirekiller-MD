#include "item_effects.h"
#include "generated/item_data.inc"
VKByte vk_item_outline_pixel(VKByte mode,VKByte x,VKByte y)
{
    const int size=mode==2?16:32;
    return mode>=2 && x<size && y<size && (!x || !y || x==size-1 || y==size-1)?14:0;
}
int vk_item_block_present(const VKCandles *c,const VKWall *w,VKByte stage,VKByte room,int i)
{
    if(stage>=19 || room>=12 || i<0 || i>=8 || !vk_item_blocks[stage][room][i].mode)return 0;
    (void)w;
    return room<VK_CANDLE_ROOM_LIMIT(stage) && !(VK_CANDLE_OUT(c,stage,room)&(1u<<i));
}

int vk_item_map_step(VKItemMapState *s,VKCandles *c,VKByte flags,VKByte boss,VKByte pressed)
{
    if(!(flags&64) || boss)return 0;
    if(s->phase==1){s->phase=2;return VK_ITEM_MAP_DRAW;}
    if(s->phase==2){if(!pressed)return 0;s->phase=0;return VK_ITEM_MAP_CLOSE;}
    if(!pressed || !vk_candles_map_use(c))return 0;
    s->phase=1;return VK_ITEM_MAP_OPEN;
}
