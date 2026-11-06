/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage 14 DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE14_OBJECTS_H
#define VK_STAGE14_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE14_ROOMS 8
#define VK_STAGE14_CANDLES 22
#define VK_STAGE14_FIXED 10
#define VK_STAGE14_RAW 33
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage14RawSlot;
extern const VKCandleDef vk_stage14_candles[VK_STAGE14_CANDLES];
extern const VKCandleDef vk_stage14_fixed[VK_STAGE14_FIXED];
extern const VKStage14RawSlot vk_stage14_raw[VK_STAGE14_RAW];
extern const VKByte vk_stage14_candle_first[VK_STAGE14_ROOMS+1];
extern const VKByte vk_stage14_fixed_first[VK_STAGE14_ROOMS+1];
extern const VKByte vk_stage14_raw_first[VK_STAGE14_ROOMS+1];
#endif
