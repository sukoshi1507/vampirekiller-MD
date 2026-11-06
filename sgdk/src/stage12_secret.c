/* ROM 85F4 / 70FB / B98C + BBCD, per research/enemy-behavior.md.
 * Only isolated state logic. No guessed SGDK timing, collision, or audio hookup.
 */
#include "stage12_secret.h"
void vk_stage12_secret_reset(VKStage12SecretState *s)
{
    s->active = 0;
    s->timer = 0;
    s->destination = VK_ROOM_NONE;
}
VKByte vk_stage12_secret_begin(VKStage12SecretState *s, VKByte room,
    VKByte simon_y, VKByte simon_x, VKByte crouching, VKByte up_new)
{
    unsigned i;
    if (s->active || !crouching || !up_new) return 0;
    for (i=0; i<VK_STAGE12_PASSAGE_COUNT; i++) {
        const VKStage12Passage *p=&vk_stage12_raw_passages[i];
        /* Original Z80 arithmetic: 8-bit difference, positive result < 16. */
        VKByte dy=(VKByte)(simon_y-4-p->y);
        VKByte dx=(VKByte)(simon_x-p->x);
        if (p->source==room && dy<16 && dx<16) {
            s->active=1; /* 7 in C420, represented by boolean for an isolated stage */
            s->timer=VK_STAGE12_SECRET_DELAY; /* C42D = 40h */
            s->destination=p->destination;
            return VK_STAGE12_SECRET_EFFECT; /* event requested, not played here */
        }
    }
    return 0;
}
VKByte vk_stage12_secret_tick(VKStage12SecretState *s, VKByte *destination)
{
    if (!s->active) return 0;
    if (s->timer) s->timer--;
    if (s->timer) return 0;
    if (destination) *destination=s->destination;
    vk_stage12_secret_reset(s);
    return 1; /* room switch should clear C43D, C41B and load D001 */
}
