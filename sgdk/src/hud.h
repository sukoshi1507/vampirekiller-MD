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
 * The scale is read in 45D1: the PLAYER bar is 2 x C415 px wide at (60, 14),
 * 4 px high, colour 11h (490A). PROVISIONAL: the empty colour (0). */
#define HUD_TILE_INDEX 900          /* 27 tiles, free VRAM between the scene and sprite tiles */

/* Text of the HUD (4513, texts at 4C00 drawn by 4ACB in the MSX font):
 * "SCORE-" (8, 0), "PLAYER" (8, 12), "ENEMY" (8, 20), "STAGE-" (108, 0),
 * heart and dash (176, 0), "P-" (212, 0); the values in BCD digits: score
 * C405-C407 at (56, 0), stage C411 at (156, 0), hearts C417 at (192, 0), lives
 * C410 at (228, 0) (4531, 453B, 4566, 456E). Composed tiles from
 * HUD_TEXT_TILE_INDEX: VRAM after the largest scene (16 + 201 tiles) and before
 * FLAME_A_CLEAR_TILE_INDEX (400). */
#define HUD_TEXT_TILE_INDEX 224
#define HUD_TEXT_TILE_COUNT 51

/* Draws the bar cells, the labels and the status line (last values) after a
 * scene or room load. */
void vk_hud_show(void);
/* Score, stage, hearts and lives of the top line; redraws the fields that
 * changed. */
void vk_hud_status(u32 score, u16 stage, u16 hearts, u16 lives);
/* Score (C405-C407, 6 BCD digits): 0 at a new game (44C6); 44EE adds the
 * points of a killed enemy (81AB, table 81CE in hundreds) up to 999999. */
extern u32 vk_hud_score;
void vk_hud_add_score(u32 points);
/* Redraws the PLAYER bar for the energy (C415) when its width changed. */
void vk_hud_update(int energy);

#endif
