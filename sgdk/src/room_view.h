#ifndef VK_ROOM_VIEW_H
#define VK_ROOM_VIEW_H

#include <genesis.h>
#include "player.h"
#include "whip.h"

/* Room display: ROM scenes, stage rooms, courtyard flames, training target and
 * the debug room label. Plane A only; sprites are drawn by actors.c. */
#define COURTYARD_MAX_FLAMES 2

/* Full-screen ROM scene (intro_assets): tiles, palette PAL0 and 32x28 map. */
void vk_show_scene(u16 index);
/* Room viewer: one stage room (or its tile set with atlas) and the help text. */
void vk_show_room(u16 stage, u16 room, bool atlas);
/* Play scenes. They copy vk_game.player into vk_player and set vk_front_mode. */
void vk_show_courtyard(u16 room);
void vk_show_interior(void);
void vk_show_stage1_room(u16 room);
/* Movement test in the first stage room (viewer B). */
void vk_enter_play(VKPlayer *player);

/* Courtyard flames (courtyard) or training target (elsewhere), relit / replaced. */
void vk_reset_training_target(void);
void vk_hide_targets(void);
bool vk_targets_visible(void);
/* Whip test against the courtyard flames: a hit flame is put out and its torch
 * background cleared. Returns its index, or -1 (also when not in the courtyard). */
int vk_courtyard_flames_hit(const VKWhip *whip);
/* Whip test against the training target; TRUE on a new hit. */
bool vk_training_target_hit(const VKWhip *whip);
/* Advances the flame animation by one frame. */
void vk_flames_tick(void);
/* Flames or target as sprites 7.. ; the last one links to enemy_link. Returns
 * TRUE when the courtyard flames were drawn (sprites 7..8, no enemy link). */
bool vk_draw_targets(u16 enemy_link);

/* Redraws the test note of the play room on row 26 (after a diagnostic). */
void vk_draw_play_note(void);

/* Debug overlay with the internal room number (Konami code in play). */
void vk_debug_label_toggle(void);
void vk_debug_label_draw(void);

#endif
