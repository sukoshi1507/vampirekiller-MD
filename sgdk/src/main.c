#include <genesis.h>
#include "input.h"
#include "generated/rooms.h"
#include "player.h"
#include "gameplay.h"
#include "room_manager.h"
#include "whip.h"
#include "targets.h"
#include "generated/player_assets.h"
#include "generated/simon_attack_md.h"
#include "generated/whip_level1_md.h"
#include "generated/whip_level2_md.h"
#include "generated/simon_crouch_md.h"
#include "generated/flame_fx_md.h"
#include "generated/intro_assets.h"
#include "sound_test.h"
#include "psg_player.h"
#include "generated/sfx_md.h"

/* Room explorer plus a movement prototype in the first room. */
static const u16 hud_palette[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0EEE
};

static void show_room(u16 stage, u16 room, bool atlas)
{
    const VKStage *group = &vk_stages[stage - 1];
    const u16 index = group->first + room;
    char label[40];
    u16 i;

    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    VDP_loadTileData(vk_tiles[group->tileset], TILE_USER_INDEX, VK_TILE_COUNT, CPU);
    PAL_setPalette(PAL0, vk_palettes[index], CPU);
    PAL_setPalette(PAL1, hud_palette, CPU);
    VDP_setTextPalette(PAL1);
    sprintf(label, "STAGE %02u  ROOM %02u/%02u", stage, room, group->count - 1);
    VDP_drawText(label, 3, 1);
    VDP_drawText("B PLAY | START RESET", 6, 2);
    if (atlas)
    {
        VDP_drawText("TILESET / 192 TILES", 5, 4);
        for (i = 0; i < VK_TILE_COUNT; i++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                             TILE_USER_INDEX + i), 8 + (i % 16), 6 + (i / 16));
        VDP_drawText("STATIC ASSETS / NO GAMEPLAY", 3, 22);
    }
    else
        VDP_setTileMapDataRectEx(BG_A, vk_maps[index],
            TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
            0, 4, 32, 22, 32, CPU);
    VDP_drawText("L/R ROOM  U/D STAGE  A TILES", 2, 27);
    VDP_setEnable(TRUE);
}

#define PLAYER_TILE_INDEX 1024
#define WHIP_TILE_INDEX (PLAYER_TILE_INDEX + VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE)
#define ARM_TILE_INDEX (WHIP_TILE_INDEX + 3)
#define ATTACK_TILE_INDEX (ARM_TILE_INDEX + 4)

static VKGame game;
static VKPlayer player;
static VKWhip whip;
static VKTarget test_target;
static bool target_visible = FALSE;
static u16 diagnostic_timer;
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

enum { FRONT_LOGO, FRONT_TITLE, FRONT_CASTLE, FRONT_COURTYARD,
       FRONT_INTERIOR, FRONT_VIEWER, FRONT_TEST, FRONT_SOUND };
static u16 front_mode;

/* 16x16 training target, deliberately NOT an original ROM asset. */
#define TARGET_TILE_INDEX (ATTACK_TILE_INDEX + VK_ATTACK_FRAME_COUNT * VK_ATTACK_TILES_PER_FRAME)
#define FLAME_TILE_INDEX (TARGET_TILE_INDEX + 4)
#define CROUCH_TILE_INDEX (FLAME_TILE_INDEX + (VK_FLAME_FRAMES + VK_SPARK_FRAMES) * VK_FLAME_TILES_PER_FRAME)
static const u32 target_tiles[32] = {
    0,0,0x000FF000UL,0x00F00F00UL,0x0F0000F0UL,0x0F0000F0UL,0xF000000FUL,0xF000000FUL,
    0,0,0x000FF000UL,0x00F00F00UL,0x0F0000F0UL,0x0F0000F0UL,0xF000000FUL,0xF000000FUL,
    0xF000000FUL,0xF000000FUL,0x0F0000F0UL,0x0F0000F0UL,0x00F00F00UL,0x000FF000UL,0,0,
    0xF000000FUL,0xF000000FUL,0x0F0000F0UL,0x0F0000F0UL,0x00F00F00UL,0x000FF000UL,0,0
};
/* Prototype flame/impact animation generated from the V15 artwork.
 * Source and provenance: tools/build_flame_fx.py, NOT the original MSX ROM. */
