#ifndef VK_HUD_H
#define VK_HUD_H

#include <genesis.h>

/* Energy bars of the MSX2 HUD (rows 1..3, columns 7..15 of plane A).
 *
 * The HUD frame is baked in the ROM scenes 3..6 (courtyard and hall, rows 0..3).
 * Its PLAYER bar (fill colour 1, 64 px wide, 4 px high) covers the cells of
 * columns 7..15 in rows 1 and 2, the ENEMY bar (colour 8) rows 2 and 3. Their
 * ROM tiles (scene 6 tiles 27-29, 41-43, 51-53) are copied here with the fill
 * pixels past the current energy cleared, and drawn in PAL1.
 * PROVISIONAL: the MSX HUD drawing routine is not read (research/player-health.md
 * 5.1): the scale (2 px per energy point) and the empty colour (0) are guesses. */
#define HUD_TILE_INDEX 900          /* 27 tiles, free VRAM between the scene and sprite tiles */

/* Draws the bar cells after a scene or room load. With labels, also writes
 * "PLAYER" and "ENEMY" (rooms without the baked HUD frame). */
void vk_hud_show(bool labels);
/* Redraws the PLAYER bar for the energy (C415) when its width changed. */
void vk_hud_update(int energy);

#endif
