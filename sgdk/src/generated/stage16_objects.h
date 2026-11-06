/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage 16 DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE16_OBJECTS_H
#define VK_STAGE16_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE16_ROOMS 10
#define VK_STAGE16_CANDLES 10
#define VK_STAGE16_FIXED 5
#define VK_STAGE16_WHITE_KEYS 3
#define VK_STAGE16_RAW 18
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage16RawSlot;
extern const VKCandleDef vk_stage16_candles[VK_STAGE16_CANDLES];
extern const VKCandleDef vk_stage16_fixed[VK_STAGE16_FIXED];
extern const VKCandleDef vk_stage16_white_keys[VK_STAGE16_WHITE_KEYS];
extern const VKStage16RawSlot vk_stage16_raw[VK_STAGE16_RAW];
extern const VKByte vk_stage16_candle_first[VK_STAGE16_ROOMS+1];
extern const VKByte vk_stage16_fixed_first[VK_STAGE16_ROOMS+1];
extern const VKByte vk_stage16_white_key_first[VK_STAGE16_ROOMS+1];
extern const VKByte vk_stage16_raw_first[VK_STAGE16_ROOMS+1];
#endif
