/* Full 768-byte collision maps verified against original MSX2 routine 4F83.
 * Map-byte fidelity does not imply complete gameplay or enemy validation. */
#ifndef VK_STAGE4_MAPS_H
#define VK_STAGE4_MAPS_H
#include "../room_manager.h"
#define VK_STAGE4_ROOM_COUNT 6
extern const VKByte vk_stage4_collision[VK_STAGE4_ROOM_COUNT][768];
extern const VKRoomLink vk_stage4_links[VK_STAGE4_ROOM_COUNT];
#endif