static u16 flame_clock;
/* Courtyard flames, per screen (rooms 0..2 = scenes 3..5). Each entry is one
 * real torch flame: its top-left corner, and the baked torch cells it covers in
 * the background (see set_courtyard_flame_background). Rooms 0 and 1 have torches
 * A and B (openMSX: torch B at x 192 is animated and extinguished by the whip in
 * room 1, with the same after-hit VRAM as torch A); room 2 shows only torch A. */
#define COURTYARD_ROOMS 3
#define COURTYARD_MAX_FLAMES 2
#define FLAME_CELL_ROW 18
static const u16 courtyard_flame_count[COURTYARD_ROOMS] = { 2, 2, 1 };
static const s16 courtyard_flame_x[COURTYARD_ROOMS][COURTYARD_MAX_FLAMES] = {
    { 64, 192 }, { 64, 192 }, { 64, 0 }
};
static const s16 courtyard_flame_y[COURTYARD_ROOMS][COURTYARD_MAX_FLAMES] = {
    { 144, 144 }, { 144, 144 }, { 144, 0 }
};
/* First baked cell column of each flame (both 2x2 cells, rows 18..19). */
static const u16 flame_cell_col[COURTYARD_MAX_FLAMES] = { 8, 24 };
static VKTarget courtyard_flame[COURTYARD_MAX_FLAMES];
static u16 courtyard_room;
/* Torch background once its flame is out, for torch A (cells 8..9) and torch B
 * (cells 24..25), rows 18..19. The MSX2 reference (openMSX VRAM after the hit)
 * keeps the brazier and clears only the flame. The four 8x8 tiles, in the order
 * TL, TR, BL, BR, are the same for both torches: they were converted from the
 * after-hit VRAM of torch A (validated) and of torch B. Pixels come from the
 * openMSX dump, not from the scene tile set. Index 1400 lies beyond the scene
 * tiles (user index up to ~313) and the sprite tiles (1024..1359). */
#define FLAME_A_CLEAR_TILE_INDEX 1400
/* Standing frames of whip levels 1 and 2 (6 frames: 3 right, 3 left, see
 * whip_levels_manifest.json). Loaded only when such a level is selected; level 0
 * keeps its frames at ATTACK_TILE_INDEX. */
#define WHIP_LEVEL_TILE_INDEX (FLAME_A_CLEAR_TILE_INDEX + 4)
#define WHIP_LEVEL_TILE_COUNT (VK_WHIP_LEVEL1_FRAME_COUNT * VK_WHIP_LEVEL1_TILES_PER_FRAME)
/* Whip level chosen for the next load (0 by default). No input binds it yet, so
 * the wired gameplay keeps the level 0 frames and the level 0 hit box. */
static int whip_level_choice = 0;
static const u32 flame_a_cleared_tiles[32] = {
    0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL,
    0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL,
    0xCEC0CEC0UL, 0xDDD0DDD0UL, 0xDDD2DDD2UL, 0x02000200UL, 0x0C000C00UL, 0x0D000D00UL, 0xCDC0CDC0UL, 0x0C000C00UL,
    0xCEC0CEC0UL, 0xDDD0DDD0UL, 0xDDD2DDD2UL, 0x02000200UL, 0x0C000C00UL, 0x0D000D00UL, 0xCDC0CDC0UL, 0x0C000C00UL,
};
/* Background cells of flame 'flame' (0 = torch A, 1 = torch B): lit restores
 * the baked scene cells, unlit swaps in the four cleared tiles. */
static void set_courtyard_flame_background(u16 flame, int lit)
{
    const u16 scene = 3 + courtyard_room;
    u16 i;
    for (i = 0; i < 4; i++)
    {
        const u16 col = flame_cell_col[flame] + (i & 1);
        const u16 row = FLAME_CELL_ROW + (i >> 1);
        VDP_setTileMapXY(BG_A, lit ?
            TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX + vk_scenes[scene].map[row * 32 + col]) :
            TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, FLAME_A_CLEAR_TILE_INDEX + i), col, row);
    }
}

/* Every real flame of the current courtyard screen is lit again, with its
 * baked cells restored. Unused slots stay inactive and off the hit test. */
