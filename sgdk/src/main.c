#include <genesis.h>
#include "input.h"
#include "generated/rooms.h"
#include "frontend.h"
#include "player.h"
#include "gameplay.h"
#include "room_manager.h"
#include "whip.h"
#include "actors.h"
#include "room_view.h"
#include "scene.h"
#include "sound_test.h"
#include "psg_player.h"
#include "generated/sfx_md.h"
#include "generated/sfx_hit_md.h"
#include "enemies.h"
#include "health.h"
#include "hud.h"
#include "items.h"
#include "item_view.h"
#include "generated/intro_assets.h"

/* Front-end state machine and game loop. Display lives in scene.c (logo, title,
 * castle), room_view.c (rooms, flames, target) and actors.c (Simon, whip, enemies). */
u16 vk_front_mode;
VKGame vk_game;
VKPlayer vk_player;
VKWhip vk_whip;
static u16 diagnostic_timer;
/* Lives, death and game over (health.c); the energy is vk_enemies.energy. */
static VKHealth health;
/* White key and room 7 door (items.c). */
static VKItems items;

/* Short play message on row 26 (rows 0..3 hold the HUD bars); the play note
 * comes back when diagnostic_timer runs out. */
static void show_message(const char *text)
{
    diagnostic_timer = 40;
    VDP_clearText(1, 26, 30);
    VDP_drawText(text, 1, 26);
}

static void new_game(void)
{
    vk_game_begin(&vk_game);
    vk_reset_enemies();
    vk_health_begin(&health);
    vk_items_begin(&items);
    vk_show_courtyard(0);
}

/* A castle room of the current stage, then the key, door and HUD key box of
 * items.c over it. */
static void show_hall_room(u16 room)
{
    vk_show_castle_room(vk_game.stage, room);
    vk_item_view_room(&items, &vk_game);
}

/* Castle door of courtyard room 2 (research/castle-entrance.md). The MSX copies the
 * door jamb, X 224..239 and Y 144..175, into sprites 0..7 in front of Simon
 * (routine 55EC); the copy is opaque. Here the jamb's eight background cells take
 * the plane priority and Simon's sprite gives his up while he walks in. Under
 * them, high-priority solid black cells on the otherwise unused plane B (system
 * tile 2, PAL1 colour 2 = black) fill the jamb's transparent pixels. */
#define DOOR_COL 28
#define DOOR_ROW 18
static void castle_door_front(bool front)
{
    u16 row, col;
    for (row = DOOR_ROW; row < DOOR_ROW + 4; row++)
        for (col = DOOR_COL; col < DOOR_COL + 2; col++)
        {
            if (front)
                VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE,
                    TILE_USER_INDEX + vk_scenes[5].map[row * 32 + col]), col, row);
            VDP_setTileMapXY(BG_B, front ?
                TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, TILE_SYSTEM_INDEX + 2) : 0, col, row);
        }
}

/* One frame of the castle entrance: Simon walks in alone behind the door jamb,
 * the screen goes black for 36 frames, then the hall comes up (gameplay.c). The
 * MSX turns the display off (R1 bit 6); here the four palettes go black instead,
 * because with the display off the VDP reports VBlank all the time and the game
 * loop would no longer wait for the frame. The hall reloads all four palettes. */
static void castle_entrance_step(void)
{
    static const u16 black[64] = {0};
    const VKEvent event = vk_game_tick_controls(&vk_game, 0, 0, 0, 0);
    vk_player = vk_game.player;
    if (event == VK_EVENT_ENTER)
    {
        castle_door_front(FALSE);
        show_hall_room(0);
    }
    else if (event == VK_EVENT_ENTRANCE_DARK) PAL_setColors(0, black, 64, DMA_QUEUE);
    else if (vk_game.entrance != VK_ENTRANCE_DARK)
    {
        vk_flames_tick();
        vk_show_player(&vk_player);
        VDP_setSpritePriority(0, FALSE);
    }
}

