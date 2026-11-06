/* Full 768-byte collision maps verified against original MSX2 routine 4F83.
 * Map-byte fidelity does not imply complete gameplay or enemy validation. */
#ifndef VK_STAGE10_MAPS_H
#define VK_STAGE10_MAPS_H
#include "../room_manager.h"
#define VK_STAGE10_ROOM_COUNT 9
extern const VKByte vk_stage10_collision[VK_STAGE10_ROOM_COUNT][768];
extern const VKRoomLink vk_stage10_links[VK_STAGE10_ROOM_COUNT];
#endif