static void reset_courtyard_flames(void)
{
    u16 k;
    for (k = 0; k < COURTYARD_MAX_FLAMES; k++)
    {
        const int present = k < courtyard_flame_count[courtyard_room];
        vk_target_init(&courtyard_flame[k], courtyard_flame_x[courtyard_room][k],
                       courtyard_flame_y[courtyard_room][k], 16, 16);
        courtyard_flame[k].active = (VKByte)present;
        if (present) set_courtyard_flame_background(k, TRUE);
    }
}

static void reset_training_target(void)
{
    if (front_mode == FRONT_COURTYARD)
        reset_courtyard_flames();
    else
        vk_target_init(&test_target, (int)player.x + 24, (int)player.y - 24, 16, 16);
    target_visible = TRUE;
    flame_clock = 0;
}
static void show_player(const VKPlayer *player)
{
    const int attack_frame = vk_whip_arm_frame(&whip);
    const int crouch_mode = !player->on_stairs && (player->crouched || (attack_frame && whip.crouched));
    const u16 crouch_frame = attack_frame ? (u16)vk_whip_crouch_visual_step(&whip) : 0;
    const u16 crouch_facing = attack_frame ? whip.facing : player->facing;
    /* Standing body on sprite 0. In crouch mode and during any attack the body
     * is the two-half composition drawn on sprites 5 and 6 (below); the lash
     * is part of those ROM-derived frames, so no separate lash is overlaid. */
    if (crouch_mode || attack_frame)
        VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE,
                           PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE), 1);
    else
        VDP_setSpriteFull(0, (s16) player->x - 8, (s16) player->y - 31, SPRITE_SIZE(3, 4),
            TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE,
                           PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE), 1);
    {
        int left = 0, top = 0, right = 0, bottom = 0;
        /* The MSX2 ROM composition already contains the visible lash.
         * Keep gameplay reach in whip.c, but never overlay the old synthetic
         * three-segment graphics on top of the ROM sprite. */
        int count = 0;
        int segment;
        if (count) vk_whip_bounds(&whip, &left, &top, &right, &bottom);
        for (segment = 0; segment < 3; segment++)
        {
            s16 sx = -32;
            s16 sy = -32;
            u16 tile = WHIP_TILE_INDEX;
            if (segment < count)
            {
                /* Sprite positions and collision reach now match exactly.
                 * Left-facing attacks are drawn from Simon outwards. */
                sx = (s16)(whip.facing ? right - 8 - segment * 8 : left + segment * 8);
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
        int frame = vk_whip_arm_frame(&whip);
        VDP_setSpriteFull(4, -32, -32, SPRITE_SIZE(1, 1),
            TILE_ATTR_FULL(PAL3, TRUE, FALSE, whip.facing,
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
        const u16 frame_index = attack_frame ? (u16)vk_whip_visual_step(&whip) : 0;
        /* Levels 1 and 2 hold their left-facing standing frames (3..5) as decoded
         * pictures, so they are drawn without flip. Crouch frames stay on level 0. */
        const bool level_frames = !crouch_mode && attack_frame && vk_whip_level(&whip) != 0;
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
                i == 0 ? 6 : (target_visible ? 7 : 0));
        }
    }
    if (target_visible && front_mode == FRONT_COURTYARD)
    {
        /* Lit courtyard flames animate at their torch positions. A hit flame is
         * out at once (no spark, as on MSX2) and stays out until A+B. */
        const u16 cycle = (flame_clock >> 3) & (VK_FLAME_FRAMES - 1);
        u16 k;
        for (k = 0; k < COURTYARD_MAX_FLAMES; k++)
        {
            const VKTarget *flame = &courtyard_flame[k];
            VDP_setSpriteFull((u16)(7 + k),
                flame->active ? (s16)flame->x : -32,
                flame->active ? (s16)flame->y : -32, SPRITE_SIZE(2, 2),
                TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE,
                    FLAME_TILE_INDEX + cycle * VK_FLAME_TILES_PER_FRAME),
                k == COURTYARD_MAX_FLAMES - 1 ? 0 : (u16)(8 + k));
        }
        VDP_updateSprites(7 + COURTYARD_MAX_FLAMES, DMA_QUEUE);
        return;
    }
    if (target_visible)
    {
        VDP_setSpriteFull(7, test_target.active ? (s16)test_target.x : -32,
            test_target.active ? (s16)test_target.y : -32, SPRITE_SIZE(2, 2),
            TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, TARGET_TILE_INDEX), 0);
    }
    VDP_updateSprites(target_visible ? 8 : 7, DMA_QUEUE);
}

