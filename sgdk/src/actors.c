#include "actors.h"
#include "frontend.h"
#include "room_view.h"
#include "generated/whip_level2_md.h"
#include "generated/enemy_sprites_md.h"
#include "hud.h"

_Static_assert(HUD_TEXT_TILE_INDEX + HUD_TEXT_TILE_COUNT <= DEATH_TILE_INDEX &&
               DEATH_TILE_INDEX + 2 * VK_PLAYER_DEATH_TILES <= FLAME_A_CLEAR_TILE_INDEX,
               "death pose tiles between the HUD text and the torch tiles");
_Static_assert(STAIR_TILE_INDEX + VK_PLAYER_STAIR_POSES * VK_PLAYER_TILES_PER_POSE <= FLAME_A_CLEAR_TILE_INDEX,
               "stair pose tiles before the torch tiles");
#include "candle_view.h"
#include "psg_player.h"

_Static_assert(ENEMY4_TILE_INDEX + VK_ENEMY4_TILE_COUNT <= HUD_TILE_INDEX,
               "sprite tiles 400.. must end before the HUD tiles");
_Static_assert(ENEMY5_TILE_INDEX + VK_ENEMY5_TILE_COUNT <= HUD_TILE_INDEX,
               "type 5 tiles must end before the HUD tiles");
_Static_assert(ENEMY5_TILE_INDEX + VK_ENEMY5_TILE_COUNT <= SHIELD_TILE_INDEX &&
               SHIELD_TILE_INDEX + VK_PLAYER_TILES_PER_POSE <= HUD_TILE_INDEX,
               "shield pose tiles between the type 5 tiles and the HUD tiles");
_Static_assert(ENEMY5_TILE_INDEX + VK_ENEMY2_TILE_COUNT <= HUD_TILE_INDEX,
               "type 2 tiles (loaded in the type 5 block) must end before the HUD tiles");
_Static_assert(ENEMY_TIME_STOP_ITEM == VK_ITEM_BIT_0A,
               "object 0A: Enemies_useTimeStop reads the C701 bit 6 of vk_candles.c701");

/* Simon, whip and hall enemies on screen. Their gameplay lives in player.c,
 * whip.c and enemies.c; this module only feeds them and draws the result. */
EnemyState vk_enemies;
/* Hall enemies (enemies.c). Their tiles use PAL2 indices given by the MSX2 colour table. */
static EnemySpawnTimer enemy_timer;
static u16 enemy_room_key = 0xFFFF;   /* room_key() while the enemies belong to it, else 0 */
static int last_attack_frame = -1;
/* vk_player_shield_tiles pose now at SHIELD_TILE_INDEX, -1 none. */
static int shield_loaded = -1;
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
/* PAL3. Entries 1..3 belong to Simon's one-colour silhouette (790C, see
 * simon_palette below): Simon's tiles only use indices 1..3 (MSX colours 1, 2 and
 * their OR 3), so his sprites take PAL3 on the blink passes. The flame and spark
 * tiles (MSX indices 1, 2, 8, E) are loaded with 1 -> 4 and 2 -> 5 (load_flames). */
#define SIMON_FLASH_FIRST 1
#define FLAME_COLOR_1 4
#define FLAME_COLOR_2 5
static const u16 attack_palette[16] = {
    0,0,0,0,0x002A,0x006E,0,0,0,0,0,0,0,0,0x0CA0,0x0EEE
};
/* MSX colour shown in PAL3 entries 1..3, 0 when not loaded since the last PAL3 load. */
static u8 simon_flash_color = 0;
/* Backdrop colour last written to VDP register 7 (F3EB of 7604). */
static u8 backdrop_color = 0;
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

/* C420 as the contact box 847A reads it: 1 for a jump until it lands (6B9B .. 6D43,
 * including the port's mode 4 past the jump table), 2 crouched (6C20), 3 on a staircase
 * (6BEE), 4 for a fall begun on the ground (6BA5), 0 otherwise. */
