#ifndef VK_ITEM_EFFECTS_H
#define VK_ITEM_EFFECTS_H
#include "generated/item_data.h"
#include "candles.h"
#include "wall.h"
/* 8264/864B, using raw D700 type;12 is a harmless flame, ignored by8610. */
#include "item_shield.h"
/* 870C: rectangular white outline, mode2=16px, mode>=3=32px. */
VKByte vk_item_outline_pixel(VKByte mode,VKByte x,VKByte y);
int vk_item_block_present(const VKCandles *c,const VKWall *w,VKByte stage,VKByte room,int i);
typedef struct { VKByte phase; } VKItemMapState;
#define VK_ITEM_MAP_OPEN 1
#define VK_ITEM_MAP_DRAW 2
#define VK_ITEM_MAP_CLOSE 4
int vk_item_map_step(VKItemMapState *s,VKCandles *c,VKByte flags,VKByte boss,VKByte pressed);
#endif