static void load_whip_graphics(void)
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
    vk_whip_reset(&whip);
    vk_whip_set_level(&whip, whip_level_choice);
    if (vk_whip_level(&whip) == 1)
        VDP_loadTileData((const u32 *)vk_whip_level1_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    else if (vk_whip_level(&whip) == 2)
        VDP_loadTileData((const u32 *)vk_whip_level2_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    target_visible = FALSE;
    last_attack_frame = -1;
}

static void enter_play(VKPlayer *player)
{
    show_room(1, 0, FALSE);
    vk_player_reset(player);
    VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                    VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_palettes[0], CPU);
    load_whip_graphics();
    reset_training_target();
    VDP_drawText("FIRST ROOM MOVEMENT TEST  ", 3, 1);
    VDP_drawText(" ROM ATTACK CANDIDATES TEST   ", 2, 2);
    VDP_drawText(" L/R MOVE  C JUMP  B EXIT    ", 2, 27);
    show_player(player);
}

static u16 front_timer;

static void show_scene(u16 index)
{
    const VKScene *scene = &vk_scenes[index];
    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    VDP_loadTileData(scene->tiles, TILE_USER_INDEX, scene->count, CPU);
    PAL_setPalette(PAL0, scene->palette, CPU);
    PAL_setPalette(PAL1, hud_palette, CPU);
    VDP_setTextPalette(PAL1);
    VDP_setTileMapDataRectEx(BG_A, scene->map,
        TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, 32, 28, 32, CPU);
    VDP_setEnable(TRUE);
}

static void show_title(void)
{
    u16 x;
    show_scene(1);
    /* The ROM's keyboard prompt is adapted to the Mega Drive controller. */
    for (x = 9; x < 24; x++) VDP_setTileMapXY(BG_A, 0, x, 20);
    VDP_drawText("PRESS START", 10, 20);
    front_mode = FRONT_TITLE;
    front_timer = 0;
}

static void show_castle_frame(u16 frame)
{
    u16 i;
    for (i = 0; i < 8; i++)
    {
        const VKIntroSprite *sprite = &vk_intro_frames[frame][i];
        VDP_setSpriteFull(i, sprite->x, sprite->y, SPRITE_SIZE(2, 2),
            TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, PLAYER_TILE_INDEX + sprite->tile),
            i == 7 ? 0 : i + 1);
    }
    VDP_updateSprites(8, DMA_QUEUE);
}

static void begin_castle(void)
{
    show_scene(2);
    VDP_loadTileData(vk_intro_sprite_tiles, PLAYER_TILE_INDEX, vk_intro_sprite_tile_count, CPU);
    show_castle_frame(0);
    front_mode = FRONT_CASTLE;
    front_timer = 0;
}

static void load_actor(u16 scene)
{
    VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                    VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_scenes[scene].palette, CPU);
    load_whip_graphics();
    if (scene == 6 || (scene >= 3 && scene <= 5)) reset_training_target();
    show_player(&player);
}

static void show_courtyard(u16 room)
{
    courtyard_room = room;
    front_mode = FRONT_COURTYARD;
    show_scene(3 + room);
    player = game.player;
    load_actor(3 + room);
    VDP_loadTileData(flame_a_cleared_tiles, FLAME_A_CLEAR_TILE_INDEX, 4, CPU);
    VDP_drawText(room == 2 ? "B FOUET C SAUT UP PORTE" : "B FOUET  A+B FLAMME  C SAUT", 2, 27);
}

static void show_interior(void);
/* Stage-1 playable room loader: common VRAM/player setup for test navigation.
 * The room graph is intentionally marked experimental. */
