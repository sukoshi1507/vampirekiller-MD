#ifndef VK_ROOM_MANAGER_H
#define VK_ROOM_MANAGER_H
#include "player.h"
/* Stage-1 (hall) left/right graph: proven by openMSX traces. No vertical links. */
typedef struct {
    VKByte left, right;
    VKByte verified_collision;
} VKRoomLink;
#define VK_ROOM_NONE 255
#define VK_STAGE1_ROOM_COUNT 8
extern const VKRoomLink vk_stage1_links[VK_STAGE1_ROOM_COUNT];
/* Returns 1 only when crossing a configured boundary. Preserves vertical state,
 * animation timing, and facing. The caller owns rendering and room palettes. */
int vk_room_try_edge(VKByte *room, VKPlayer *player, int direction);
#endif
