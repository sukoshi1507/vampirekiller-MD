#include "platforms.h"

/* Table 906C: stage, room, Y, X, step, length (stage 5: rooms 1 and 4; stage 10: rooms 0, 2, 3 and 4, read in the ROM). */
static const struct { VKByte stage, room, y, x; signed char dx; VKByte length; } vk_platform_defs[] = {
    {  5, 1, 0x5F, 0x60,  1, 0x40 },
    {  5, 4, 0x5F, 0x20,  1, 0x30 },
    {  5, 4, 0x5F, 0xB8, -1, 0x38 },
    { 10, 0, 0x8F, 0x60,  1, 0x60 },
    { 10, 2, 0xA7, 0x40,  1, 0x80 },
    { 10, 3, 0x8F, 0x20,  1, 0xA0 },
    { 10, 4, 0xA7, 0x80,  1, 0x40 },
};

void vk_platforms_room(VKPlatforms *p, VKByte stage, VKByte room)
{
    unsigned i;
    int n = 0;
    for (i = 0; i < VK_PLATFORM_MAX; i++)
    {
        p->slot[i].id = 0;
        p->slot[i].y = p->slot[i].x = p->slot[i].length = p->slot[i].pass = p->slot[i].count = 0;
        p->slot[i].dx = 0;
    }
    p->under = 0;
    if (stage != 5 && stage != 10) return;
    for (i = 0; i < sizeof vk_platform_defs / sizeof vk_platform_defs[0]; i++)
    {
        if (vk_platform_defs[i].stage != stage || vk_platform_defs[i].room != room) continue;
        p->slot[n].id = (VKByte)(n + 1);
        p->slot[n].y = vk_platform_defs[i].y;
        p->slot[n].x = vk_platform_defs[i].x;
        p->slot[n].dx = vk_platform_defs[i].dx;
        p->slot[n].length = vk_platform_defs[i].length;
        n++;
    }
}

void vk_platforms_step(VKPlatforms *p)
{
    int i;
    for (i = 0; i < VK_PLATFORM_MAX; i++)
    {
        VKPlatform *s = &p->slot[i];
        if (!s->id) continue;
        s->pass++;
        if (++s->count == s->length)
        {
            s->count = 0;
            s->dx = (signed char)-s->dx;   /* this pass does not move */
        }
        else s->x = (VKByte)(s->x + s->dx);
    }
}

void vk_platforms_probe(VKPlatforms *p, VKByte x, VKByte y, int falling, VKByte fall_phase)
{
    VKByte under = 0;
    int i;
    if (falling && fall_phase < 3) return;   /* 8524: C420 = 4 and C428 < 3 */
    for (i = 0; i < VK_PLATFORM_MAX; i++)
    {
        const VKPlatform *s = &p->slot[i];
        if (!s->id || (VKByte)(y - s->y) >= 8) continue;
        if ((VKByte)(x - 7 - s->x) < 0x20 || (VKByte)(x + 7 - s->x) < 0x20) under |= s->id;
    }
    p->under = under;
}

int vk_platforms_carry(const VKPlatforms *p)
{
    if (!p->under) return 0;
    return p->slot[p->under == 1 ? 0 : 1].dx < 0 ? -1 : 1;   /* 6BAF, 6BBF: the sign of +3 */
}
