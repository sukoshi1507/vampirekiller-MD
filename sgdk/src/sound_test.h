#ifndef VK_SOUND_TEST_H
#define VK_SOUND_TEST_H

#include <genesis.h>

/* Konami code on the title screen: UP UP DOWN DOWN LEFT RIGHT LEFT RIGHT B A.
 * Call once per frame with the edge-triggered button mask. */
bool vk_konami_step(u16 pressed);

/* Debug sound test. Entering loads the XGM2 driver on first use and draws the
 * menu. Returns TRUE from vk_sound_test_tick when the player leaves with B. */
void vk_sound_test_enter(void);
bool vk_sound_test_tick(u16 pressed);

#endif
