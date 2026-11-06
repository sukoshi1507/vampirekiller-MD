/* Spiked crushers of stage 6 room 1: routines 8F9A, 8FEA and 85A6 (research/stage6-enemies.md). */
#include <assert.h>
#include "crushers.h"

int main(void)
{
    VKCrushers c;
    int i;
    vk_crushers_room(&c, 5, 1);
    vk_crushers_step(&c);
    assert(!c.slot[0].state);
    vk_crushers_room(&c, 6, 0);
    vk_crushers_step(&c);
    assert(!c.slot[0].state);

    /* openMSX trace, first pass of room 1: Y 64, 64, 60 (the third waits for an even pass counter). */
    vk_crushers_room(&c, 6, 1);
    vk_crushers_step(&c);
    assert(c.slot[0].y == 0x64 && c.slot[1].y == 0x64 && c.slot[2].y == 0x60);
    assert(c.slot[0].pass == 1 && c.slot[1].pass == 2 && c.slot[2].pass == 3);
    vk_crushers_step(&c);
    assert(c.slot[0].y == 0x68 && c.slot[1].y == 0x68 && c.slot[2].y == 0x64);
    /* The first one reaches 8Ch on its 11th step and turns; it then rises 4 px every 4th pass (trace: 88 at once). */
    for (i = 0; i < 9; i++) vk_crushers_step(&c);
    assert(c.slot[0].y == 0x8C && c.slot[0].state == 2);
    vk_crushers_step(&c);
    assert(c.slot[0].y == 0x88);
    vk_crushers_step(&c); vk_crushers_step(&c); vk_crushers_step(&c);
    assert(c.slot[0].y == 0x88);
    vk_crushers_step(&c);
    assert(c.slot[0].y == 0x84);
    /* Trace (R3), cycle of the first one: 41 passes up, 11 down, 41 up (flips at pass 52, 63, 104). */
    for (i = 0; i < 35; i++) vk_crushers_step(&c);
    assert(c.slot[0].state == 2 && c.slot[0].y == 0x64);
    vk_crushers_step(&c);
    assert(c.slot[0].state == 1 && c.slot[0].y == 0x60 && c.slot[0].pass == 52);
    for (i = 0; i < 11; i++) vk_crushers_step(&c);
    assert(c.slot[0].state == 2 && c.slot[0].y == 0x8C && c.slot[0].pass == 63);
    for (i = 0; i < 41; i++) vk_crushers_step(&c);
    assert(c.slot[0].state == 1 && c.slot[0].y == 0x60 && c.slot[0].pass == 104);

    /* Trace: Simon (70, 160) is hit by the first one going up at 84h (damage 8), by the third one going down
     * at 80h (damage 16, Simon at X 196). */
    c.slot[0] = (VKCrusher){ 2, 0x84, 0x3C, 0, 0, 0x0B, 3 };
    assert(vk_crushers_hit(&c, 70, 160) == 8);
    c.slot[0].y = 0x88;
    assert(vk_crushers_hit(&c, 70, 160) == 0);
    c.slot[0].y = 0x7C;
    assert(vk_crushers_hit(&c, 70, 160) == 0);
    c.slot[0].y = 0x84;
    assert(vk_crushers_hit(&c, 0x3C + 0x20, 160) == 0);
    c.slot[2] = (VKCrusher){ 1, 0x80, 0xBC, 1, 0, 0x0B, 3 };
    assert(vk_crushers_hit(&c, 196, 160) == 16);
    return 0;
}
