/* Z80 fields: X=+2, Y=+1, type=+4, chest content=+13.
 * All earlier 71 VKCandleDef fixed objects match. */
#include "stage12_fixed_objects.h"
const VKCandleDef vk_stage12_fixed_object_defs[VK_STAGE12_FIXED_OBJECT_COUNT] = {
    { 224, 160, 0, 0x19, 0x12 }, /* room 2 */
    { 96, 128, 0, 0x19, 0x11 }, /* room 4 */
    { 192, 128, 0, 0x19, 0x0F }, /* room 7 */
    { 224, 160, 0, 0x17, 0x00 }, /* room 8 */
    { 64, 128, 0, 0x19, 0x05 }, /* room 11 */
};
const VKByte vk_stage12_fixed_object_first[13] = {0, 0, 0, 1, 1, 2, 2, 2, 3, 4, 4, 4, 5};
