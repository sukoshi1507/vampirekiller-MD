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
#include "msx_text.h"
#include "enemies.h"
#include "health.h"
#include "hud.h"
#include "items.h"
#include "item_view.h"
#include "candle_view.h"
#include "generated/intro_assets.h"
#include "room_freeze.h"
#include "wilhelm_audio.h"

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
/* State 9 after a room change (room_freeze.c). */
static VKRoomFreeze room_freeze;

/* Short play message on row 26 (rows 0..3 hold the HUD bars); the play note
 * comes back when diagnostic_timer runs out. */
static void show_message(const char *text)
{
    diagnostic_timer = 40;
    VDP_clearText(1, 26, 30);
    VDP_drawText(text, 1, 26);
}

static void show_courtyard(u16 room);

/* Stage music (research/audio-driver.md): table 7972 indexed by D000, played by
 * 794F in state 4 at the start of play and after a lost life. The courtyard is
 * D000 = 0, the hall and stage 2 are D000 = stage. */
static const u8 stage_music[19] = {
    0x80, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x82, 0x82, 0x82,
    0x85, 0x81, 0x81, 0x84, 0x84, 0x84, 0x83, 0x83, 0x85
};
static void play_stage_music(void)
{
    const u16 d000 = vk_game.area == VK_AREA_INTERIOR ? vk_game.stage : 0;
    if (d000 < sizeof(stage_music)) vk_psg_music(stage_music[d000]);
}

/* Death jingle 89 (70A0-70AC): the music stops and 89 starts when C428 runs out.
 * Fall: C428 = 30 frames in the water stages, 1 elsewhere (76BC). Fatal contact:
 * PROVISIONAL 79 frames (C0A7 = 7 at 41.94 s after the contact at 40.617 s,
 * research/player-health.md). */
#define DEATH_JINGLE_CONTACT_FRAMES 79
static u16 death_jingle_timer;
static void step_death_jingle(void)
{
    if (!death_jingle_timer || --death_jingle_timer) return;
    vk_psg_stop();
    vk_psg_music(VK_MUSIC_DEATH);
}

static void new_game(void)
{
    vk_game_begin(&vk_game);
    vk_reset_enemies();
    vk_health_begin(&health);
    vk_items_begin(&items);
    vk_candles_begin(&vk_candles);
    vk_set_whip_level(0);
    vk_room_freeze_clear(&room_freeze);
    show_courtyard(0);
    death_jingle_timer = 0;
    play_stage_music();
}

/* A courtyard screen, then its torches' objects and the heart count. */
static void show_courtyard(u16 room)
{
    vk_show_courtyard(room);
    vk_candle_view_room();
}

/* A castle room of the current stage, then the key, door and HUD key box of
 * items.c and the candles of candle_view.c over it. */
static void show_hall_room(u16 room)
{
    vk_show_castle_room(vk_game.stage, room);
    vk_item_view_room(&items, &vk_game);
    vk_candle_view_room();
}

/* One frame of candles.c: whip hits, drops and pickups, and what they change on
 * screen and in items.c (white key) and the whip (C416). */
