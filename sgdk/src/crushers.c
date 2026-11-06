#include "crushers.h"

/* 8FBD: state, Y, X, mask, pass counter, limit of the three records; the step counter starts at 0. */
static const VKCrusher vk_crusher_defs[VK_CRUSHER_MAX] = {
    { 1, 0x60, 0x3C, 0, 0, 0x0B, 0 },
    { 1, 0x60, 0x7C, 0, 1, 0x0A, 0 },
    { 1, 0x60, 0xBC, 1, 2, 0x0B, 0 },
};

void vk_crushers_room(VKCrushers *c, VKByte stage, VKByte room)
{
    unsigned i;
    c->room = (VKByte)(stage == 6 && room == 1);
    for (i = 0; i < VK_CRUSHER_MAX; i++) c->slot[i] = (VKCrusher){ 0, 0, 0, 0, 0, 0, 0 };
}

void vk_crushers_step(VKCrushers *c)
{
    unsigned i;
    if (!c->room) return;
    if (!c->slot[0].state)   /* 8F9A: RET NZ while the first record lives */
        for (i = 0; i < VK_CRUSHER_MAX; i++) c->slot[i] = vk_crusher_defs[i];
    for (i = 0; i < VK_CRUSHER_MAX; i++)
    {
        VKCrusher *const s = &c->slot[i];
        if (!s->state) continue;
        s->pass++;                                        /* 8FF1 */
        if (s->state == 1)
        {
            if (s->mask & s->pass) continue;              /* 8FF7: nothing this pass */
            s->y = (VKByte)(s->y + 4);
        }
        else
        {
            if (s->pass & 3) continue;                    /* 8FFE: one pass in four */
            s->y = (VKByte)(s->y - 4);
        }
        if (++s->count == s->limit)                       /* 9008 */
        {
            s->count = 0;
            s->state ^= 3;                                /* 1 <-> 2 */
        }
    }
}

int vk_crushers_hit(const VKCrushers *c, VKByte simon_x, VKByte simon_y)
{
    unsigned i;
    if (!c->room) return 0;
    for (i = 0; i < VK_CRUSHER_MAX; i++)
    {
        const VKCrusher *const s = &c->slot[i];
        if (!s->state) continue;
        /* 85DE: (C425 - 1Ch - Y) < 8 and (C427 - X) < 20h, both unsigned bytes. */
        if ((VKByte)(simon_y - 0x1C - s->y) < 8 && (VKByte)(simon_x - s->x) < 0x20)
            return (s->state & 1) ? 16 : 8;               /* 85CB: RRA, B = 10h when the state is 1 */
    }
    return 0;
}
