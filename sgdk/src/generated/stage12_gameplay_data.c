/* Positions reproduced from existing historical 5A49/5B1B harness documentation.
 * NO claim about object identity, drop, activation or gameplay validation. */
#include "stage12_gameplay_data.h"
#include "stage12_foundations.h"
const VKStage12Position vk_stage12_candle_positions[VK_STAGE12_CANDLE_POSITION_COUNT] = {
    {48,144}, /* room 0 */
    {64,112}, /* room 1 */
    {144,128}, /* room 1 */
    {224,128}, /* room 1 */
    {32,128}, /* room 2 */
    {112,144}, /* room 2 */
    {192,144}, /* room 2 */
    {48,144}, /* room 3 */
    {128,144}, /* room 3 */
    {176,144}, /* room 3 */
    {224,144}, /* room 3 */
    {224,128}, /* room 4 */
    {192,96}, /* room 5 */
    {32,144}, /* room 5 */
    {128,144}, /* room 5 */
    {160,144}, /* room 5 */
    {160,112}, /* room 7 */
    {224,112}, /* room 7 */
    {16,144}, /* room 7 */
    {32,112}, /* room 8 */
    {144,112}, /* room 8 */
    {192,144}, /* room 8 */
    {32,112}, /* room 9 */
    {64,112}, /* room 9 */
    {112,112}, /* room 9 */
    {160,112}, /* room 9 */
    {192,128}, /* room 10 */
    {16,144}, /* room 10 */
    {96,144}, /* room 10 */
    {112,144}, /* room 11 */
};
const VKByte vk_stage12_candle_start[13] = {0,1,4,7,11,12,16,16,19,22,26,29,30};
const VKStage12Position vk_stage12_other_object_positions[VK_STAGE12_OTHER_OBJECT_POSITION_COUNT] = {
    {224,160}, /* room 2 */
    {96,128}, /* room 4 */
    {192,128}, /* room 7 */
    {224,160}, /* room 8 */
    {64,128}, /* room 11 */
};
const VKByte vk_stage12_other_object_start[13] = {0,0,0,1,1,2,2,2,3,4,4,4,5};
const VKByte *vk_stage12_collision_tiles(VKByte room)
{
    return room < VK_STAGE12_ROOM_COUNT ? vk_stage12_tiles[room] : 0;
}