static void step_candles(void)
{
    int hit;
    const VKByte gold_keys = vk_candles.gold_keys;
    const int events = vk_candles_tick(&vk_candles, &vk_game, &vk_whip, &hit);
    if (events & VK_CANDLE_EVENT_HIT)
    {
        vk_courtyard_flame_out(hit);
        vk_candle_view_out(hit);
        /* 7FD8: effect 0C on the contact; 87D8: effect 0E for a block (mode >= 2). */
        vk_psg_effect(VK_SFX_HIT);
        if (vk_candles_def(&vk_candles, hit)->mode >= 2) vk_psg_effect(VK_SFX_BLOCK);
    }
    if (events & VK_CANDLE_EVENT_WHITE_KEY)
    {
        items.white_key_revealed = 1;
        vk_item_view_room(&items, &vk_game);
    }
    if (events & VK_CANDLE_EVENT_HEARTS) vk_candle_view_hearts();
    if (events & VK_CANDLE_EVENT_WEAPON) vk_set_whip_level(vk_candles.weapon);
    /* Pickup effects, returned by the 8D3E handlers to 509F: hearts 0F (8DAF,
     * 8DB7), whip 12 (8D8A), gold key 14 (8E5D); a chest uses a key: 11 (8F7B). */
    if (events & VK_CANDLE_EVENT_HEARTS) vk_psg_effect(VK_SFX_HEART);
    if (events & VK_CANDLE_EVENT_WEAPON) vk_psg_effect(VK_SFX_ITEM);
    if (events & VK_CANDLE_EVENT_GOLD_KEY)
        vk_psg_effect(vk_candles.gold_keys < gold_keys ? VK_SFX_CHEST : VK_SFX_KEY);
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

/* Sprite 0 parked and alone in the list: no sprite on screen (63C5 writes Y E0 to
 * the whole SAT). The next vk_show_player rebuilds the full list. */
static void hide_sprites(void)
{
    VDP_setSpriteFull(0, -32, -32, SPRITE_SIZE(1, 1), 0, 0);
    VDP_updateSprites(1, DMA_QUEUE);
}

/* VBlank count of the last freeze step, and whether the new room is drawn. */
static u32 freeze_vtimer;
static bool freeze_drawn;

/* Room change by an edge or a vertical link (state 9, research/room-links.md
 * section 9): the game state is already in the new room; the screen keeps the old
 * one without sprites until the freeze shows it. */
static void begin_room_freeze(void)
{
    vk_room_freeze_begin(&room_freeze,
        vk_game.area == VK_AREA_INTERIOR ? vk_game.stage : 0, vk_game.room);
    hide_sprites();
    freeze_vtimer = vtimer;
    freeze_drawn = FALSE;
}

/* One frozen frame: no input, Simon, whip, enemies, candles, drops or flames;
 * only the music goes on (4028 with the C005 guard). The display goes off as the
 * castle entrance does (black palettes). The port draws the new room in about
 * ROOM_DRAW_FRAMES frames of 68000 time (display off during vk_show_room, then
 * tile loads with the room on screen), so it starts that many frames before the
 * end, and the room comes up, without sprites, near the end as on the MSX (47C7
 * in the last C000 = 9 frame). The freeze counts VBlanks (vtimer), not loop
 * turns, so that it keeps the MSX frame count across the draw. */
#define ROOM_DRAW_FRAMES 6
static void room_freeze_step(void)
{
    static const u16 black[64] = {0};
    const u32 now = vtimer;
    u32 frames = now - freeze_vtimer;
    freeze_vtimer = now;
    if (!frames) frames = 1;
    while (frames-- && vk_room_freeze_active(&room_freeze))
    {
        if (vk_room_freeze_tick(&room_freeze) == VK_FREEZE_DARK)
            PAL_setColors(0, black, 64, DMA_QUEUE);
    }
    if (!freeze_drawn && room_freeze.left <= ROOM_DRAW_FRAMES)
    {
        freeze_drawn = TRUE;
        if (vk_game.area == VK_AREA_INTERIOR) show_hall_room(vk_game.room);
        else show_courtyard(vk_game.room);
        hide_sprites();
    }
}

/* Pause (5C3A, state 11): "PAUSE" copied at (108, 88) over the room (4495), the
 * pause jingle FD; the key again restores the room (44B8) and plays FE. */
#define PAUSE_X 108
#define PAUSE_Y 88
static bool paused;

static void pause_start(void)
{
    paused = TRUE;
    vk_msx_text(BG_B, PAUSE_X, PAUSE_Y, "PAUSE", MSX_TEXT_TILE_INDEX, TRUE);
    vk_psg_pause();
}

static void pause_end(void)
{
    paused = FALSE;
    vk_msx_text_clear(BG_B, PAUSE_X, PAUSE_Y, 5);
    vk_psg_resume();
}

/* State 4 after a lost life: full energy, empty room, provisional restart place. */
static void respawn(void)
{
    vk_game_respawn(&vk_game);
    vk_reset_enemies();
    vk_items_new_life(&items);
    vk_candles_new_life(&vk_candles, vk_candles_stage_of(&vk_game));
    vk_set_whip_level(0);
    if (vk_game.area == VK_AREA_INTERIOR) show_hall_room(vk_game.room);
    else show_courtyard(vk_game.room);
    /* 4242: C40D = 1 after a lost life, so 794F starts the stage music again. */
    play_stage_music();
}

/* State 7 (42E3): the screen is cleared (47B9), "GAME  OVER" written at (88, 88)
 * (text 4D3A) and the HUD drawn again (4513). The HUD rows 0..3 are kept here, as
 * 4513 redraws them as they were. "F5 _ CONTINUE" (4300) needs E600, set by 5C92
 * only with a second Konami cartridge in another slot: not on the Mega Drive.
 * PROVISIONAL: the HUD gold key (a sprite here, 8EE6) is not shown. */
static void show_game_over(void)
{
    hide_sprites();
    VDP_clearTileMapRect(BG_A, 0, 4, 32, 24);
    VDP_clearTileMapRect(BG_B, 0, 0, 32, 28);
    vk_msx_text(BG_A, 88, 88, "GAME  OVER", MSX_TEXT_TILE_INDEX, FALSE);
}

/* One frame of health.c after the enemy contact; redraws the HUD bar. */
static void step_health(void)
{
    const bool fell = health.fell;
    const VKHealthEvent life = vk_health_tick(&health, vk_enemies.energy);
    if (life == VK_HEALTH_TITLE) { vk_show_title(); return; }
    if (life == VK_HEALTH_DIED && !fell) death_jingle_timer = DEATH_JINGLE_CONTACT_FRAMES;
    step_death_jingle();
    /* End of a fall (70B4): no more than 5 hearts. */
    if (fell && (life == VK_HEALTH_REFILL || life == VK_HEALTH_GAME_OVER) &&
        vk_candles.hearts > VK_HEALTH_HEARTS_AFTER_FALL)
    {
        vk_candles.hearts = VK_HEALTH_HEARTS_AFTER_FALL;
        vk_candle_view_hearts();
    }
    if (life == VK_HEALTH_REFILL) vk_enemies.energy = ENEMY_ENERGY_MAX;
    else if (life == VK_HEALTH_RESPAWN) respawn();
    /* PROVISIONAL: the MSX game over screen (state 7) is not captured. */
    else if (life == VK_HEALTH_GAME_OVER)
    {
        show_game_over();
        vk_psg_music(VK_MUSIC_GAME_OVER);   /* 42AB, state 6 without lives */
    }
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
    /* Loaded once here: a load at the first scream would cost a frame. */
    SND_PCM_loadDriver(TRUE);
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
            if (pressed & BUTTON_START)
            {
                vk_begin_castle();
                vk_psg_music(VK_MUSIC_START);   /* 4222: state 3, before the stage */
            }
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
            /* State 9: every key, START included, waits for the end of the freeze. */
            if (vk_room_freeze_active(&room_freeze)) room_freeze_step();
            /* State 11 (43DA): only the pause key is read. */
            else if (paused)
            {
                if (pressed & BUTTON_START) pause_end();
            }
            /* Debug: A+START goes back to the title. */
            else if ((pressed & BUTTON_START) && (input.keys & BUTTON_A))
            {
                if (vk_game_entrance_locked(&vk_game)) castle_door_front(FALSE);
                vk_show_title();
            }
            /* 5C3A: the pause key in state 5 (alive or dying, not states 6 and 7). */
            else if ((pressed & BUTTON_START) &&
                     (health.phase == VK_LIFE_ALIVE || health.phase == VK_LIFE_DYING))
                pause_start();
            /* State 7: only the countdown to the title runs. */
            else if (health.phase == VK_LIFE_GAME_OVER) step_health();
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
                /* Debug: A+B lights the candles of the room again. */
                if ((vk_front_mode == FRONT_INTERIOR || vk_front_mode == FRONT_COURTYARD) &&
                    (held & BUTTON_A) && (pressed & BUTTON_B))
                {
                    vk_candles_relight(&vk_candles);
                    /* Hall room 0 redraws its scene, which holds the baked candles. */
                    if (vk_front_mode == FRONT_COURTYARD) vk_reset_training_target();
                    else show_hall_room(vk_game.room);
                    show_message("BOUGIES RALLUMEES!");
                }
                const VKEvent event = vk_game_tick_controls(&vk_game, direction,
                    jump, !!(held & BUTTON_UP),
                    !!(held & BUTTON_DOWN));
                vk_player = vk_game.player;
                {
                    const bool idle = vk_whip.timer == 0;
                    /* No whip while knocked back: C420 = 5 ignores the controls. */
                    vk_whip_tick(&vk_whip, &vk_player, !!((pressed & BUTTON_B) && !(held & BUTTON_A)) &&
                                 !vk_game_recoiling(&vk_game));
                    if (idle && vk_whip.timer == 14) vk_psg_effect(VK_SFX_WHIP);   /* 7225 */
                }
                /* Make controller/attack state observable in BlastEm. No text
                 * is reprinted every frame: refresh on input or at timeout. */
                if (pressed & BUTTON_B)
                    show_message("B: ATTAQUE DETECTEE");
                vk_draw_attack_frame_label();
                if (diagnostic_timer && --diagnostic_timer == 0)
                    vk_draw_play_note();
                if (event == VK_EVENT_ENTRANCE)
                {
                    castle_door_front(TRUE);
                    vk_whip_reset(&vk_whip);
                    vk_psg_effect(VK_SFX_DOOR);   /* 7774: C5AC = 5 */
                }
                if (event == VK_EVENT_LAND) vk_psg_effect(VK_SFX_LANDING);   /* 6F7E */
                if (event == VK_EVENT_ENTER) show_hall_room(0);
                else if (event == VK_EVENT_ROOM || event == VK_EVENT_VERTICAL) begin_room_freeze();
                else if (event == VK_EVENT_FALL)
                {
                    vk_whip_reset(&vk_whip);
                    vk_health_fall(&health, vk_game.area == VK_AREA_INTERIOR ? vk_game.stage : 0);
                    show_message("CHUTE!");
                    /* Port addition, not in the ROM: the Wilhelm scream on the
                     * DAC (FM channel 6), the PSG keeps the MSX sounds. */
                    SND_PCM_startPlay(wilhelm_pcm, sizeof(wilhelm_pcm), SOUND_PCM_RATE_22050, SOUND_PAN_CENTER, FALSE);
                    /* 76BC: splash 09 in the water stages (D000 2 or 10). */
                    if (vk_game.area == VK_AREA_INTERIOR && (vk_game.stage == 2 || vk_game.stage == 10))
                    {
                        vk_psg_effect(VK_SFX_WATER);
                        death_jingle_timer = 30;
                    }
                    else death_jingle_timer = 1;
                }
                else if (vk_front_mode == FRONT_INTERIOR && direction &&
                         !(held & BUTTON_A) && !vk_game_recoiling(&vk_game) &&
                         vk_room_try_edge(vk_game.stage, &vk_game.room, &vk_game.player, direction))
                {
                    /* Keep player state across edge; collision map changes on
                     * next gameplay tick. No fabricated vertical connections. */
                    vk_player = vk_game.player;
                    begin_room_freeze();
                }
                else
                {
                    const VKItemEvent item = vk_items_tick(&items, &vk_game);
                    vk_player = vk_game.player;
                    if (item == VK_ITEM_STAGE_EXIT)
                    {
                        /* Stage 2 room 0. */
                        vk_game_enter_stage(&vk_game, 2);
                        vk_reset_enemies();
                        vk_candles_reset_stage(&vk_candles, 2);
                        show_hall_room(0);
                        SYS_doVBlankProcess();
                        continue;
                    }
                    vk_item_view_event(item, &items, &vk_game);
                    /* 8E68: white key 14. PROVISIONAL: the room 7 door gives 1A
                     * (7772: 15 only when C5AC reaches 5, at the castle door). */
                    if (item == VK_ITEM_KEY_TAKEN) vk_psg_effect(VK_SFX_KEY);
                    else if (item == VK_ITEM_DOOR_OPENED) vk_psg_effect(VK_SFX_DOOR_1A);
                    {
                        /* 6F9A: effect 13 when a contact starts the recoil (C42D set). */
                        const EnemyU8 invuln = vk_enemies.invuln;
                        vk_step_enemies();
                        if (!invuln && vk_enemies.invuln) vk_psg_effect(VK_SFX_DAMAGE);
                    }
                    step_candles();
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
