#include <assert.h>
#include <stdio.h>
#include "stage12_secret.h"
int main(void) {
    VKStage12SecretState s; VKByte dst=0xFF;
    unsigned i;
    vk_stage12_secret_reset(&s);
    /* MSX2 table: room 0: passage at X=88,Y=168, destination 3. */
    assert(vk_stage12_secret_begin(&s,0,172,88,1,0)==0);
    assert(vk_stage12_secret_begin(&s,0,172,88,0,1)==0);
    assert(vk_stage12_secret_begin(&s,0,171,88,1,1)==0); /* Y one pixel before lower bound */
    assert(vk_stage12_secret_begin(&s,0,172,87,1,1)==0); /* X one pixel before lower bound */
    assert(vk_stage12_secret_begin(&s,0,188,88,1,1)==0); /* dy 16 excluded */
    assert(vk_stage12_secret_begin(&s,0,172,104,1,1)==0); /* dx 16 excluded */
    assert(vk_stage12_secret_begin(&s,6,172,88,1,1)==0); /* room 6 has none */
    assert(vk_stage12_secret_begin(&s,0,172,88,1,1)==VK_STAGE12_SECRET_EFFECT);
    assert(s.active==1 && s.timer==64 && s.destination==3);
    assert(vk_stage12_secret_begin(&s,0,172,88,1,1)==0);
    for(i=0;i<63;i++) {assert(vk_stage12_secret_tick(&s,&dst)==0);assert(dst==0xFF);}
    assert(vk_stage12_secret_tick(&s,&dst)==1);
    assert(dst==3 && s.active==0 && s.timer==0);
    assert(vk_stage12_secret_tick(&s,&dst)==0);
    /* Check all ROM destinations at the proved lower-bound position. */
    for(i=0;i<VK_STAGE12_PASSAGE_COUNT;i++) {
        const VKStage12Passage *p=&vk_stage12_raw_passages[i];
        vk_stage12_secret_reset(&s);
        assert(vk_stage12_secret_begin(&s,p->source,(VKByte)(p->y+4),p->x,1,1)==VK_STAGE12_SECRET_EFFECT);
        assert(s.destination==p->destination);
    }
    puts("Stage 12 secret passage: 10 ROM entries, 16px bounds and 64-state countdown: PASS");
    return 0;
}
