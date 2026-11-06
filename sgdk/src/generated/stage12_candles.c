/* Literal Z80 candle fields: X=+2, Y=+1, mode=+4, kind=+5, content=+9.
 * Exact field mapping checked against all 275 existing stage 0..11 candles. */
#include "stage12_candles.h"
const VKCandleDef vk_stage12_candle_defs[VK_STAGE12_CANDLE_COUNT] = {
    { 48, 144, 0, 0x01, 0x00 }, /* room 0 */
    { 144, 128, 0, 0x01, 0x00 }, /* room 1 */
    { 224, 128, 0, 0x01, 0x00 }, /* room 1 */
    { 64, 112, 3, 0x17, 0x00 }, /* room 1 */
    { 32, 128, 0, 0x00, 0x00 }, /* room 2 */
    { 112, 144, 0, 0x02, 0x00 }, /* room 2 */
    { 192, 144, 0, 0x00, 0x00 }, /* room 2 */
    { 48, 144, 0, 0x01, 0x00 }, /* room 3 */
    { 128, 144, 0, 0x01, 0x00 }, /* room 3 */
    { 176, 144, 0, 0x00, 0x00 }, /* room 3 */
    { 224, 144, 0, 0x02, 0x00 }, /* room 3 */
    { 224, 128, 0, 0x01, 0x00 }, /* room 4 */
    { 128, 144, 0, 0x02, 0x00 }, /* room 5 */
    { 192, 96, 3, 0x18, 0x00 }, /* room 5 */
    { 32, 144, 3, 0x1F, 0x88 }, /* room 5 */
    { 160, 144, 3, 0x00, 0x00 }, /* room 5 */
    { 16, 144, 0, 0x0C, 0x00 }, /* room 7 */
    { 160, 112, 0, 0x15, 0x00 }, /* room 7 */
    { 224, 112, 0, 0x00, 0x00 }, /* room 7 */
    { 32, 112, 0, 0x01, 0x00 }, /* room 8 */
    { 144, 112, 0, 0x15, 0x00 }, /* room 8 */
    { 192, 144, 0, 0x02, 0x00 }, /* room 8 */
    { 32, 112, 0, 0x02, 0x00 }, /* room 9 */
    { 112, 112, 0, 0x01, 0x00 }, /* room 9 */
    { 160, 112, 0, 0x15, 0x00 }, /* room 9 */
    { 64, 112, 3, 0x17, 0x00 }, /* room 9 */
    { 16, 144, 0, 0x01, 0x00 }, /* room 10 */
    { 96, 144, 0, 0x08, 0x00 }, /* room 10 */
    { 192, 128, 0, 0x02, 0x00 }, /* room 10 */
    { 112, 144, 0, 0x00, 0x00 }, /* room 11 */
};
const VKByte vk_stage12_candle_first[13] = {0, 1, 4, 7, 11, 12, 16, 16, 19, 22, 26, 29, 30};
