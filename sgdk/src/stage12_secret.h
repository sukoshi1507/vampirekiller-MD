/* Standalone ROM-derived stage 12 secret passage state. Not wired to gameplay. */
#ifndef VK_STAGE12_SECRET_H
#define VK_STAGE12_SECRET_H
#include "generated/stage12_foundations.h"
#define VK_STAGE12_SECRET_DELAY 0x40
#define VK_STAGE12_SECRET_EFFECT 15
/* Action 1 requests the ROM effect 15; action 2 requests room change to destination. */
typedef struct { VKByte active, timer, destination; } VKStage12SecretState;
void vk_stage12_secret_reset(VKStage12SecretState *s);
/* Input is the current player C425/C427 and freshly pressed UP. Only state 2 may enter. */
VKByte vk_stage12_secret_begin(VKStage12SecretState *s, VKByte room,
    VKByte simon_y, VKByte simon_x, VKByte crouching, VKByte up_new);
/* One update of ROM state 7; returns destination-event after 64 countdown passes. */
VKByte vk_stage12_secret_tick(VKStage12SecretState *s, VKByte *destination);
#endif
