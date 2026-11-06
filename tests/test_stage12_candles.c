/* Every stage 12 candle field must match the archived Z80 slot bytes. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage12_candles.h"
#include "stage12_object_view.h"
int main(void) {
    unsigned room, i, candle=0, block=0;
    assert(vk_stage12_candle_first[0]==0);
    assert(vk_stage12_candle_first[12]==VK_STAGE12_CANDLE_COUNT);
    for(room=0;room<12;room++) {
        unsigned j=vk_stage12_candle_first[room];
        unsigned end=vk_stage12_candle_first[room+1];
        assert(j<=end);
        for(i=0;i<vk_stage12_object_count(room);i++) {
            const VKStage12RawObject *raw=vk_stage12_object_at(room,i);
            const VKCandleDef *def;
            if(raw->raw[0]!=1)continue;
            assert(j<end);
            def=&vk_stage12_candle_defs[j++];
            assert(def->x==raw->raw[2]);
            assert(def->y==raw->raw[1]);
            assert(def->mode==raw->raw[4]);
            assert(def->kind==raw->raw[5]);
            assert(def->content==raw->raw[9]);
            if(def->mode==3)block++;
            candle++;
        }
        assert(j==end);
    }
    assert(candle==30 && block>0);
    puts("Stage 12 30 candle defs vs Z80 raw slots: PASS");
    return 0;
}
