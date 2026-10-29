#ifndef VK_ACTORS_H
#define VK_ACTORS_H

#include <genesis.h>
#include "player.h"
#include "enemies.h"
#include "generated/player_assets.h"
#include "generated/simon_attack_md.h"
#include "generated/whip_level1_md.h"
#include "generated/simon_crouch_md.h"
#include "generated/flame_fx_md.h"

/* Sprite VRAM layout, shared by every scene that shows Simon. */
#define PLAYER_TILE_INDEX 1024
#define WHIP_TILE_INDEX (PLAYER_TILE_INDEX + VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE)
#define ARM_TILE_INDEX (WHIP_TILE_INDEX + 3)
#define ATTACK_TILE_INDEX (ARM_TILE_INDEX + 4)
/* 16x16 training target, deliberately NOT an original ROM asset. */
#define TARGET_TILE_INDEX (ATTACK_TILE_INDEX + VK_ATTACK_FRAME_COUNT * VK_ATTACK_TILES_PER_FRAME)
#define FLAME_TILE_INDEX (TARGET_TILE_INDEX + 4)
#define CROUCH_TILE_INDEX (FLAME_TILE_INDEX + (VK_FLAME_FRAMES + VK_SPARK_FRAMES) * VK_FLAME_TILES_PER_FRAME)
/* Torch background once its flame is out (room_view.c). The block from here
 * (flame, whip levels, enemies: 400..691) sits in the free VRAM between the scene
 * tiles (user index up to 313) and the HUD tiles (900, hud.h). Tiles from 1536 on
 * hold the plane tables, so nothing may be loaded past the sprite tiles (1024..1382). */
#define FLAME_A_CLEAR_TILE_INDEX 400
/* Death pose (4 x 2 tiles each, two facings): after the HUD text tiles (hud.h). */
#define DEATH_TILE_INDEX 280
/* Stair poses of the ROM (12 tiles each), after the death pose. */
#define STAIR_TILE_INDEX (DEATH_TILE_INDEX + 2 * VK_PLAYER_DEATH_TILES)
/* Standing frames of whip levels 1 and 2 (6 frames: 3 right, 3 left, see
 * whip_levels_manifest.json). Loaded only when such a level is selected; level 0
 * keeps its frames at ATTACK_TILE_INDEX. */
#define WHIP_LEVEL_TILE_INDEX (FLAME_A_CLEAR_TILE_INDEX + 4)
#define WHIP_LEVEL_TILE_COUNT (VK_WHIP_LEVEL1_FRAME_COUNT * VK_WHIP_LEVEL1_TILES_PER_FRAME)
/* Hall enemy layers: VK_ENEMY_TILE_COUNT tiles after the whip levels; pattern N uses
 * tile ENEMY_TILE_INDEX + (N - VK_ENEMY_FIRST_PATTERN). */
#define ENEMY_TILE_INDEX (WHIP_LEVEL_TILE_INDEX + WHIP_LEVEL_TILE_COUNT)
/* Type 4 layers (room 4): their pattern numbers overlap types A and B, so they have
 * their own block; pattern ENEMY_PATTERN_TYPE4 | N uses ENEMY4_TILE_INDEX + (N - 0x48). */
#define ENEMY4_TILE_INDEX (ENEMY_TILE_INDEX + VK_ENEMY_TILE_COUNT)
/* Type 5 layers (rooms 2, 3, 7, patterns 40..BC, sprites of bank 1): after the key and
 * door tiles of item_view.c; pattern N uses ENEMY5_TILE_INDEX + (N - 0x40). */
#define ENEMY5_TILE_INDEX 760
#define ENEMY_SPRITE_BASE 9           /* hardware sprites 9.. (0..8 are Simon, whip, target) */

/* Hall enemies (enemies.c). Energy and invulnerability live here across rooms. */
extern EnemyState vk_enemies;

/* New game or new life: Enemies_init (full energy, no invulnerability or
 * recoil) and a rebuild of the room's enemies on the next step. */
void vk_reset_enemies(void);
/* New game (44C6 clears the RAM): the spawn clock CF10 shared by the enemy channels
 * restarts at 0. It is kept across rooms, lost lives and stages. */
void vk_reset_enemy_clock(void);
/* Whip level picked up (C416); levels without an asset are ignored. */
void vk_set_whip_level(int level);
/* Loads the whip, attack, crouch, target and flame tiles and resets the whip. */
void vk_load_whip_graphics(void);
/* Simon, the whip, the hall enemies and the targets of room_view.c, as hardware
 * sprites, then queues the sprite table. */
void vk_show_player(const VKPlayer *player);
/* One gameplay frame of the hall enemies (spawn, move, contact, whip hits, recoil). */
void vk_step_enemies(void);
/* Object 0A, before vk_step_enemies: DOWN newly pressed in a jump stops the enemies
 * for 5 hearts (Enemies_useTimeStop) and plays FB. */
void vk_step_time_stop(int down_pressed, int whip_pressed);
/* "F0".."F4" whip keyframe check at the top right, redrawn on change only. */
void vk_draw_attack_frame_label(void);

#endif
