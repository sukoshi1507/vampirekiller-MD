/* Generated from the reference ROM by extract_stage_exits.py. */
#ifndef VK_STAGE_EXITS_H
#define VK_STAGE_EXITS_H
#include "../player.h"
typedef struct { VKByte key_room,key_x,key_y,key_hidden,door_room,door_x,door_y; } VKStageItems;
static const VKStageItems kStageItems[10] = {
    { 2, 128, 160, 1, 7, 236, 48 }, /* stage 1 */
    { 5, 192, 112, 1, 1, 236, 48 }, /* stage 2 */
    { 2, 160, 160, 1, 4, 236, 128 }, /* stage 3 */
    { 0, 32, 64, 1, 3, 12, 48 }, /* stage 4 */
    { 3, 192, 64, 1, 5, 12, 80 }, /* stage 5 */
    { 0, 192, 160, 1, 4, 12, 80 }, /* stage 6 */
    { 3, 192, 128, 1, 6, 236, 96 }, /* stage 7 */
    { 7, 32, 64, 0, 7, 236, 64 }, /* stage 8 */
    { 8, 208, 96, 0, 8, 12, 128 }, /* stage 9 */
    { 0, 48, 48, 0, 8, 236, 128 }, /* stage 10 */
};
typedef struct { VKByte x,y,facing; } VKStageEntry;
static const VKStageEntry kStageEntries[10] = {
    { 16, 192, 0 },
    { 16, 96, 0 },
    { 16, 96, 0 },
    { 16, 192, 0 },
    { 232, 96, 1 },
    { 232, 128, 1 },
    { 232, 176, 1 },
    { 16, 144, 0 },
    { 16, 112, 0 },
    { 32, 144, 0 },
};
#endif
