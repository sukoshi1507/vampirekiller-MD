/* Full 768-byte collision maps verified against original MSX2 routine 4F83.
 * Map-byte fidelity does not imply complete gameplay or enemy validation. */
#ifndef VK_STAGE9_MAPS_H
#define VK_STAGE9_MAPS_H
#include "../room_manager.h"
#define VK_STAGE9_ROOM_COUNT 9
extern const VKByte vk_stage9_collision[VK_STAGE9_ROOM_COUNT][768];
extern const VKRoomLink vk_stage9_links[VK_STAGE9_ROOM_COUNT];
#endif
