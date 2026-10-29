#include "actors.h"
#include "frontend.h"
#include "room_view.h"
#include "generated/whip_level2_md.h"
#include "generated/enemy_sprites_md.h"
#include "hud.h"
#include "candle_view.h"
#include "psg_player.h"

_Static_assert(ENEMY4_TILE_INDEX + VK_ENEMY4_TILE_COUNT <= HUD_TILE_INDEX,
               "sprite tiles 400.. must end before the HUD tiles");
_Static_assert(ENEMY5_TILE_INDEX + VK_ENEMY5_TILE_COUNT <= HUD_TILE_INDEX,
               "type 5 tiles must end before the HUD tiles");
_Static_assert(ENEMY5_TILE_INDEX + VK_ENEMY2_TILE_COUNT <= HUD_TILE_INDEX,
               "type 2 tiles (loaded in the type 5 block) must end before the HUD tiles");

/* Simon, whip and hall enemies on screen. Their gameplay lives in player.c,
 * whip.c and enemies.c; this module only feeds them and draws the result. */
EnemyState vk_enemies;
/* Hall enemies (enemies.c). Their tiles use PAL2 indices given by the MSX2 colour table. */
static EnemySpawnTimer enemy_timer;
static u16 enemy_room_key = 0xFFFF;   /* room_key() while the enemies belong to it, else 0 */
static int last_attack_frame = -1;
/* Three animation-friendly tiles: grip, narrow lash and bright tip.
 * These are original placeholder graphics, not ROM-extracted attack frames. */
static const u32 whip_tiles[24] = {
    0,0,0,0x000FFF00UL,0x000FFF00UL,0,0,0,
    0,0,0,0x0FFFFFF0UL,0x0FFFFFF0UL,0,0,0,
    0,0,0,0x00FFFF00UL,0x0FFFFFF0UL,0,0,0
};

/* Four 8x8 arm/keyframe graphics: skin (E), bright glove (F).
 * Placeholder Mega Drive art, not yet verified MSX2 attack data. */
static const u32 arm_tiles[32] = {
    0x000EE000UL,0x00EEEE00UL,0x0EEEEEE0UL,0x00EEFF00UL,
    0x000FF000UL,0,0,0,
    0,0,0xEEE00000UL,0x0EEEE000UL,
    0x00EEEEF0UL,0x000FFFF0UL,0,0,
    0,0,0xEEEEEEEEUL,0xEEEEEEEEUL,
    0x000FFFFFUL,0,0,0,
    0,0x0EEE0000UL,0x00EEEE00UL,0x000EEFF0UL,
    0x0000FF00UL,0,0,0
};
static const u16 attack_palette[16] = {
    0,0x002A,0x006E,0x00CE,0,0,0,0,0,0,0,0,0,0,0x0CA0,0x0EEE
};
/* 16x16 training target, deliberately NOT an original ROM asset. */
static const u32 target_tiles[32] = {
    0,0,0x000FF000UL,0x00F00F00UL,0x0F0000F0UL,0x0F0000F0UL,0xF000000FUL,0xF000000FUL,
    0,0,0x000FF000UL,0x00F00F00UL,0x0F0000F0UL,0x0F0000F0UL,0xF000000FUL,0xF000000FUL,
    0xF000000FUL,0xF000000FUL,0x0F0000F0UL,0x0F0000F0UL,0x00F00F00UL,0x000FF000UL,0,0,
    0xF000000FUL,0xF000000FUL,0x0F0000F0UL,0x0F0000F0UL,0x00F00F00UL,0x000FF000UL,0,0
};
/* Whip level chosen for the next load (0 by default). No input binds it yet, so
 * the wired gameplay keeps the level 0 frames and the level 0 hit box. */
static int whip_level_choice = 0;