static void show_stage1_room(u16 room)
{
    if (room == 0)
    {
        show_interior();
        return;
    }
    show_room(1, room, FALSE);
    player = game.player;
    VDP_loadTileData((const u32 *)vk_player_tiles, PLAYER_TILE_INDEX,
        VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_palettes[vk_stages[0].first + room], CPU);
    load_whip_graphics();
    reset_training_target();
    VDP_drawText("PARCOURS TEST / LIENS NON MSX2", 1, 1);
    VDP_drawText(room == 1 ? "A+HAUT ESCALIER B FOUET" :
        "B FOUET / A+L-R SALLE", 1, 27);
    show_player(&player);
    front_mode = FRONT_INTERIOR;
}

static void show_interior(void)
{
    show_scene(6);
    player = game.player;
    /* Set the mode first: the hall target must not be placed as a courtyard flame. */
    front_mode = FRONT_INTERIOR;
    load_actor(6);
    VDP_drawText("HALL: ORIGINAL MSX2 COLLISIONS", 1, 1);
    VDP_drawText("B FOUET A+B CIBLE C SAUT", 1, 27);
    front_mode = FRONT_INTERIOR;
}

static void logo_palette(u16 level)
{
    /* DMA_QUEUE keeps the pointer until the VBlank flush: the buffer must be static. */
    static u16 colors[16];
    u16 i;
    for (i = 0; i < 16; i++)
    {
        const u16 source = vk_scenes[0].palette[i];
        u16 r = ((source >> 1) & 7) * level / 16;
        u16 g = ((source >> 5) & 7) * level / 16;
        u16 b = ((source >> 9) & 7) * level / 16;
        colors[i] = (b << 9) | (g << 5) | (r << 1);
    }
    PAL_setPalette(PAL0, colors, DMA_QUEUE);
}

