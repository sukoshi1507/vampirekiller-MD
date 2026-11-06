/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage 15 DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE15_OBJECTS_H
#define VK_STAGE15_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE15_ROOMS 10
#define VK_STAGE15_CANDLES 22
#define VK_STAGE15_FIXED 10
#define VK_STAGE15_WHITE_KEYS 1
#define VK_STAGE15_RAW 33
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage15RawSlot;
extern const VKCandleDef vk_stage15_candles[VK_STAGE15_CANDLES];
extern const VKCandleDef vk_stage15_fixed[VK_STAGE15_FIXED];
extern const VKCandleDef vk_stage15_white_keys[VK_STAGE15_WHITE_KEYS];
extern const VKStage15RawSlot vk_stage15_raw[VK_STAGE15_RAW];
extern const VKByte vk_stage15_candle_first[VK_STAGE15_ROOMS+1];
extern const VKByte vk_stage15_fixed_first[VK_STAGE15_ROOMS+1];
extern const VKByte vk_stage15_white_key_first[VK_STAGE15_ROOMS+1];
extern const VKByte vk_stage15_raw_first[VK_STAGE15_ROOMS+1];
#endif