/* One gameplay frame of the castle enemies. The state follows the castle room: a
 * room change or a leave of the castle rebuilds it (key 0 outside). In stage 2 the
 * type 4 (rooms 0, 1, 3) and the type 1 of room 2 are ported (enemies.c,
 * kEnemyStage2RoomFlags); the type 1 reuses the hall layers of vk_enemy_tiles
 * (same VRAM patterns 40..6C, checked in the room 2 trace). A contact starts
 * Simon's recoil in gameplay.c (vk_game_hit). */
/* Castle room the enemy state belongs to: stage 1 rooms 1..8, stage 2 rooms 9..14. */
static u16 room_key(void)
{
    if (vk_front_mode != FRONT_INTERIOR) return 0;
    return (u16)(8 * (vk_game.stage - 1) + vk_game.room + 1);
}

void vk_step_enemies(void)
{
    const u16 key = room_key();
    EnemyRoomVars vars;
    if (key != enemy_room_key)
    {
        enemy_room_key = key;
        Enemies_setRoom(&vk_enemies);
        Enemies_spawnTimerRoom(&enemy_timer);   /* 9CA9: CF10 carries over */
        if (key)
        {
            VDP_loadTileData((const u32 *)vk_enemy_tiles, ENEMY_TILE_INDEX, VK_ENEMY_TILE_COUNT, CPU);
            VDP_loadTileData((const u32 *)vk_enemy4_tiles, ENEMY4_TILE_INDEX, VK_ENEMY4_TILE_COUNT, CPU);
            /* Stage 2 rooms 4 and 5: the fishman and its splash take the type 5 block. */
            if (vk_game.stage == 2 && (vk_game.room == 4 || vk_game.room == 5))
                VDP_loadTileData((const u32 *)vk_enemy2_tiles, ENEMY5_TILE_INDEX, VK_ENEMY2_TILE_COUNT, CPU);
            else
                VDP_loadTileData((const u32 *)vk_enemy5_tiles, ENEMY5_TILE_INDEX, VK_ENEMY5_TILE_COUNT, CPU);
        }
    }
    if (!key) return;
    vars.stage = vk_game.stage;
    vars.room = vk_game.room;
    vars.simonX = vk_game.player.x;
    vars.simonY = vk_game.player.y;
    vars.sideFlag = 0;
    vars.difficulty = 0;
    vars.map = vk_game_collision(&vk_game);   /* floor for falling enemies */
    /* C420 = 5 while Simon is knocked back: no enemy contact (7EF4). */
    vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    if (Enemies_frame(&vk_enemies, &enemy_timer, &vars))
    {
        /* 7EC1: the whip stops (C422 := 0) and Simon is knocked back. */
        vk_whip_reset(&vk_whip);
        vk_game_hit(&vk_game, vk_enemies.recoilDir, vk_enemies.energy == 0);
        vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    }
    if (vk_whip_active(&vk_whip))
    {
        int left = 0, top = 0, right = 0, bottom = 0;
        int hits, fish = 0;
        vk_whip_bounds(&vk_whip, &left, &top, &right, &bottom);
        hits = Enemies_whipHit(&vk_enemies, left, top, right, bottom);
        /* Type 5: only in the MSX whip phase 3 (C422 = 3), taken as the full lash
         * (vk_whip_phase 2, PROVISIONAL match), from the latched whip origin. */
        if (vk_whip_phase(&vk_whip) == 2)
        {
            hits += Enemies_whipHitType5(&vk_enemies, vk_whip.x, vk_whip.y,
                                         vk_whip.facing, vk_whip.crouched);
            /* Stage 2 type 1: same phase, ROM box of class 1 (enemies.h). */
            hits += Enemies_whipHitType1Rom(&vk_enemies, vk_whip.x, vk_whip.y,
                                            vk_whip.facing, vk_whip.crouched);
            /* Stage 2 type 2 (fishman): same phase, ROM box of class 1. */
            fish = Enemies_whipHitType2(&vk_enemies, vk_whip.x, vk_whip.y,
                                        vk_whip.facing, vk_whip.crouched);
        }
        /* 7DF9 -> 81AB: score of the killed enemy, table 81CE in hundreds: types
         * 1, 4 and 5 give 100, type 2 (fishman) 200. */
        if (hits || fish) vk_hud_add_score((u32)hits * 100 + (u32)fish * 200);
        hits += fish;
        /* 7F65 (D700 loop) and 7DD4 (C800 objects): effect 0C on a whip contact. */
        if (hits) vk_psg_effect(VK_SFX_HIT);
    }
}

