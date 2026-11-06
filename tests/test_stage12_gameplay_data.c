/* Stage 12 test: decoded collision tiles and historical object positions.
 * Does NOT validate enemy, drop, collision semantics, or SGDK runtime.
 */
#include <assert.h>
#include <stdio.h>
#include "generated/stage12_foundations.h"
#include "generated/stage12_gameplay_data.h"
int main(void)
{
    static const VKByte candle_counts[12] = {1,3,3,4,1,4,0,3,3,4,3,1};
    static const VKByte other_counts[12]  = {0,0,1,0,1,0,0,1,1,0,0,1};
    unsigned r,i;
    assert(sizeof vk_stage12_tiles == 12u*768u);
    assert(vk_stage12_collision_tiles(12) == 0);
    assert(vk_stage12_collision_tiles(255) == 0);
    assert(vk_stage12_candle_start[0] == 0);
    assert(vk_stage12_other_object_start[0] == 0);
    assert(vk_stage12_candle_start[12] == VK_STAGE12_CANDLE_POSITION_COUNT);
    assert(vk_stage12_other_object_start[12] == VK_STAGE12_OTHER_OBJECT_POSITION_COUNT);
    for (r=0;r<12;r++) {
        const VKByte *data=vk_stage12_collision_tiles((VKByte)r);
        assert(data == vk_stage12_tiles[r]);
        assert(vk_stage12_candle_start[r+1]-vk_stage12_candle_start[r]==candle_counts[r]);
        assert(vk_stage12_other_object_start[r+1]-vk_stage12_other_object_start[r]==other_counts[r]);
        for(i=0;i<768;i++) assert(data[i]<192);
        for(i=vk_stage12_candle_start[r];i<vk_stage12_candle_start[r+1];i++) {
            assert(vk_stage12_candle_positions[i].x%16==0);
            assert(vk_stage12_candle_positions[i].y%16==0);
        }
        for(i=vk_stage12_other_object_start[r];i<vk_stage12_other_object_start[r+1];i++) {
            assert(vk_stage12_other_object_positions[i].x%16==0);
            assert(vk_stage12_other_object_positions[i].y%16==0);
        }
    }
    assert(vk_stage12_candle_positions[0].x==0x30 && vk_stage12_candle_positions[0].y==0x90);
    assert(vk_stage12_other_object_positions[0].x==0xE0 && vk_stage12_other_object_positions[0].y==0xA0);
    puts("Stage 12 gameplay DATA: 12 collision maps / 30 candle positions / 5 other positions: PASS");
    return 0;
}
