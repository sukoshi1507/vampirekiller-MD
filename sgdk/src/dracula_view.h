#ifndef VK_DRACULA_VIEW_H
#define VK_DRACULA_VIEW_H
#include <genesis.h>
#include "dracula.h"
void vk_dracula_view_begin(VKDracula *d);
void vk_dracula_view_pass(const VKDracula *d);
void vk_dracula_view_reset(void);
u16 vk_dracula_view_sprites(const VKDracula *d,u16 first,u16 tail);
#endif