static u8 simon_state(void)
{
    const VKPlayer *p = &vk_game.player;
    if (p->on_stairs) return ENEMY_SIMON_STAIRS;
    if (p->mode == 1) return ENEMY_SIMON_JUMP;
    if (p->mode == 4) return vk_game.ground_fall ? ENEMY_SIMON_FALL : ENEMY_SIMON_JUMP;
    if (p->crouched) return ENEMY_SIMON_CROUCH;
    return ENEMY_SIMON_GROUND;
}

/* Object 0A (7116-718F, in Simon's routine before the timers 75C0-75F2): DOWN newly
 * pressed (C006 bit 1) during a jump (C420 = 1, 7136), not on the frame of a whip
 * stroke (C006 bit 4 takes the 711D branch). Then 715F: C701 bit 6, C43B = 0, 5 hearts. */
void vk_step_time_stop(int down_pressed, int whip_pressed)
{
    if (whip_pressed || !down_pressed || simon_state() != ENEMY_SIMON_JUMP) return;
    if (!room_key()) return;   /* no 0A in the courtyard, whose frames run no Enemies_frame */
    if (Enemies_useTimeStop(&vk_enemies, &vk_candles.hearts, vk_candles.c431, vk_candles.c701))
        vk_psg_timer();        /* 718F: FB */
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
    vars.simonState = simon_state();          /* C420 for the contact box 847A */
    vars.facingLeft = vk_game.player.facing;  /* C42C: armor of object 03 (8179) */
    /* C420 = 5 while Simon is knocked back: no enemy contact (7EF4). */
    vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    /* 7064: the death (C420 = 6) clears C42D: no more blinking. */
    if (vk_game.recoil == VK_RECOIL_DEAD) vk_enemies.invuln = 0;
    const EnemyU8 armor = vk_enemies.armor;
    if (Enemies_frame(&vk_enemies, &enemy_timer, &vars))
    {
        /* 7EC1: the whip stops (C422 := 0) and Simon is knocked back. */
        vk_whip_reset(&vk_whip);
        vk_game_hit(&vk_game, vk_enemies.recoilDir, vk_enemies.energy == 0);
        vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    }
    /* 819C: the last armor hit (C441 = 0) clears C701 bit 4 and redraws the
     * item box (8EE6): the shield goes from the HUD and from Simon. */
    if (armor && !vk_enemies.armor) vk_candles_armor_out(&vk_candles);
    /* Object 09 (C434): an enemy touched is destroyed, scored as by the whip (81AB),
     * effect 0D (7EBC). Object 08 (C43A): effect 17 when it drops to 10h (75ED). */
    if (vk_enemies.contactScore) vk_hud_add_score((u32)vk_enemies.contactScore * 100);
    if (vk_enemies.itemSound) vk_psg_effect(vk_enemies.itemSound);
    /* 75FA-7601: C43B ran out, D010 bit 0 cleared, FC gives the music back. */
    if (vk_enemies.timeStopEnd) vk_psg_timer_end();
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

/* 790C: while C43A (object 08) or C434 (object 09) runs, every line of Simon's
 * sprites (D400, the lash included) takes colour 0Eh or 8 without the CC bit on the
 * passes where bit 0 of C003 is set: a one-colour silhouette, 2 frames out of 4.
 * The MD sprites switch to PAL3, whose entries 1..3 then hold that colour of the
 * room palette (PAL2, the MSX palette). Returns the palette of Simon's sprites. */
static u16 simon_palette(void)
{
    const u8 color = (u8)(room_key() ? Enemies_simonColor(&vk_enemies) : 0);
    if (!color) return PAL2;
    if (color != simon_flash_color)
    {
        /* Sent in the next VBlank with the sprite table (DMA queue). */
        static u16 rgb[3];
        rgb[0] = rgb[1] = rgb[2] = PAL_getColor((u16)(PAL2 * 16 + color));
        PAL_setColors((u16)(PAL3 * 16 + SIMON_FLASH_FIRST), rgb, 3, DMA_QUEUE);
        simon_flash_color = color;
    }
    return PAL3;
}

/* 7604 (object 05, C43E): BDRCLR F3EB = 0Eh or 0 then CHGCLR. The MD backdrop
 * (register 7, PAL0 = the room palette) stands for the MSX border colour; it shows
 * wherever the planes and sprites are transparent. Called right after the VBlank,
 * so the whole picture changes at once, one frame after the pass, like the sprites. */
void vk_show_backdrop(void)
{
    const u8 color = (u8)(room_key() ? vk_enemies.border : 0);
    if (color == backdrop_color) return;
    VDP_setBackgroundColor(color);
    backdrop_color = color;
}

/* 7618 (end of Simon's routine, 6B36): while C422 = 0 and C701 & 30h (objects
 * 03, 04), C42E < 14h gets +14h and C42F 00-02 / 0F-11 become 1E-20 / 21-23:
 * the poses with the shield. The recoil and death pictures do not change
 * (tools/extract_player.py). Returns the vk_player_shield_tiles pose, or -1. */
static int shield_pose(const VKPlayer *player, bool crouch_mode, int attack_frame)
{
    if (!(vk_candles.c701 & VK_ITEM_SHIELD) || attack_frame || vk_whip.timer) return -1;
    if (crouch_mode) return VK_PLAYER_SHIELD_CROUCH + (player->facing ? 1 : 0);
    if (player->pose >= VK_PLAYER_STAIR_POSE)
        return VK_PLAYER_SHIELD_STAIR + player->pose - VK_PLAYER_STAIR_POSE;
    if (player->pose < VK_PLAYER_RECOIL_POSE) return player->pose;
    return -1;
}

void vk_show_player(const VKPlayer *shown)
{
    /* After a fall Simon is parked at Y FA, whose top rows would still reach the
     * 224-line screen: he is drawn lower, out of sight. */
    VKPlayer parked = *shown;
    const VKPlayer *player = &parked;
    if (parked.y == VK_FALL_Y) parked.y = 255;
    /* 78CC: while C42D runs, Simon's sprites go to Y E0 when bit 1 of the 30 Hz
     * tick C003 is set: 4 frames hidden, 4 shown. C42D drops once a tick too, so
     * its bit 1 gives the same rhythm (PROVISIONAL phase: C003 runs freely). */
    if (vk_enemies.invuln & 2) parked.y = 255;
    const u16 simon_pal = simon_palette();
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
    const int shield = shield_pose(player, crouch_mode, attack_frame);
    if (shield >= 0 && shield != shield_loaded)
    {
        VDP_loadTileData((const u32 *)vk_player_shield_tiles[shield], SHIELD_TILE_INDEX,
                         VK_PLAYER_TILES_PER_POSE, DMA_QUEUE);
        shield_loaded = shield;
    }
    /* Standing body on sprite 0. In crouch mode and during any attack the body
     * is the two-half composition drawn on sprites 5 and 6 (below); the lash
     * is part of those ROM-derived frames, so no separate lash is overlaid. */
    if (shield >= 0)
        /* Shield poses share the 24 x 32 canvas of the normal ones (top y - 31). */
        VDP_setSpriteFull(0, (s16)player->x - 8, (s16)player->y - 31, SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, SHIELD_TILE_INDEX), 1);
    else if (player->pose >= VK_PLAYER_STAIR_POSE && !crouch_mode && !attack_frame)
        VDP_setSpriteFull(0, (s16)player->x - 8, (s16)player->y - 31, SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(simon_pal, TRUE, FALSE, FALSE, STAIR_TILE_INDEX +
                (player->pose - VK_PLAYER_STAIR_POSE) * VK_PLAYER_TILES_PER_POSE), 1);
    else if (player->pose >= VK_PLAYER_DEATH_POSE && player->pose < VK_PLAYER_STAIR_POSE)
    {
        /* 7073: the death pose lies on the floor, 32 x 16 px (4 x 2 tiles). */
        const u16 side = (u16)(player->pose - VK_PLAYER_DEATH_POSE);
        VDP_setSpriteFull(0, (s16)player->x + vk_player_death_left[side], (s16)player->y - 15,
            SPRITE_SIZE(4, 2),
            TILE_ATTR_FULL(simon_pal, TRUE, FALSE, FALSE, DEATH_TILE_INDEX + side * VK_PLAYER_DEATH_TILES), 1);
    }
    else if (crouch_mode || attack_frame)
        VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(simon_pal, TRUE, FALSE, FALSE,
                           PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE), 1);
    else
        /* The recoil poses reach one pixel higher (extract_player.py POSE_TOP). */
        VDP_setSpriteFull(0, (s16) player->x - 8,
            (s16) player->y - (player->pose >= VK_PLAYER_RECOIL_POSE ? 32 : 31), SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(simon_pal, TRUE, FALSE, FALSE,
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
        const bool body = (crouch_mode && shield < 0) || attack_frame;
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
                TILE_ATTR_FULL(simon_pal, TRUE, FALSE, flip,
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

/* Flame and spark tiles with MSX colours 1 and 2 moved to PAL3 entries 4 and 5,
 * so that entries 1..3 stay free for Simon's silhouette (simon_palette). */
static void load_flames(void)
{
    static u32 frame[VK_FLAME_TILES_PER_FRAME * 8];
    u16 f, i;
    for (f = 0; f < VK_FLAME_FRAMES + VK_SPARK_FRAMES; f++)
    {
        for (i = 0; i < VK_FLAME_TILES_PER_FRAME * 8; i++)
        {
            /* All eight pixels at once: bit k of each nibble in bk. */
            const u32 row = vk_flame_fx_tiles[f * VK_FLAME_TILES_PER_FRAME * 8 + i];
            const u32 b0 = row & 0x11111111UL, b1 = (row >> 1) & 0x11111111UL;
            const u32 high = ((row >> 2) | (row >> 3)) & 0x11111111UL;
            const u32 is1 = b0 & ~b1 & ~high;     /* 0001 -> 0100 (FLAME_COLOR_1) */
            const u32 is2 = b1 & ~b0 & ~high;     /* 0010 -> 0101 (FLAME_COLOR_2) */
            frame[i] = row ^ is1 ^ (is1 << 2) ^ (is2 << 1) ^ is2 ^ (is2 << 2);
        }
        VDP_loadTileData(frame, (u16)(FLAME_TILE_INDEX + f * VK_FLAME_TILES_PER_FRAME),
                         VK_FLAME_TILES_PER_FRAME, CPU);
    }
}

void vk_load_whip_graphics(void)
{
    VDP_loadTileData(whip_tiles, WHIP_TILE_INDEX, 3, CPU);
    VDP_loadTileData(arm_tiles, ARM_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *) vk_simon_attack_tiles, ATTACK_TILE_INDEX,
                     VK_ATTACK_FRAME_COUNT * VK_ATTACK_TILES_PER_FRAME, CPU);
    PAL_setPalette(PAL3, attack_palette, CPU);
    VDP_loadTileData(target_tiles, TARGET_TILE_INDEX, 4, CPU);
    load_flames();
    simon_flash_color = 0;
    VDP_loadTileData((const u32 *)vk_simon_crouch_tiles, CROUCH_TILE_INDEX, VK_CROUCH_TILE_COUNT, CPU);
    vk_whip_reset(&vk_whip);
    vk_whip_set_level(&vk_whip, whip_level_choice);
    if (vk_whip_level(&vk_whip) == 1)
        VDP_loadTileData((const u32 *)vk_whip_level1_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    else if (vk_whip_level(&vk_whip) == 2)
        VDP_loadTileData((const u32 *)vk_whip_level2_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    vk_hide_targets();
    last_attack_frame = -1;
    shield_loaded = -1;              /* the scene tiles may have covered the slot */
}

/* Independent on-screen state check for BlastEm, on the diagnostic rows under
 * the 212 MSX lines (row 2 would land in the weapon box of the HUD). */
void vk_draw_attack_frame_label(void)
{
    int frame = vk_whip_arm_frame(&vk_whip);
    if (frame != last_attack_frame)
    {
        last_attack_frame = frame;
        VDP_drawText(frame == 0 ? "F0" :
            (frame == 1 ? "F1" : (frame == 2 ? "F2" :
            (frame == 3 ? "F3" : "F4"))), 30, 27);
    }
}
