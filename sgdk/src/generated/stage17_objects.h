/* Original Z80 5A49/5B1B slot snapshot and validated field layout.
 * Stage 17 DATA-ONLY. Not connected to gameplay, enemies or level transitions. */
#ifndef VK_STAGE17_OBJECTS_H
#define VK_STAGE17_OBJECTS_H
#include "candles_md.h"
#define VK_STAGE17_ROOMS 12
#define VK_STAGE17_CANDLES 37
#define VK_STAGE17_FIXED 18
#define VK_STAGE17_WHITE_KEYS 1
#define VK_STAGE17_RAW 58
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage17RawSlot;
extern const VKCandleDef vk_stage17_candles[VK_STAGE17_CANDLES];
extern const VKCandleDef vk_stage17_fixed[VK_STAGE17_FIXED];
extern const VKCandleDef vk_stage17_white_keys[VK_STAGE17_WHITE_KEYS];
extern const VKStage17RawSlot vk_stage17_raw[VK_STAGE17_RAW];
extern const VKByte vk_stage17_candle_first[VK_STAGE17_ROOMS+1];
extern const VKByte vk_stage17_fixed_first[VK_STAGE17_ROOMS+1];
extern const VKByte vk_stage17_white_key_first[VK_STAGE17_ROOMS+1];
extern const VKByte vk_stage17_raw_first[VK_STAGE17_ROOMS+1];
#endif