void vk_reset_enemies(void)
{
    Enemies_init(&vk_enemies);
    enemy_room_key = 0xFFFF;
}

void vk_reset_enemy_clock(void)
{
    Enemies_spawnTimerInit(&enemy_timer);
}

/* Enemy layers as hardware sprites from ENEMY_SPRITE_BASE, one 16x16 sprite each,
 * chained in order, the last one linking to tail. Returns the count, 0 outside the
 * hall room the state belongs to. */
static u16 show_enemy_sprites(u16 tail)
{
    EnemySprite list[ENEMY_MAX * ENEMY_SPRITES];
    u16 count = 0;
    u16 i;
    if (!enemy_room_key || enemy_room_key != room_key()) return 0;
    for (i = 0; i < ENEMY_MAX; i++)
    {
        EnemySprite sprites[ENEMY_SPRITES];
        const int n = Enemies_sprites(&vk_enemies, (EnemyU8)i, sprites);
        int k;
        for (k = 0; k < n; k++) list[count++] = sprites[k];
    }
    for (i = 0; i < count; i++)
    {
        const u8 pattern = list[i].pattern;
        const u16 tile = list[i].bank
            ? (u16)(ENEMY5_TILE_INDEX + pattern - VK_ENEMY5_FIRST_PATTERN)
            : (pattern & ENEMY_PATTERN_TYPE4)
            ? (u16)(ENEMY4_TILE_INDEX + (pattern & ~ENEMY_PATTERN_TYPE4) - VK_ENEMY4_FIRST_PATTERN)
            : (u16)(ENEMY_TILE_INDEX + pattern - VK_ENEMY_FIRST_PATTERN);
        VDP_setSpriteFull((u16)(ENEMY_SPRITE_BASE + i), list[i].x, list[i].y, SPRITE_SIZE(2, 2),
            TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, tile),
            (u16)(i + 1 < count ? ENEMY_SPRITE_BASE + i + 1 : tail));
    }
    return count;
}