int main(bool hardReset)
{
    VKInput input = {0};
    u16 stage = 1;
    u16 room = 0;
    bool atlas = FALSE;
    (void) hardReset;
    VDP_setScreenWidth256();
    VDP_setScreenHeight224();
    JOY_init();
    front_mode = FRONT_LOGO;
    front_timer = 0;
    show_scene(0);
    logo_palette(0);
    while (TRUE)
    {
        u16 pressed;
        bool dirty = FALSE;
        handleInput(&input);
        pressed = input.pressed;
        if (front_mode == FRONT_LOGO)
        {
            front_timer++;
            if (pressed & BUTTON_START || front_timer >= 120) show_title();
            else if (front_timer <= 16) logo_palette(front_timer);
            else if (front_timer > 104) logo_palette(120 - front_timer);
        }
        else if (front_mode == FRONT_TITLE)
        {
            if (pressed & BUTTON_START) begin_castle();
            else if (vk_konami_step(pressed)) { vk_sound_test_enter(); front_mode = FRONT_SOUND; }
            else if (pressed & BUTTON_A)
            {
                stage = 1; room = 0; atlas = FALSE;
                show_room(stage, room, atlas); front_mode = FRONT_VIEWER;
            }
        }
        else if (front_mode == FRONT_CASTLE)
        {
            if (pressed & BUTTON_START || ++front_timer >= 320) { vk_game_begin(&game); show_courtyard(0); }
            else show_castle_frame(front_timer);
        }
        else if (front_mode == FRONT_COURTYARD || front_mode == FRONT_INTERIOR)
        {
            const int direction = input.direction;
            if (pressed & BUTTON_START) show_title();
            else
            {
                if (front_mode == FRONT_INTERIOR && (input.keys & BUTTON_A) &&
                    (pressed & (BUTTON_RIGHT | BUTTON_LEFT)))
                {
                    int target = (int)game.room + ((pressed & BUTTON_RIGHT) ? 1 : -1);
                    if (target >= 0 && target < 8 && vk_game_debug_room(&game, (VKByte)target))
                    {
                        show_stage1_room((u16)target);
                    }
                    SYS_doVBlankProcess();
                    continue;
                }
                if ((front_mode == FRONT_INTERIOR || front_mode == FRONT_COURTYARD) &&
                    (input.keys & BUTTON_A) && (pressed & BUTTON_B))
                {
                    reset_training_target();
                    VDP_drawText(front_mode == FRONT_COURTYARD ?
                        "FLAMME RALLUMEE! " : "CIBLE REINITIALISEE", 8, 3);
                }
                const VKEvent event = vk_game_tick_controls(&game, direction,
                    input.jump_pressed, !!(input.keys & BUTTON_UP),
                    !!(input.keys & BUTTON_DOWN));
                player = game.player;
                {
                    const bool idle = whip.timer == 0;
                    vk_whip_tick(&whip, &player, !!((pressed & BUTTON_B) && !(input.keys & BUTTON_A)));
                    if (idle && whip.timer == 14) vk_psg_play(vk_sfx_whip);
                }
                if (target_visible && front_mode == FRONT_COURTYARD)
                {
                    const int flame = vk_targets_tick(courtyard_flame,
                        courtyard_flame_count[courtyard_room], &whip);
                    if (flame >= 0)
                    {
                        set_courtyard_flame_background((u16)flame, FALSE);
                        VDP_drawText("FLAMME ETEINTE! ", 8, 3);
                    }
                }
                else if (target_visible && vk_target_tick(&test_target, &whip))
                    VDP_drawText("CIBLE TOUCHEE!  ", 8, 3);
                /* Independent on-screen state check for BlastEm. */
                {
                    int frame = vk_whip_arm_frame(&whip);
                    if (frame != last_attack_frame)
                    {
                        last_attack_frame = frame;
                        VDP_drawText(frame == 0 ? "F0" :
                            (frame == 1 ? "F1" : (frame == 2 ? "F2" :
                            (frame == 3 ? "F3" : "F4"))), 28, 2);
                    }
                }
                /* Make controller/attack state observable in BlastEm. No text
                 * is reprinted every frame: refresh on input or at timeout. */
                if (pressed & BUTTON_B)
                {
                    diagnostic_timer = 40;
                    VDP_drawText(player.on_stairs ? "B: BLOQUE SUR ESCALIER" :
                        "B: ATTAQUE DETECTEE    ", 1, 2);
                }
                else if (diagnostic_timer && --diagnostic_timer == 0)
                    VDP_drawText("                         ", 1, 2);
                if (event == VK_EVENT_ENTER) show_interior();
                else if (event == VK_EVENT_ROOM) show_courtyard(game.room);
                else if (event == VK_EVENT_VERTICAL) show_stage1_room(game.room);
                else if (front_mode == FRONT_INTERIOR && direction &&
                         !(input.keys & BUTTON_A) &&
                         vk_room_try_edge(&game.room, &game.player, direction))
                {
                    /* Keep player state across edge; collision map changes on
                     * next gameplay tick. No fabricated vertical connections. */
                    player = game.player;
                    show_stage1_room(game.room);
                }
                else { ++flame_clock; show_player(&player); }
            }
        }
        else if (front_mode == FRONT_SOUND)
        {
            if (vk_sound_test_tick(pressed)) show_title();
        }
        else if (front_mode == FRONT_TEST)
        {
            if (pressed & BUTTON_B)
            { show_room(1, 0, FALSE); front_mode = FRONT_VIEWER; }
            else if (pressed & BUTTON_START) enter_play(&player);
            else
            {
                const int direction = input.direction;
                vk_player_tick(&player, direction, input.jump_pressed);
                vk_whip_tick(&whip, &player, !!(pressed & BUTTON_A));
                if (target_visible && vk_target_tick(&test_target, &whip))
                    VDP_drawText("CIBLE TOUCHEE!", 8, 3);
                show_player(&player);
            }
        }
        else /* The asset explorer remains available from the title with A. */
        {
            if (pressed & BUTTON_B)
            { enter_play(&player); front_mode = FRONT_TEST; }
            else
            {
                if (pressed & BUTTON_UP)
                { stage = stage == 18 ? 1 : stage + 1; room = 0; dirty = TRUE; }
                else if (pressed & BUTTON_DOWN)
                { stage = stage == 1 ? 18 : stage - 1; room = 0; dirty = TRUE; }
                if (pressed & BUTTON_RIGHT)
                { room = (room + 1) % vk_stages[stage - 1].count; dirty = TRUE; }
                else if (pressed & BUTTON_LEFT)
                { room = room == 0 ? vk_stages[stage - 1].count - 1 : room - 1; dirty = TRUE; }
                if (pressed & BUTTON_A) { atlas = !atlas; dirty = TRUE; }
                if (pressed & BUTTON_START) { stage = 1; room = 0; atlas = FALSE; dirty = TRUE; }
                if (dirty) show_room(stage, room, atlas);
            }
        }
        vk_psg_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
