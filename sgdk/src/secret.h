#ifndef VK_SECRET_H
#define VK_SECRET_H
#include "generated/secret_data.h"
/* Executed85F4,6DA9,70FB,B98C; research/secret-passages.md.
 * C42D belongs to the common timer75C0, never to70FB. No legacy layout changes. */
#define VK_SECRET_EFFECT 0x15
#define VK_SECRET_DELAY 0x40
typedef struct {
    VKByte point[4]; /* C5B1..C5B4 */
    VKByte active,phase,route,platform; /* C420=7/0,C421,C41B,C43D */
} VKSecret;
extern VKSecret vk_secret;
void vk_secret_enter(VKSecret *s,VKByte stage,VKByte room);
int vk_secret_near(const VKSecret *s,VKByte y,VKByte x);
/* simon_state is the real C420 value, UP is the newly pressed C006 bit5. */
VKByte vk_secret_begin(VKSecret *s,VKByte simon_state,VKByte y,VKByte x,
                       VKByte up_new,VKByte *invuln);
/* Called BEFORE common timer75C0. Does not decrement invuln. */
int vk_secret_poll(VKSecret *s,VKByte invuln);
/* B963's FF branch B98C: keep X,Y,stage, clear request, select destination. */
int vk_secret_route(VKSecret *s,VKByte *room);
/* 85C6: crusher contact can replace C420=7 with5 at zero C42D. */
void vk_secret_interrupt(VKSecret *s);
#endif
