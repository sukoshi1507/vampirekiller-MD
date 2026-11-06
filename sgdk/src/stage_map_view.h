#ifndef VK_STAGE_MAP_VIEW_H
#define VK_STAGE_MAP_VIEW_H

#include <genesis.h>

/* The stage map on plane A; what it covers comes back at the close. */
void vk_stage_map_open(void);
void vk_stage_map_close(void);
bool vk_stage_map_is_open(void);

#endif
