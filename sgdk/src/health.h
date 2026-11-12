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
 * Respawn place (vk_game_respawn in gameplay.c): 70D6 clears D001, 6402 reads
 * 641F[D000] (code; traced at stage 2: room 0, x 16, y 96).
 * Controls: no key is read during the recoil (C420 = 5) nor in the death state
 * (C420 = 6, 7053-7070: pose 09/05, the remains of Simon; 767B and the routines
 * of Simon return while C420 = 6), so they stay locked from the fatal contact to
 * the respawn (code, research/player-health.md; the pose is in gameplay.c).
 * Bar scale: 2 px per energy point, 4 px high, empty part in colour 0 (45B9
 * executed for the 33 values by tools/verify_rom_markers.py). */
#define VK_HEALTH_LIVES_START 2      /* C410 once play starts (3, minus the first state 4) */
#define VK_HEALTH_ENERGY_MAX 0x20    /* C415, same as ENEMY_ENERGY_MAX */
/* Fatal contact: the death clock starts when the recoil ends on the floor (7053 sets
 * C420 = 6, C428 = 1; the contact -> state 6 delay of 252 frames of the run 4 is this
 * recoil plus the delay below). 7093 then runs once a pass (2 frames): the next pass
 * (709C) sets C428 = 90 and asks for 89 (70A0), 70B2 counts it down and 70C0 ends the
 * death 90 passes later; state 6 follows. openMSX, 2 runs identical: 7070 at 41.9116 s,
 * 709E / 5136 at 41.9437 s (2 frames), 70C0 at 44.8138 s (172 frames later: the passes are
 * 2 frames long except the last 8 even values of C428 (14..0), 1 frame long), 4251 at
 * 44.8239 s. */
#define VK_HEALTH_JINGLE_FRAMES 2    /* 7070 -> 709E / 5136 (jingle 89) */
#define VK_HEALTH_DEAD_FRAMES 174    /* 7070 -> 70C0 (state 6 the frame after) */
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
    VKByte fell;             /* the death comes from a fall (C420 = 6) */
} VKHealth;

/* Start of play: VK_HEALTH_LIVES_START lives, alive. */
void vk_health_begin(VKHealth *h);
/* Fall out of the room (7093, C420 = 6, openMSX 2026-11-12, tools/run_jump_cases.py cases water_s2r4b and
 * air_s15r3): C428 = 30 passes in the water stages (D000 2 or 10, splash sound 9) or 1 elsewhere, one pass
 * = 2 frames (C003), then C428 = 90 (jingle 89, 709E) and 90 more passes: C413 = 0 (70D9) 182 frames after
 * the first frame with C420 = 6 (240 in the water), state 6 one frame later; 709E comes 2 frames (60 in the
 * water) after the fall. All the passes last 2 frames (the 1-frame passes of the contact death are not
 * seen). The hearts are capped to 5 (70B4). Same timer convention as VK_HEALTH_DEAD_FRAMES (the first
 * decrement is the next frame). */
#define VK_HEALTH_FALL_WATER_PASSES 30
#define VK_HEALTH_FALL_PASSES 1
#define VK_HEALTH_FALL_FRAMES 180   /* 90 passes of 2 frames after C428 := 90 */
#define VK_HEALTH_HEARTS_AFTER_FALL 5
#define VK_HEALTH_FALL_JINGLE(passes) (2 * (passes))    /* 709E */
void vk_health_fall(VKHealth *h, int stage);
/* The fatal recoil ended (C420 = 6, 7070): starts the death clock of VK_HEALTH_DEAD_FRAMES. */
void vk_health_dead(VKHealth *h);
/* One gameplay frame, after the enemy contact of the frame; energy is C415. */
VKHealthEvent vk_health_tick(VKHealth *h, int energy);
/* 1 while Simon cannot be controlled (from the fatal contact to the respawn). */
int vk_health_controls_locked(const VKHealth *h);
/* Filled width of the HUD bar for an energy value (0..VK_HEALTH_BAR_PIXELS). */
int vk_health_bar_pixels(int energy);

#endif
