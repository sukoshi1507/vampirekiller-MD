#ifndef VK_ROOM_VIEW_H
#define VK_ROOM_VIEW_H

#include <genesis.h>
#include "player.h"
#include "whip.h"

/* Room display: ROM scenes, stage rooms, courtyard flames, training target and
 * the debug room label. Plane A only; sprites are drawn by actors.c. */
#define COURTYARD_MAX_FLAMES 2

/* Room draw of a room-change freeze (main.c room_freeze_step sets vk_fast_room_draw):
 * the display stays off from the start of vk_show_courtyard / vk_show_castle_room until
 * vk_room_display_on, and the tile loads go by blocking DMA (about 170 bytes per line, 3x
 * the CPU loop, and 10x it with the display on: research/room-draw-budget.md). Every other
 * draw (respawn, stage start, debug jump) keeps the CPU copies with the display on, whose
 * timing the scenarios calibrate on. */
extern bool vk_fast_room_draw;
extern bool vk_tiles_by_dma;
#define VK_TM (vk_tiles_by_dma ? DMA : CPU)
/* Ends a room draw: back to CPU copies, display on. */
void vk_room_display_on(void);

/* Full-screen ROM scene (intro_assets): tiles, palette PAL0 and 32x28 map. */
void vk_show_scene(u16 index);
/* Room viewer: one stage room (or its tile set with atlas) and the help text. */
void vk_show_room(u16 stage, u16 room, bool atlas);
/* Play scenes. They copy vk_game.player into vk_player and set vk_front_mode. */
void vk_show_courtyard(u16 room);
void vk_show_interior(void);
void vk_show_castle_room(u16 stage, u16 room);
/* Movement test in the first stage room (viewer B). */
void vk_enter_play(VKPlayer *player);

/* Courtyard flames (courtyard) or training target (elsewhere), relit / replaced. */
void vk_reset_training_target(void);
void vk_hide_targets(void);
bool vk_targets_visible(void);
/* Courtyard torch 'flame' put out by the whip (candles.c): its flame goes and
 * its torch background is cleared. */
void vk_courtyard_flame_out(int flame);
/* Whip test against the training target; TRUE on a new hit. */
bool vk_training_target_hit(const VKWhip *whip);
/* Advances the flame animation by one frame. */
void vk_flames_tick(void);
/* Flames or target as sprites 7.. ; the last one links to enemy_link. Returns
 * TRUE when the courtyard flames were drawn (sprites 7..8). */
bool vk_draw_targets(u16 enemy_link);

/* Redraws the test note of the play room on row 26 (after a diagnostic). */

/* Debug overlay with the internal room number (Konami code in play). */
void vk_debug_label_toggle(void);
void vk_debug_label_draw(void);

#endif
