#include <genesis.h>
#include "input.h"
#include "generated/rooms.h"
#include "player.h"
#include "gameplay.h"
#include "whip.h"
#include "generated/player_assets.h"
#include "generated/simon_attack_md.h"
#include "generated/intro_assets.h"

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

static VKWhip whip;
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
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00CA,0x0EEE
};

static void show_player(const VKPlayer *player)
{
    const int attack_frame = vk_whip_arm_frame(&whip);
    /* ROM-derived body frames and the lash are complementary layers. */
    VDP_setSpriteFull(0, attack_frame ? -32 : (s16) player->x - 8,
        attack_frame ? -32 : (s16) player->y - 31, SPRITE_SIZE(3, 4),
        TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE,
                       PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE), 1);
    {
        int left = 0, top = 0, right = 0, bottom = 0;
        int count = vk_whip_segments(&whip);
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
    /* Retired placeholder arm: ROM-derived body already contains the arm. */
    {
        int ax = -32, ay = -32;
        int frame = vk_whip_arm_frame(&whip);
        if (frame && !attack_frame)
        {
            vk_whip_arm_position(&whip, &ax, &ay);
        }
        VDP_setSpriteFull(4, (s16)ax, (s16)ay, SPRITE_SIZE(1, 1),
            TILE_ATTR_FULL(PAL3, TRUE, FALSE, whip.facing,
                           ARM_TILE_INDEX + (frame ? (u16)(frame - 1) : 0)), 5);
    }
    /* Two 32x32 hardware sprites cover a 64x32 anchored composition.
     * The decoded candidate is already right-facing: player/whip facing == 0
     * means right, facing == 1 means left. Only left-facing attacks require
     * horizontal reflection and reversal of the source half order.
     * Phase-to-index mapping is provisional pending a live MSX2 trace. */
    {
        u16 i;
        const u16 frame_index = attack_frame ? (u16)(attack_frame - 1) : 0;
        const u16 base = ATTACK_TILE_INDEX + frame_index * VK_ATTACK_TILES_PER_FRAME;
        for (i = 0; i < 2; ++i)
        {
            const u16 source_half = whip.facing ? (u16)(1 - i) : i;
            VDP_setSpriteFull((u16)(5 + i),
                attack_frame ? (s16)player->x - 32 + (s16)i * 32 : -32,
                attack_frame ? (s16)player->y - 30 : -32,
                SPRITE_SIZE(4, 4),
                TILE_ATTR_FULL(PAL2, TRUE, FALSE, whip.facing,
                               base + source_half * VK_ATTACK_TILES_PER_HALF),
                i == 0 ? 6 : 0);
        }
    }
    VDP_updateSprites(7, CPU);
}

static void load_whip_graphics(void)
{
    VDP_loadTileData(whip_tiles, WHIP_TILE_INDEX, 3, CPU);
    VDP_loadTileData(arm_tiles, ARM_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *) vk_simon_attack_tiles, ATTACK_TILE_INDEX,
                     VK_ATTACK_FRAME_COUNT * VK_ATTACK_TILES_PER_FRAME, CPU);
    PAL_setPalette(PAL3, attack_palette, CPU);
    vk_whip_reset(&whip);
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
    VDP_drawText("FIRST ROOM MOVEMENT TEST  ", 3, 1);
    VDP_drawText(" ROM ATTACK CANDIDATES TEST   ", 2, 2);
    VDP_drawText(" L/R MOVE  C JUMP  B EXIT    ", 2, 27);
    show_player(player);
}

static VKPlayer player;
static VKGame game;
enum { FRONT_LOGO, FRONT_TITLE, FRONT_CASTLE, FRONT_COURTYARD,
       FRONT_INTERIOR, FRONT_VIEWER, FRONT_TEST };
static u16 front_mode;
static u16 front_timer;
static u16 courtyard_room;

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
    VDP_updateSprites(8, CPU);
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
    show_player(&player);
}

static void show_courtyard(u16 room)
{
    courtyard_room = room;
    show_scene(3 + room);
    player = game.player;
    load_actor(3 + room);
    VDP_drawText(room == 2 ? "L/R MOVE  C JUMP  UP ENTER" : "L/R MOVE C JUMP START TITLE", 2, 27);
    front_mode = FRONT_COURTYARD;
}

static void show_interior(void)
{
    show_scene(6);
    player = game.player;
    load_actor(6);
    VDP_drawText("HALL: ORIGINAL MSX2 COLLISIONS", 1, 1);
    VDP_drawText("B FOUET C SAUT A+L/R SALLES", 1, 27);
    front_mode = FRONT_INTERIOR;
}

static void logo_palette(u16 level)
{
    u16 colors[16];
    u16 i;
    for (i = 0; i < 16; i++)
    {
        const u16 source = vk_scenes[0].palette[i];
        u16 r = ((source >> 1) & 7) * level / 16;
        u16 g = ((source >> 5) & 7) * level / 16;
        u16 b = ((source >> 9) & 7) * level / 16;
        colors[i] = (b << 9) | (g << 5) | (r << 1);
    }
    PAL_setPalette(PAL0, colors, CPU);
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
        SYS_doVBlankProcess();
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
                        show_room(1, (u16)target, FALSE);
                        player = game.player;
                        VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                            VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
                        PAL_setPalette(PAL2, vk_palettes[vk_stages[0].first + target], CPU);
                        load_whip_graphics();
                        VDP_drawText(target == 1 ? "A+HAUT: ESCALIER  B: FOUET" :
                            "A+L/R SALLE  B: FOUET", 1, 27);
                        show_player(&player);
                    }
                    continue;
                }
                /* Demo-only stair placement. A+UP on stage-1 room-1 puts Simon
                 * inside the authored stair test area; normal collision tiles
                 * otherwise prevent reaching the strip reliably. */
                if (front_mode == FRONT_INTERIOR && game.room == 1 &&
                    (input.keys & BUTTON_A) && (pressed & BUTTON_UP))
                {
                    game.player.x = 128;
                    game.player.y = 176;
                    game.player.mode = 0;
                    game.player.phase = 0;
                    game.player.on_stairs = 1;
                    game.player.clock = 0;
                    game.player.stair_anim = 0;
                    game.player.crouched = 0;
                    VDP_drawText("ESCALIER DEMO: HAUT / BAS", 1, 1);
                }
                const VKEvent event = vk_game_tick_controls(&game, direction,
                    input.jump_pressed, !!(input.keys & BUTTON_UP),
                    !!(input.keys & BUTTON_DOWN));
                player = game.player;
                vk_whip_tick(&whip, &player, !!(pressed & BUTTON_B));
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
                else show_player(&player);
            }
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
    }
    return 0;
}
