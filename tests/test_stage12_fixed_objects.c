#include <assert.h>
#include <stdio.h>
#include "generated/stage12_fixed_objects.h"
#include "generated/stage12_object_records.h"

int main(void)
{
    static const unsigned counts[12] = {0,0,1,0,1,0,0,1,1,0,0,1};
    unsigned room, i, total = 0;
    for (room=0; room<12; room++) {
        unsigned first=vk_stage12_fixed_object_first[room];
        unsigned last=vk_stage12_fixed_object_first[room+1];
        unsigned raw_first=vk_stage12_raw_object_start[room];
        unsigned raw_last=vk_stage12_raw_object_start[room+1];
        unsigned found=0;
        assert(last>=first);
        assert(last-first==counts[room]);
        for (i=raw_first; i<raw_last; i++) {
            const VKStage12RawObject *raw=&vk_stage12_raw_objects[i];
            const VKCandleDef *d;
            if(raw->raw[0]!=0x83 || raw->raw[4]==0x18) continue;
            assert(found < counts[room]);
            d=&vk_stage12_fixed_object_defs[first+found++];
            assert(d->x==raw->raw[2] && d->y==raw->raw[1]);
            assert(d->mode==0 && d->kind==raw->raw[4]);
            assert(d->content==raw->raw[13]);
            assert(d->kind==0x17 || d->kind==0x19);
        }
        assert(found==counts[room]);
        total+=found;
    }
    assert(total==5 && vk_stage12_fixed_object_first[12]==5);
    /* The non-object slot from original Z80 (C5B0) must never be converted. */
    assert(vk_stage12_raw_objects[14].slot==0xC5B0);
    assert(vk_stage12_raw_objects[14].raw[0]==0);
    puts("test_stage12_fixed_objects: OK (5 records, 12 rooms)");
    return 0;
}
