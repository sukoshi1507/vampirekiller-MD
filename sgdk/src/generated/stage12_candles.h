/* Stage 12 candle definitions from archived Z80 execution 5A49/5B1B.
 * Byte offsets validated against 275 preceding definitions (stages 0..11).
 * NO stage 12 gameplay hook; non-candle slots are not represented here. */
#ifndef VK_STAGE12_CANDLES_H
#define VK_STAGE12_CANDLES_H
#include "candles_md.h"
#define VK_STAGE12_CANDLE_COUNT 30
extern const VKCandleDef vk_stage12_candle_defs[VK_STAGE12_CANDLE_COUNT];
extern const VKByte vk_stage12_candle_first[13];
#endif
