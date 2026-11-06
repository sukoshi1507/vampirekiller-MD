/* Test archived 16-byte Z80 object slots. Not a gameplay test. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage12_object_records.h"
int main(void) {
    static const unsigned char counts[12]={1,3,4,4,3,4,0,4,4,4,3,2};
    unsigned i,r,candles=0,other=0,unclassified=0;
    assert(VK_STAGE12_RAW_OBJECT_COUNT==36);
    assert(vk_stage12_raw_object_start[0]==0);
    assert(vk_stage12_raw_object_start[12]==VK_STAGE12_RAW_OBJECT_COUNT);
    for(r=0;r<12;r++){
        assert(vk_stage12_raw_object_start[r+1]-vk_stage12_raw_object_start[r]==counts[r]);
        for(i=vk_stage12_raw_object_start[r];i<vk_stage12_raw_object_start[r+1];i++){
            const VKStage12RawObject *p=&vk_stage12_raw_objects[i];
            assert(p->slot>=0xC420&&p->slot<=0xC5F0&&(p->slot&15)==0);
            if(p->raw[0]==1) {assert(p->slot>=0xC470&&p->slot<=0xC4F0);candles++;}
            else if(p->raw[0]==131) {assert(p->slot>=0xC500);other++;}
            else {assert(r==4&&p->slot==0xC5B0&&p->raw[0]==0);unclassified++;}
        }
    }
    assert(candles==30&&other==5&&unclassified==1);
    /* Canary values from original Z80 records, including the non-object-like slot. */
    assert(vk_stage12_raw_objects[0].slot==0xC470);
    assert(vk_stage12_raw_objects[0].raw[5]==1);
    assert(vk_stage12_raw_objects[14].slot==0xC5B0);
    assert(vk_stage12_raw_objects[14].raw[5]==1);
    puts("Stage 12 Z80 raw objects: 30 candles, 5 type-83 slots, 1 unclassified slot: PASS");
    return 0;
}
