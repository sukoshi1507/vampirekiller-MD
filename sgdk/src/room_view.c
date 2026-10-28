#include "room_view.h"
#include "frontend.h"
#include "actors.h"
#include "targets.h"
#include "candle_view.h"
#include "hud.h"
#include "generated/rooms.h"
#include "generated/intro_assets.h"

/* Text colour (15) and the HUD bar colours of hud.c: PLAYER fill (1), ENEMY
 * fill (8) and frame (14), the values of ROM scenes 3..6 at those indices. */
static const u16 hud_palette[16] = {
    0, 0x08AE, 0, 0, 0, 0, 0, 0, 0x020E, 0, 0, 0, 0, 0, 0x0EEE, 0x0EEE
};
/* Test note of the current play room, on row 26 below the playfield (rows 1..3
 * hold the HUD bars). The B diagnostic of main.c shares the row. */
static const char *play_note = "";

void vk_show_room(u16 stage, u16 room, bool atlas)
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

void vk_show_scene(u16 index)
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

static VKTarget test_target;
static bool target_visible = FALSE;
/* Prototype flame/impact animation generated from the V15 artwork.
 * Source and provenance: tools/build_flame_fx.py, NOT the original MSX ROM. */
static u16 flame_clock;
/* Courtyard flames, per screen (rooms 0..2 = scenes 3..5). Each entry is one
 * real torch flame: its top-left corner, and the baked torch cells it covers in
 * the background (see set_courtyard_flame_background). Rooms 0 and 1 have torches
 * A and B (openMSX: torch B at x 192 is animated and extinguished by the whip in
 * room 1, with the same after-hit VRAM as torch A); room 2 shows only torch A. */
#define COURTYARD_ROOMS 3
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
/* Debug overlay with the internal room number, toggled by the Konami code in play.
 * The key is what was last drawn; 0xFFFF forces a redraw after a scene change. */
static bool debug_room_label;
static u16 debug_room_key = 0xFFFF;
/* Torch background once its flame is out, for torch A (cells 8..9) and torch B
 * (cells 24..25), rows 18..19, at FLAME_A_CLEAR_TILE_INDEX (actors.h). The MSX2
 * reference (openMSX VRAM after the hit) keeps the brazier and clears only the
 * flame. The four 8x8 tiles, in the order TL, TR, BL, BR, are the same for both
 * torches: they were converted from the after-hit VRAM of torch A (validated)
 * and of torch B. Pixels come from the openMSX dump, not from the scene tile set. */
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

/* Every flame of the current courtyard screen that candles.c still counts as lit
 * is shown, with its baked cells restored; a torch already put out keeps its
 * cleared cells. Unused slots stay inactive. */
static void reset_courtyard_flames(void)
{
    u16 k;
    vk_candles_enter(&vk_candles, &vk_game);
    for (k = 0; k < COURTYARD_MAX_FLAMES; k++)
    {
        const int present = k < courtyard_flame_count[courtyard_room];
        const int lit = present && vk_candles_lit(&vk_candles, k);
        vk_target_init(&courtyard_flame[k], courtyard_flame_x[courtyard_room][k],
                       courtyard_flame_y[courtyard_room][k], 16, 16);
        courtyard_flame[k].active = (VKByte)lit;
        if (present) set_courtyard_flame_background(k, lit);
    }
}

void vk_reset_training_target(void)
{
    if (vk_front_mode == FRONT_COURTYARD)
        reset_courtyard_flames();
    else
        vk_target_init(&test_target, (int)vk_player.x + 24, (int)vk_player.y - 24, 16, 16);
    /* The castle rooms have their real candles (candle_view.c): no training target. */
    target_visible = vk_front_mode != FRONT_INTERIOR;
    flame_clock = 0;
}

void vk_hide_targets(void) { target_visible = FALSE; }
bool vk_targets_visible(void) { return target_visible; }
void vk_flames_tick(void) { ++flame_clock; }

void vk_courtyard_flame_out(int flame)
{
    if (vk_front_mode != FRONT_COURTYARD || flame < 0 || flame >= courtyard_flame_count[courtyard_room])
        return;
    courtyard_flame[flame].active = 0;
    set_courtyard_flame_background((u16)flame, FALSE);
}

bool vk_training_target_hit(const VKWhip *whip)
{
    return target_visible && vk_target_tick(&test_target, whip);
}

bool vk_draw_targets(u16 enemy_link)
{
    if (target_visible && vk_front_mode == FRONT_COURTYARD)
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
                k == COURTYARD_MAX_FLAMES - 1 ? enemy_link : (u16)(8 + k));
        }
        return TRUE;
    }
    if (target_visible)
    {
        VDP_setSpriteFull(7, test_target.active ? (s16)test_target.x : -32,
            test_target.active ? (s16)test_target.y : -32, SPRITE_SIZE(2, 2),
            TILE_ATTR_FULL(PAL3, TRUE, FALSE, FALSE, TARGET_TILE_INDEX), enemy_link);
    }
    return FALSE;
}