void vk_show_player(const VKPlayer *shown)
{
    /* After a fall Simon is parked at Y FA, whose top rows would still reach the
     * 224-line screen: he is drawn lower, out of sight. */
    VKPlayer parked = *shown;
    const VKPlayer *player = &parked;
    if (parked.y == VK_FALL_Y) parked.y = 255;
    /* Enemy sprites come after the player chain: sprite 6 (or 7 when the target is
     * shown) links to ENEMY_SPRITE_BASE when there are enemies, then the candles and
     * objects of candle_view.c close the chain. */
    const u16 candle_count = vk_candle_view_sprites();
    const u16 candle_link = candle_count ? CANDLE_SPRITE_BASE : 0;
    const u16 enemy_count = show_enemy_sprites(candle_link);
    const u16 enemy_link = enemy_count ? ENEMY_SPRITE_BASE : candle_link;
    const bool target_visible = vk_targets_visible();
    const int attack_frame = vk_whip_arm_frame(&vk_whip);
    const int crouch_mode = !player->on_stairs && (player->crouched || (attack_frame && vk_whip.crouched));
    const u16 crouch_frame = attack_frame ? (u16)vk_whip_crouch_visual_step(&vk_whip) : 0;
    const u16 crouch_facing = attack_frame ? vk_whip.facing : player->facing;
    /* Standing body on sprite 0. In crouch mode and during any attack the body
     * is the two-half composition drawn on sprites 5 and 6 (below); the lash
     * is part of those ROM-derived frames, so no separate lash is overlaid. */
    if (crouch_mode || attack_frame)
        VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE,
                           PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE), 1);
    else
        /* The recoil poses reach one pixel higher (extract_player.py POSE_TOP). */
        VDP_setSpriteFull(0, (s16) player->x - 8,
            (s16) player->y - (player->pose >= VK_PLAYER_RECOIL_POSE ? 32 : 31), SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE,
                           PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE), 1);
    {
        int left = 0, top = 0, right = 0, bottom = 0;
        /* The MSX2 ROM composition already contains the visible lash.
         * Keep gameplay reach in whip.c, but never overlay the old synthetic
         * three-segment graphics on top of the ROM sprite. */
        int count = 0;
        int segment;
        if (count) vk_whip_bounds(&vk_whip, &left, &top, &right, &bottom);
        for (segment = 0; segment < 3; segment++)
        {
            s16 sx = -32;
            s16 sy = -32;
            u16 tile = WHIP_TILE_INDEX;
            if (segment < count)
            {
                /* Sprite positions and collision reach now match exactly.
                 * Left-facing attacks are drawn from Simon outwards. */
                sx = (s16)(vk_whip.facing ? right - 8 - segment * 8 : left + segment * 8);
                sy = (s16)top - 2;
                tile += (u16)(segment == count - 1 ? 2 : (segment == 0 ? 0 : 1));
            }
            VDP_setSpriteFull((u16)(segment + 1), sx, sy, SPRITE_SIZE(1, 1),
                TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, tile), (u16)(segment + 2));
        }
    }
    /* Retired placeholder arm: ROM-derived body already contains the arm, so
     * this sprite stays parked off-screen. */
    {
        int frame = vk_whip_arm_frame(&vk_whip);
        VDP_setSpriteFull(4, -32, -32, SPRITE_SIZE(1, 1),
            TILE_ATTR_FULL(PAL3, TRUE, FALSE, vk_whip.facing,
                           ARM_TILE_INDEX + (frame ? (u16)(frame - 1) : 0)), 5);
    }
    /* Two 32x32 hardware sprites cover a 64x32 anchored composition, both for
     * the standing attack and for the crouch body / crouched attack.
     * The decoded candidates are already right-facing: facing == 0 means right,
     * facing == 1 means left. Only left-facing frames need horizontal reflection
     * and reversal of the source half order.
     * The right-facing canvas starts at x-24 (the traced frames reach x+39).
     * Its mirror starts at x-39, so that the left-facing block lands on x-39..x+24.
     * Crouch frames are placed 25 px above Simon's y (feet anchor), attack frames 30 px.
     * Phase-to-index mapping: research/simon-assets/ATTACK_TRACE_OPENMSX.md (standing)
     * and the crouched-attack trace summarized in tools/build_simon_crouch_md.py. */
    {
        u16 i;
        const u16 facing = (u16)crouch_facing;
        const bool body = crouch_mode || attack_frame;
        const u16 frame_index = attack_frame ? (u16)vk_whip_visual_step(&vk_whip) : 0;
        /* Levels 1 and 2 hold their left-facing standing frames (3..5) as decoded
         * pictures, so they are drawn without flip. Crouch frames stay on level 0. */
        const bool level_frames = !crouch_mode && attack_frame && vk_whip_level(&vk_whip) != 0;
        const u16 flip = level_frames ? 0 : facing;
        const u16 base = crouch_mode
            ? CROUCH_TILE_INDEX + crouch_frame * VK_CROUCH_TILES_PER_FRAME
            : (level_frames
                ? WHIP_LEVEL_TILE_INDEX + (frame_index + (facing ? 3 : 0)) * VK_ATTACK_TILES_PER_FRAME
                : ATTACK_TILE_INDEX + frame_index * VK_ATTACK_TILES_PER_FRAME);
        const u16 tiles_per_half = crouch_mode ? VK_CROUCH_TILES_PER_HALF : VK_ATTACK_TILES_PER_HALF;
        const s16 origin_x = facing ? -39 : -24;
        const s16 origin_y = crouch_mode ? -25 : -30;
        for (i = 0; i < 2; ++i)
        {
            const u16 source_half = flip ? (u16)(1 - i) : i;
            VDP_setSpriteFull((u16)(5 + i),
                body ? (s16)player->x + origin_x + (s16)i * 32 : -32,
                body ? (s16)player->y + origin_y : -32,
                SPRITE_SIZE(4, 4),
                TILE_ATTR_FULL(PAL2, TRUE, FALSE, flip,
                               base + source_half * tiles_per_half),
                i == 0 ? 6 : (target_visible ? 7 : enemy_link));
        }
    }
    /* Courtyard flames or training target on sprites 7.. (room_view.c). */
    if (vk_draw_targets(enemy_link))
    {
        VDP_updateSprites(candle_count ? CANDLE_SPRITE_BASE + candle_count : 7 + COURTYARD_MAX_FLAMES,
                          DMA_QUEUE);
        return;
    }
    VDP_updateSprites(candle_count ? CANDLE_SPRITE_BASE + candle_count
                      : enemy_count ? ENEMY_SPRITE_BASE + enemy_count : (target_visible ? 8 : 7), DMA_QUEUE);
}

