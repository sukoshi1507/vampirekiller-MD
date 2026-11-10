#ifndef VK_ITEM_SHIELD_H
#define VK_ITEM_SHIELD_H
#include "generated/item_data.h"
/* Geometry864B; boxes from executed8264, included statically by the generator. */
static inline int vk_item_shield_touch(VKByte type,VKByte sx,VKByte sy,VKByte left,VKByte x,VKByte y)
{
    VKByte point,dx,dy,l,h;
    if(!type || type>=12)return 0;
    l=(VKByte)(vk_item_shield_boxes[type][0]+4);h=vk_item_shield_boxes[type][1];
    point=(VKByte)(sx+(left?-8:8));dx=point>=x?(VKByte)(point-x):(VKByte)(x-point);
    dy=(VKByte)(sy-y+h);return dx<l && dy<(VKByte)(32+h);
}
#endif
