#ifndef VK_HEALTH_H
#define VK_HEALTH_H

#include "player.h"

/* Simon's lives, death and game over (research/player-health.md 3.3-3.5).
 * No SGDK dependency: tested natively by tests/test_health.c. The energy itself
 * (C415) lives in EnemyState (enemies.c), which applies the contact damage.
 *
 * Proven by openMSX trace (player-health.md):
 *   - lives = C410: 3 at game init, 2 once play starts (first state 4); a death
 *     with C410 != 0 refills the energy and decrements C410; with C410 = 0 the
 *     game is over and returns to the title.
 *   - Timings (60 Hz): contact at energy 0 -> state 6 after 4.207 s (252 frames);
 *     state 6 -> state 4 (C410 decrement, play again) 0.45 s (27 frames);
 *     state 6 -> title 6.31 s (378 frames) when the game is over.
 * PROVISIONAL (not traced):
 *   - Simon's controls are locked from the fatal contact to the respawn; the MSX
 *     death animation is not documented.
 *   - Respawn place (vk_game_respawn in gameplay.c).
 *   - Bar scale: 2 px per energy point (32 points on the 64 px HUD bar). */
#define VK_HEALTH_LIVES_START 2      /* C410 once play starts (3, minus the first state 4) */
#define VK_HEALTH_ENERGY_MAX 0x20    /* C415, same as ENEMY_ENERGY_MAX */
#define VK_HEALTH_DEATH_FRAMES 252   /* fatal contact -> state 6 (40.617 s -> 44.824 s) */
#define VK_HEALTH_RESPAWN_FRAMES 27  /* state 6 -> state 4 (44.824 s -> 45.274 s) */
#define VK_HEALTH_GAMEOVER_FRAMES 378 /* state 6 -> title (44.824 s -> 51.13 s) */
#define VK_HEALTH_BAR_PIXELS 64      /* filled width of the HUD PLAYER bar at full energy */

typedef enum {
    VK_LIFE_ALIVE = 0,
    VK_LIFE_DYING = 1,       /* energy 0, waiting for state 6 */
    VK_LIFE_RESPAWN = 2,     /* state 6 with lives left, waiting for state 4 */
    VK_LIFE_GAME_OVER = 3    /* state 7, waiting for the title */
} VKLifePhase;

typedef enum {
    VK_HEALTH_NONE = 0,
    VK_HEALTH_DIED = 1,      /* energy just reached 0 */
    VK_HEALTH_REFILL = 2,    /* state 6 with lives left: energy back to the maximum (62D0) */
    VK_HEALTH_RESPAWN = 3,   /* state 4: one life used, restart */
    VK_HEALTH_GAME_OVER = 4, /* state 6 with no life left: game over screen */
    VK_HEALTH_TITLE = 5      /* end of state 7: back to the title */
} VKHealthEvent;

typedef struct {
    VKByte lives;            /* C410 */
    VKByte phase;            /* VKLifePhase */
    VKWord timer;            /* frames left in the current phase */
} VKHealth;

/* Start of play: VK_HEALTH_LIVES_START lives, alive. */
void vk_health_begin(VKHealth *h);
/* One gameplay frame, after the enemy contact of the frame; energy is C415. */
VKHealthEvent vk_health_tick(VKHealth *h, int energy);
/* 1 while Simon cannot be controlled (from the fatal contact to the respawn). */
int vk_health_controls_locked(const VKHealth *h);
/* Filled width of the HUD bar for an energy value (0..VK_HEALTH_BAR_PIXELS). */
int vk_health_bar_pixels(int energy);

#endif
