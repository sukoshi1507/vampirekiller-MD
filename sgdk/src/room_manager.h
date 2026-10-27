#ifndef VK_ROOM_MANAGER_H
#define VK_ROOM_MANAGER_H
#include "player.h"
/* Stage-1 (hall) room graph from the ROM link table (research/room-links.md).
 * Left/right are proven by openMSX traces; up/down come from the table only. */
typedef struct {
    VKByte up, down, left, right;
    VKByte verified_collision;
} VKRoomLink;
#define VK_ROOM_NONE 255
#define VK_STAGE1_ROOM_COUNT 8
extern const VKRoomLink vk_stage1_links[VK_STAGE1_ROOM_COUNT];
/* Links and room count of a castle stage (1 or 2, generated/stage2_maps.c);
 * NULL and 0 for the other stages. */
const VKRoomLink *vk_stage_links(VKByte stage);
VKByte vk_stage_room_count(VKByte stage);
/* Returns 1 only when crossing a configured boundary. Preserves vertical state,
 * animation timing, and facing. The caller owns rendering and room palettes. */
int vk_room_try_edge(VKByte stage, VKByte *room, VKPlayer *player, int direction);
#endif
