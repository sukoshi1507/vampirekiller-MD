#include "targets.h"
void vk_target_init(VKTarget *t, int x, int y, int width, int height)
{
    t->x = x; t->y = y; t->width = width; t->height = height;
    t->active = 1; t->hit_latched = 0;
}
int vk_target_tick(VKTarget *t, const VKWhip *w)
{
    int l, top, r, bottom;
    if (!t->active || !vk_whip_active(w)) return 0;
    if (t->hit_latched) return 0;
    vk_whip_bounds(w, &l, &top, &r, &bottom);
    if (r < t->x || l >= t->x + t->width ||
        bottom < t->y || top >= t->y + t->height) return 0;
    t->hit_latched = 1;
    t->active = 0;
    return 1;
}
int vk_targets_tick(VKTarget *targets, int count, const VKWhip *whip)
{
    int i, hit = -1;
    for (i = 0; i < count; i++)
    {
        if (vk_target_tick(&targets[i], whip) && hit < 0) hit = i;
    }
    return hit;
}