void vk_enter_play(VKPlayer *player)
{
    vk_show_room(1, 0, FALSE);
    vk_player_reset(player);
    VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                    VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_palettes[0], CPU);
    vk_load_whip_graphics();
    vk_reset_training_target();
    VDP_drawText("FIRST ROOM MOVEMENT TEST  ", 3, 1);
    VDP_drawText(" ROM ATTACK CANDIDATES TEST   ", 2, 2);
    VDP_drawText(" L/R MOVE  C JUMP  B EXIT    ", 2, 27);
    vk_show_player(player);
}

static void load_actor(u16 scene)
{
    VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                    VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_scenes[scene].palette, CPU);
    vk_load_whip_graphics();
    if (scene == 6 || (scene >= 3 && scene <= 5)) vk_reset_training_target();
    vk_show_player(&vk_player);
}

void vk_draw_play_note(void)
{
    VDP_clearText(1, 26, 30);
    VDP_drawText(play_note, 1, 26);
}

/* HUD bars and test note of a play room, drawn last over the scene. */
static void show_play_overlay(const char *note, bool hud_labels)
{
    play_note = note;
    vk_draw_play_note();
    vk_hud_show(hud_labels);
    vk_hud_update(vk_enemies.energy);
}

void vk_debug_label_toggle(void) { debug_room_label = !debug_room_label; }

/* Draws "COUR SALLE n" or "ETAPE s SALLE n" at the top right while the debug
 * overlay is on, and clears it when off. Only redraws when the key changes. */
void vk_debug_label_draw(void)
{
    const int courtyard = vk_game.area == VK_AREA_COURTYARD;
    const u16 key = debug_room_label ?
        (u16)(1 + (courtyard ? 0 : 8 * vk_game.stage) + vk_game.room) : 0;
    char hall[16] = "ETAPE 1 SALLE 0";
    char yard[16] = "COUR SALLE 0";
    const u16 previous = debug_room_key;
    if (key == previous) return;
    debug_room_key = key;
    /* Fresh scene with the overlay off: keep the baked HUD (STAGE, hearts, P). */
    if (!key && previous == 0xFFFF) return;
    /* The screen is 32 columns wide (VDP_setScreenWidth256): the longest label,
     * 15 characters, starts at column 16 to end before column 31. */
    VDP_drawText("               ", 16, 0);
    if (!key) return;
    if (courtyard)
    {
        yard[11] = (char)('0' + vk_game.room);
        VDP_drawText(yard, 16, 0);
        return;
    }
    hall[6] = (char)('0' + vk_game.stage);
    hall[14] = (char)('0' + vk_game.room);
    VDP_drawText(hall, 16, 0);
}

void vk_show_courtyard(u16 room)
{
    courtyard_room = room;
    debug_room_key = 0xFFFF;
    vk_front_mode = FRONT_COURTYARD;
    vk_show_scene(3 + room);
    vk_player = vk_game.player;
    load_actor(3 + room);
    VDP_loadTileData(flame_a_cleared_tiles, FLAME_A_CLEAR_TILE_INDEX, 4, CPU);
    VDP_drawText(room == 2 ? "B FOUET C SAUT DROITE PORTE" : "B FOUET  A+B FLAMME  C SAUT", 2, 27);
    show_play_overlay("", FALSE);
}

/* Playable castle room loader (stage 1 hall or stage 2): common VRAM/player
 * setup. Hall room 0 is the intro scene 6. */
void vk_show_castle_room(u16 stage, u16 room)
{
    debug_room_key = 0xFFFF;
    if (stage == 1 && room == 0)
    {
        vk_show_interior();
        return;
    }
    vk_show_room(stage, room, FALSE);
    vk_player = vk_game.player;
    VDP_loadTileData((const u32 *)vk_player_tiles, PLAYER_TILE_INDEX,
        VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_palettes[vk_stages[stage - 1].first + room], CPU);
    vk_load_whip_graphics();
    vk_reset_training_target();
    /* The room viewer text of rows 1..2 gives way to the HUD bars. */
    VDP_clearText(0, 1, 32);
    VDP_clearText(0, 2, 32);
    VDP_clearText(0, 27, 32);
    VDP_drawText(stage == 1 && room == 1 ? "A+HAUT ESCALIER B FOUET" :
        "B FOUET / A+L-R SALLE", 1, 27);
    show_play_overlay(stage == 1 ? "PARCOURS TEST / LIENS NON MSX2" :
        "ETAPE 2: CHAUVES-SOURIS+ZOMBIE", TRUE);
    vk_show_player(&vk_player);
    vk_front_mode = FRONT_INTERIOR;
}

void vk_show_interior(void)
{
    debug_room_key = 0xFFFF;
    vk_show_scene(6);
    vk_player = vk_game.player;
    /* Set the mode first: the hall target must not be placed as a courtyard flame. */
    vk_front_mode = FRONT_INTERIOR;
    load_actor(6);
    VDP_drawText("B FOUET A+B BOUGIES C SAUT", 1, 27);
    show_play_overlay("HALL: ORIGINAL MSX2 COLLISIONS", FALSE);
    vk_front_mode = FRONT_INTERIOR;
}
