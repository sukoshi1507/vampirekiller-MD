/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage 13 DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE13_OBJECTS_H
#define VK_STAGE13_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE13_ROOMS 12
#define VK_STAGE13_CANDLES 34
#define VK_STAGE13_FIXED 7
#define VK_STAGE13_RAW 42
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage13RawSlot;
extern const VKCandleDef vk_stage13_candles[VK_STAGE13_CANDLES];
extern const VKCandleDef vk_stage13_fixed[VK_STAGE13_FIXED];
extern const VKStage13RawSlot vk_stage13_raw[VK_STAGE13_RAW];
extern const VKByte vk_stage13_candle_first[VK_STAGE13_ROOMS+1];
extern const VKByte vk_stage13_fixed_first[VK_STAGE13_ROOMS+1];
extern const VKByte vk_stage13_raw_first[VK_STAGE13_ROOMS+1];
#endif