/* Whip level given by an object (C416, candles.c): loads its standing frames.
 * PROVISIONAL: C416 2..4 are the dagger, axe and cross; only the levels with an
 * asset (whip.h) are taken, the others keep the current whip. */
void vk_set_whip_level(int level)
{
    if (vk_whip_set_level(&vk_whip, level) < 0) return;
    whip_level_choice = level;
    if (level == 1)
        VDP_loadTileData((const u32 *)vk_whip_level1_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    else if (level == 2)
        VDP_loadTileData((const u32 *)vk_whip_level2_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
}

void vk_load_whip_graphics(void)
{
    VDP_loadTileData(whip_tiles, WHIP_TILE_INDEX, 3, CPU);
    VDP_loadTileData(arm_tiles, ARM_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *) vk_simon_attack_tiles, ATTACK_TILE_INDEX,
                     VK_ATTACK_FRAME_COUNT * VK_ATTACK_TILES_PER_FRAME, CPU);
    PAL_setPalette(PAL3, attack_palette, CPU);
    VDP_loadTileData(target_tiles, TARGET_TILE_INDEX, 4, CPU);
    VDP_loadTileData(vk_flame_fx_tiles, FLAME_TILE_INDEX,
                     (VK_FLAME_FRAMES + VK_SPARK_FRAMES) * VK_FLAME_TILES_PER_FRAME, CPU);
    VDP_loadTileData((const u32 *)vk_simon_crouch_tiles, CROUCH_TILE_INDEX, VK_CROUCH_TILE_COUNT, CPU);
    vk_whip_reset(&vk_whip);
    vk_whip_set_level(&vk_whip, whip_level_choice);
    if (vk_whip_level(&vk_whip) == 1)
        VDP_loadTileData((const u32 *)vk_whip_level1_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    else if (vk_whip_level(&vk_whip) == 2)
        VDP_loadTileData((const u32 *)vk_whip_level2_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    vk_hide_targets();
    last_attack_frame = -1;
}

/* Independent on-screen state check for BlastEm. */
void vk_draw_attack_frame_label(void)
{
    int frame = vk_whip_arm_frame(&vk_whip);
    if (frame != last_attack_frame)
    {
        last_attack_frame = frame;
        VDP_drawText(frame == 0 ? "F0" :
            (frame == 1 ? "F1" : (frame == 2 ? "F2" :
            (frame == 3 ? "F3" : "F4"))), 28, 2);
    }
}