/* State 4 after a lost life: full energy, empty room, provisional restart place. */
static void respawn(void)
{
    vk_game_respawn(&vk_game);
    vk_reset_enemies();
    if (vk_game.area == VK_AREA_INTERIOR) show_hall_room(vk_game.room);
    else vk_show_courtyard(vk_game.room);
}

/* One frame of health.c after the enemy contact; redraws the HUD bar. */
static void step_health(void)
{
    const VKHealthEvent life = vk_health_tick(&health, vk_enemies.energy);
    if (life == VK_HEALTH_TITLE) { vk_show_title(); return; }
    if (life == VK_HEALTH_REFILL) vk_enemies.energy = ENEMY_ENERGY_MAX;
    else if (life == VK_HEALTH_RESPAWN) respawn();
    /* PROVISIONAL: the MSX game over screen (state 7) is not captured. */
    else if (life == VK_HEALTH_GAME_OVER) VDP_drawText("GAME OVER", 11, 13);
    vk_hud_update(vk_enemies.energy);
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
    vk_logo_start();
    while (TRUE)
    {
        u16 pressed;
        bool dirty = FALSE;
        handleInput(&input);
        pressed = input.pressed;
        if (vk_front_mode == FRONT_LOGO)
            vk_logo_tick(pressed);
        else if (vk_front_mode == FRONT_TITLE)
        {
            if (pressed & BUTTON_START) vk_begin_castle();
            else if (vk_konami_step(pressed)) { vk_sound_test_enter(); vk_front_mode = FRONT_SOUND; }
            else if (pressed & BUTTON_A)
            {
                stage = 1; room = 0; atlas = FALSE;
                vk_show_room(stage, room, atlas); vk_front_mode = FRONT_VIEWER;
            }
        }
        else if (vk_front_mode == FRONT_CASTLE)
        {
            if (vk_castle_tick(pressed)) new_game();
        }
        else if (vk_front_mode == FRONT_COURTYARD || vk_front_mode == FRONT_INTERIOR)
        {
            /* From a fatal contact to the respawn, and while the door opens, only
             * START (title) is read. */
            const bool locked = vk_health_controls_locked(&health) ||
                                vk_items_controls_locked(&items);
            const int direction = locked ? 0 : input.direction;
            const u16 held = locked ? 0 : input.keys;
            const bool jump = locked ? FALSE : input.jump_pressed;
            if (pressed & BUTTON_START)
            {
                if (vk_game_entrance_locked(&vk_game)) castle_door_front(FALSE);
                vk_show_title();
            }
            else if (vk_game_entrance_locked(&vk_game)) castle_entrance_step();
            else
            {
                if (locked) pressed = 0;
                /* Konami code in play toggles the internal room number on screen. */
                if (vk_konami_step(pressed)) vk_debug_label_toggle();
                vk_debug_label_draw();
                if (vk_front_mode == FRONT_INTERIOR && (held & BUTTON_A) &&
                    (pressed & (BUTTON_RIGHT | BUTTON_LEFT)))
                {
                    int target = (int)vk_game.room + ((pressed & BUTTON_RIGHT) ? 1 : -1);
                    if (target >= 0 && vk_game_debug_room(&vk_game, (VKByte)target))
                    {
                        show_hall_room((u16)target);
                    }
                    SYS_doVBlankProcess();
                    continue;
                }
                if ((vk_front_mode == FRONT_INTERIOR || vk_front_mode == FRONT_COURTYARD) &&
                    (held & BUTTON_A) && (pressed & BUTTON_B))
                {
                    vk_reset_training_target();
                    show_message(vk_front_mode == FRONT_COURTYARD ?
                        "FLAMME RALLUMEE!" : "CIBLE REINITIALISEE");
                }
                const VKEvent event = vk_game_tick_controls(&vk_game, direction,
                    jump, !!(held & BUTTON_UP),
                    !!(held & BUTTON_DOWN));
                vk_player = vk_game.player;
                {
                    const bool idle = vk_whip.timer == 0;
                    vk_whip_tick(&vk_whip, &vk_player, !!((pressed & BUTTON_B) && !(held & BUTTON_A)));
                    if (idle && vk_whip.timer == 14) vk_psg_play(vk_sfx_whip);
                }
                /* Make controller/attack state observable in BlastEm. No text
                 * is reprinted every frame: refresh on input or at timeout. */
                if (pressed & BUTTON_B)
                    show_message("B: ATTAQUE DETECTEE");
                if (vk_front_mode == FRONT_COURTYARD)
                {
                    if (vk_courtyard_flames_hit(&vk_whip) >= 0)
                    {
                        show_message("FLAMME ETEINTE!");
                        /* Torch hit: the castlevania-md hit effect stands in for MSX 0x0C. */
                        vk_psg_play(vk_sfx_hit);
                    }
                }
                else if (vk_training_target_hit(&vk_whip))
                    show_message("CIBLE TOUCHEE!");
                vk_draw_attack_frame_label();
                if (diagnostic_timer && --diagnostic_timer == 0)
                    vk_draw_play_note();
                if (event == VK_EVENT_ENTRANCE)
                {
                    castle_door_front(TRUE);
                    vk_whip_reset(&vk_whip);
                }
                if (event == VK_EVENT_ENTER) show_hall_room(0);
                else if (event == VK_EVENT_ROOM) vk_show_courtyard(vk_game.room);
                else if (event == VK_EVENT_VERTICAL) show_hall_room(vk_game.room);
                else if (vk_front_mode == FRONT_INTERIOR && direction &&
                         !(held & BUTTON_A) &&
                         vk_room_try_edge(vk_game.stage, &vk_game.room, &vk_game.player, direction))
                {
                    /* Keep player state across edge; collision map changes on
                     * next gameplay tick. No fabricated vertical connections. */
                    vk_player = vk_game.player;
                    show_hall_room(vk_game.room);
                }
                else
                {
                    const VKItemEvent item = vk_items_tick(&items, &vk_game);
                    vk_player = vk_game.player;
                    if (item == VK_ITEM_STAGE_EXIT)
                    {
                        /* Stage 2 room 0 (PROVISIONAL: no enemies nor items yet). */
                        vk_game_enter_stage(&vk_game, 2);
                        vk_reset_enemies();
                        show_hall_room(0);
                        SYS_doVBlankProcess();
                        continue;
                    }
                    vk_item_view_event(item, &items, &vk_game);
                    vk_step_enemies();
                    vk_flames_tick();
                    vk_show_player(&vk_player);
                    step_health();
                }
            }
        }
        else if (vk_front_mode == FRONT_SOUND)
        {
            if (vk_sound_test_tick(pressed)) vk_show_title();
        }
        else if (vk_front_mode == FRONT_TEST)
        {
            if (pressed & BUTTON_B)
            { vk_show_room(1, 0, FALSE); vk_front_mode = FRONT_VIEWER; }
            else if (pressed & BUTTON_START) vk_enter_play(&vk_player);
            else
            {
                const int direction = input.direction;
                vk_player_tick(&vk_player, direction, input.jump_pressed);
                vk_whip_tick(&vk_whip, &vk_player, !!(pressed & BUTTON_A));
                if (vk_training_target_hit(&vk_whip))
                    VDP_drawText("CIBLE TOUCHEE!", 8, 3);
                vk_show_player(&vk_player);
            }
        }
        else /* The asset explorer remains available from the title with A. */
        {
            if (pressed & BUTTON_B)
            { vk_enter_play(&vk_player); vk_front_mode = FRONT_TEST; }
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
                if (dirty) vk_show_room(stage, room, atlas);
            }
        }
        vk_psg_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
