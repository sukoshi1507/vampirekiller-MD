#include "whip.h"
void vk_whip_reset(VKWhip *w) { w->timer = w->facing = w->x = w->y = w->crouched = 0; }
int vk_whip_active(const VKWhip *w) { return w->timer >= 3 && w->timer <= 10; }
int vk_whip_phase(const VKWhip *w)
{
    if (!vk_whip_active(w)) return 0;
    return w->timer >= 8 ? 1 : 2;
}
int vk_whip_segments(const VKWhip *w)
{
    return vk_whip_phase(w) == 1 ? 2 : (vk_whip_phase(w) == 2 ? 3 : 0);
}
void vk_whip_tick(VKWhip *w, const VKPlayer *p, int attack_pressed)
{
    if (w->timer) w->timer--;
    else if (attack_pressed && !p->on_stairs) w->timer = 14;
    /* Latch attack direction and origin at startup; the whip cannot teleport
     * across Simon when facing changes during an active swing. */
    if (w->timer == 14) { w->facing = p->facing; w->x = p->x; w->y = p->y; w->crouched = p->crouched; }
}
void vk_whip_bounds(const VKWhip *w, int *left, int *top, int *right, int *bottom)
{
    int reach = w->timer >= 8 ? 16 : 24;
    *left = w->facing ? (int)w->x - reach - 8 : (int)w->x + 8;
    *right = w->facing ? (int)w->x - 8 : (int)w->x + reach + 8;
    *top = (int)w->y - (w->crouched ? 10 : 20);
    *bottom = (int)w->y - (w->crouched ? 7 : 17);
}

/* Shared 15-step timeline for both body styles. Indices represent the
 * current timer (14 is attack start, 0 is idle). Visual selections remain
 * experimental ROM-derived compositions; the gameplay hit windows are kept.
 * Each entry is {arm phase, standing frame, crouched frame}. */
typedef struct { VKByte phase, standing, crouched; } VKAttackKey;
static const VKAttackKey vk_attack_keys[15] = {
    {0,0,0}, {4,7,4}, {4,7,4}, {3,6,3}, {3,5,3},
    {3,5,3}, {3,5,3}, {3,4,3}, {2,4,3}, {2,3,2},
    {2,2,2}, {1,1,1}, {1,1,1}, {1,0,1}, {1,0,0}
};

static const VKAttackKey *vk_attack_key(const VKWhip *w)
{
    return &vk_attack_keys[w->timer <= 14 ? w->timer : 0];
}

int vk_whip_arm_frame(const VKWhip *w)
{
    return vk_attack_key(w)->phase;
}
void vk_whip_arm_position(const VKWhip *w, int *x, int *y)
{
    int frame = vk_whip_arm_frame(w);
    int dx = frame == 1 ? -2 : (frame == 2 ? 7 : (frame == 3 ? 13 : 1));
    if (w->facing) dx = -dx - 8;
    *x = (int)w->x + dx;
    *y = (int)w->y - (frame == 1 ? 29 : (frame == 4 ? 27 : 22));
}

int vk_whip_visual_step(const VKWhip *w)
{
    return vk_attack_key(w)->standing;
}

int vk_whip_crouch_visual_step(const VKWhip *w)
{
    return vk_attack_key(w)->crouched;
}
