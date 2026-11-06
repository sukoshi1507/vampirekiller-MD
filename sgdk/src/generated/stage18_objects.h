/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage 18 DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE18_OBJECTS_H
#define VK_STAGE18_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE18_ROOMS 10
#define VK_STAGE18_CANDLES 27
#define VK_STAGE18_FIXED 12
#define VK_STAGE18_WHITE_KEYS 1
#define VK_STAGE18_RAW 42
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage18RawSlot;
extern const VKCandleDef vk_stage18_candles[VK_STAGE18_CANDLES];
extern const VKCandleDef vk_stage18_fixed[VK_STAGE18_FIXED];
extern const VKCandleDef vk_stage18_white_keys[VK_STAGE18_WHITE_KEYS];
extern const VKStage18RawSlot vk_stage18_raw[VK_STAGE18_RAW];
extern const VKByte vk_stage18_candle_first[VK_STAGE18_ROOMS+1];
extern const VKByte vk_stage18_fixed_first[VK_STAGE18_ROOMS+1];
extern const VKByte vk_stage18_white_key_first[VK_STAGE18_ROOMS+1];
extern const VKByte vk_stage18_raw_first[VK_STAGE18_ROOMS+1];
#endif
