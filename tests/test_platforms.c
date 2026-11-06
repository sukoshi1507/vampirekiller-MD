/* Moving platforms: routines 902D, 90B8, 8524 and 6BAF (research/stage5-enemies.md). */
#include <assert.h>
#include "platforms.h"

int main(void)
{
    VKPlatforms p;
    int i;
    vk_platforms_room(&p, 4, 1);
    assert(!p.slot[0].id && !p.slot[1].id);
    vk_platforms_room(&p, 5, 0);
    assert(!p.slot[0].id);

    vk_platforms_room(&p, 5, 1);
    assert(p.slot[0].id == 1 && p.slot[0].x == 0x60 && p.slot[0].y == 0x5F && !p.slot[1].id);
    for (i = 0; i < 63; i++) vk_platforms_step(&p);
    assert(p.slot[0].x == 159 && p.slot[0].dx == 1);
    vk_platforms_step(&p);          /* the turn: no move on this pass */
    assert(p.slot[0].x == 159 && p.slot[0].dx == -1);
    for (i = 0; i < 63; i++) vk_platforms_step(&p);
    assert(p.slot[0].x == 96);
    vk_platforms_step(&p);
    assert(p.slot[0].x == 96 && p.slot[0].dx == 1);

    vk_platforms_probe(&p, 120, 96, 0, 0);
    assert(p.under == 1 && vk_platforms_carry(&p) == 1);
    vk_platforms_probe(&p, 96 - 7, 96, 0, 0);       /* X + 7 - platform X < 20h */
    assert(p.under == 1);
    vk_platforms_probe(&p, 96 - 8, 96, 0, 0);
    assert(p.under == 0);
    vk_platforms_probe(&p, 96 + 0x26, 96, 0, 0);    /* X - 7 - platform X < 20h */
    assert(p.under == 1);
    vk_platforms_probe(&p, 96 + 0x27, 96, 0, 0);
    assert(p.under == 0);
    vk_platforms_probe(&p, 120, 0x5E, 0, 0);
    assert(p.under == 0);
    vk_platforms_probe(&p, 120, 0x67, 0, 0);
    assert(p.under == 0);
    assert(vk_platforms_carry(&p) == 0);
    vk_platforms_probe(&p, 120, 0x60, 1, 2);        /* falling, C428 < 3: value kept */
    assert(p.under == 0);
    vk_platforms_probe(&p, 120, 0x60, 1, 3);
    assert(p.under == 1);

    vk_platforms_room(&p, 5, 4);
    assert(p.slot[0].id == 1 && p.slot[1].id == 2 && p.slot[1].x == 0xB8 && p.slot[1].dx == -1);
    vk_platforms_probe(&p, 0xC0, 0x60, 0, 0);
    assert(p.under == 2 && vk_platforms_carry(&p) == -1);
    return 0;
}
