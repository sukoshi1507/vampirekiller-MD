/* ROM dispatch table at CPU 9942, mapper bank 02 in window 8000.
 * These addresses identify updates only; they do not port the routines. */
#ifndef VK_LATE_ENEMY_ROUTINES_H
#define VK_LATE_ENEMY_ROUTINES_H
static const unsigned short vk_late_update_address[6] = {0xAF5F,0xB37E,0xB248,0xA9B7,0xADF8,0xBE70};
static const unsigned char vk_late_update_type[6] = {0x09,0x0B,0x0D,0x0E,0x10,0x12};
#endif
