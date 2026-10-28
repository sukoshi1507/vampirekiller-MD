#ifndef VK_CANDLE_VIEW_H
#define VK_CANDLE_VIEW_H

#include <genesis.h>
#include "candles.h"
#include "actors.h"

/* Candles, drops and fixed objects of candles.c (wall candles, object sheet of
 * VRAM page 1) as PAL0 sprites, and the heart count of the HUD.
 * On the MSX the candles and objects are copied into the bitmap; here they are
 * 16x16 sprites at the same coordinates. */

#define CANDLE_SPRITE_BASE (ENEMY_SPRITE_BASE + ENEMY_MAX * ENEMY_SPRITES)

extern VKCandles vk_candles;

/* After a room or scene is shown: loads the tiles and draws the heart count. */
void vk_candle_view_room(void);
/* Candle i of the current room was put out: in hall room 0 (scene 6, candles
 * baked in the picture) its background cells are restored. */
void vk_candle_view_out(int i);
/* Redraws the two heart digits (row 0, columns 24-25, as on the MSX HUD). */
void vk_candle_view_hearts(void);
/* Candles and drops as sprites from CANDLE_SPRITE_BASE, chained in order, the
 * last one linking to 0. Returns how many were set. */
u16 vk_candle_view_sprites(void);

#endif
