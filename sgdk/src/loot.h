#ifndef VK_LOOT_H
#define VK_LOOT_H
#include "player.h"
/* First32 IX bytes; other bytes retained by the ROM are inputs, not fabricated. */
typedef struct {VKByte raw[32],colour;} VKLoot;
/* Return1 when9C03/9C4E emits a C500 floor object;2 on99E5 screen exit. */
void vk_loot_init(VKLoot *l,VKByte type,VKByte x,VKByte y,VKByte kind);
int vk_loot_step(VKLoot *l,const VKByte *map,VKByte stage,VKByte clock,VKByte out[3]);
void vk_loot_advance(VKLoot *l);
void vk_loot_pick(VKLoot *l,VKByte *kind);
#endif
