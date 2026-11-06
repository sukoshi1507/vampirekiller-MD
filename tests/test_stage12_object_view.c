/* Exhaustive room bounds, slot integrity, and no inference from raw object bytes. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "stage12_object_view.h"
int main(void) {
    static const unsigned counts[12]={1,3,4,4,3,4,0,4,4,4,3,2};
    unsigned room,index,total=0,candles=0,other=0,unknown=0;
    assert(vk_stage12_object_count(12)==0);
    assert(vk_stage12_object_at(12,0)==NULL);
    assert(vk_stage12_object_at(255,0)==NULL);
    for(room=0;room<12;room++) {
        unsigned prevslot=0;
        assert(vk_stage12_object_count(room)==counts[room]);
        assert(vk_stage12_object_at(room, counts[room])==NULL);
        for(index=0;index<counts[room];index++) {
            const VKStage12RawObject *obj=vk_stage12_object_at(room,index);
            assert(obj!=NULL);
            assert(obj==&vk_stage12_raw_objects[total]);
            assert(obj->slot>prevslot);
            assert((obj->slot&15)==0);
            prevslot=obj->slot;
            if(obj->raw[0]==1) candles++;
            else if(obj->raw[0]==0x83) other++;
            else {
                assert(room==4 && obj->slot==0xC5B0);
                assert(obj->raw[0]==0 && obj->raw[5]==1);
                unknown++;
            }
            total++;
        }
    }
    assert(total==36 && candles==30 && other==5 && unknown==1);
    assert(vk_stage12_object_count(6)==0);
    assert(vk_stage12_object_at(6,0)==NULL);
    puts("Stage 12 object view: 12 rooms, 36 raw records, safe indexing: PASS");
    return 0;
}
