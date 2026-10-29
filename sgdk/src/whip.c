#include "whip.h"
void vk_whip_reset(VKWhip *w) { *w = (VKWhip){0}; }
int vk_whip_set_level(VKWhip *w, int level)
{
    if (level < 0 || level >= VK_WHIP_LEVEL_COUNT) return -1;
    w->level = (VKByte)level;
    return 0;
}
int vk_whip_level(const VKWhip *w) { return w->level; }
/* The MSX tests the whip once per stroke: 7DAD (C800), 7F49 (D700) and 7FB7
 * (candles C470) return unless C422 = 3, which only lasts the second pass of the
 * stroke (7240 sets it with C42F 7, 7250 moves it to 4 with C42F 8 before the
 * tests of the next pass). That pass is timers 12 and 11 here; the test is on 11,
 * the frame that holds the jump step of that pass whatever the parity of the
 * button press (research/whip-hitbox.md, "Fouet en saut"). */
int vk_whip_active(const VKWhip *w) { return w->timer == VK_WHIP_STRIKE_TIMER; }
void vk_whip_tick(VKWhip *w, const VKPlayer *p, int attack_pressed)
{
    if (w->timer) w->timer--;
    else if (attack_pressed) w->timer = 14;
    /* Direction and posture are taken at startup (7201: C42E from C420), so the
     * whip cannot turn across Simon during a stroke. The origin follows Simon on
     * every frame: the box 84A7/84D9 reads C425 and C427 in the pass of the test,
     * after Simon's move (6D5D), and the lash sprites are drawn at Simon's place
     * of each pass (openMSX trace of a stroke in a jump). */
    if (w->timer == 14) { w->facing = p->facing; w->crouched = p->crouched; }
    if (w->timer) { w->x = p->x; w->y = p->y; }
}
/* Standing lash box, inclusive: x +8..+32 and y -22..-15 relative to Simon.
 * These are the visible lash measured in-game (research/whip-hitbox-ingame.md),
 * not the openMSX trace box (research/whip-hitbox.md, x +8..+39, y -31..-16).
 * The left-facing box is the mirror image. The crouched box is set below. */
void vk_whip_bounds(const VKWhip *w, int *left, int *top, int *right, int *bottom)
{
    if (w->crouched)
    {
        /* Crouched lash measured on the Mega Drive frames (timers 9..2, right and
         * left facing mirror exactly): x +12..+32 (lash tip), y -17..-10. The
         * hit box is x +8..+32 and y -17..-10, constant over the attack. */
        *left = w->facing ? (int)w->x - 32 : (int)w->x + 8;
        *right = w->facing ? (int)w->x - 8 : (int)w->x + 32;
        *top = (int)w->y - 17;
        *bottom = (int)w->y - 10;
        return;
    }
    /* Reach and height depend on the level's lash, measured on the Mega Drive
     * frames against the MSX2 sprite table (research/whip-variants.md): level 1
     * reaches x +39, level 2 only x +12 and y -19..-15, level 0 keeps x +32. */
    {
        const int reach = w->level == 1 ? 39 : (w->level == 2 ? 12 : 32);
        const int top_offset = w->level == 2 ? 19 : 22;
        *left = w->facing ? (int)w->x - reach : (int)w->x + 8;
        *right = w->facing ? (int)w->x - 8 : (int)w->x + reach;
        *top = (int)w->y - top_offset;
        *bottom = (int)w->y - 15;
    }
}

/* Shared 15-step timeline for both body styles. Indices represent the
 * current timer (14 is attack start, 0 is idle). The standing frames follow the
 * openMSX trace: upper index 6 for 2 frames, 7 for 2 frames, 8 for 8 frames
 * (research/simon-assets/ATTACK_TRACE_OPENMSX.md). The body is shown from timer 14
 * to 3, that is 12 frames (levels 0 and 1; level 2 has its own table below). The
 * gameplay test (timer 11, vk_whip_active) is the same for every level.
 * The crouched attack follows the same 12 ticks with the lower index held at 6
 * (C42E = 6 while crouched): crouch frame 1 = upper 6 (timer 14..13), 2 = upper 7
 * (timer 12..11), 3 = upper 8 (timer 10..3). Frame 0 is the idle crouch.
 * Each entry is {arm phase, standing frame, crouched frame}. */
typedef struct { VKByte phase, standing, crouched; } VKAttackKey;
static const VKAttackKey vk_attack_keys[15] = {
    {0,0,0}, {0,0,0}, {0,0,0}, {1,2,3}, {1,2,3},
    {1,2,3}, {1,2,3}, {1,2,3}, {1,2,3}, {1,2,3},
    {1,2,3}, {1,1,2}, {1,1,2}, {1,0,1}, {1,0,1}
};
/* Level 2 lasts 6 frames in the openMSX trace: upper index 12 for 2 frames, 13 for
 * 2 frames, 14 for 2 frames (research/whip-variants.md, "Chronologie du niveau 2").
 * The same timer-14 start is kept, so the body is shown on timers 14..9 and the
 * entries for timers 8..0 are idle. The crouched column keeps the same 2-frame steps. */
static const VKAttackKey vk_attack_keys_level2[15] = {
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
    {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {1,2,3},
    {1,2,3}, {1,1,2}, {1,1,2}, {1,0,1}, {1,0,1}
};
#define VK_WHIP_LEVEL_SHORT 2

static const VKAttackKey *vk_attack_key(const VKWhip *w)
{
    const VKAttackKey *keys = w->level == VK_WHIP_LEVEL_SHORT ? vk_attack_keys_level2 : vk_attack_keys;
    return &keys[w->timer <= 14 ? w->timer : 0];
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
