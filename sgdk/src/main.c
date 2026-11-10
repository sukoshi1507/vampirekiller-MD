#include <genesis.h>
#include "input.h"
#include "generated/rooms.h"
#include "frontend.h"
#include "player.h"
#include "gameplay.h"
#include "room_manager.h"
#include "whip.h"
#include "actors.h"
#include "dracula_view.h"
#include "room_view.h"
#include "scene.h"
#include "sound_test.h"
#include "wall_view.h"
#include "stage_map_view.h"
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
#include "secret.h"
#include "item_reveal_view.h"
#include "wilhelm_audio.h"

/* Front-end state machine and game loop. Display lives in scene.c (logo, title,
 * castle), room_view.c (rooms, flames, target) and actors.c (Simon, whip, enemies). */
u16 vk_front_mode;
VKGame vk_game;
VKPlayer vk_player;
VKWhip vk_whip;
/* Lives, death and game over (health.c); the energy is vk_enemies.energy. */
static VKHealth health;
/* White key and room 7 door (items.c). */
static VKItems items;
/* State 9 after a room change (room_freeze.c). */
static VKRoomFreeze room_freeze;
static bool secret_up_pending;
static bool map_a_pending;

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

/* 794F, door path: the music restarts on arrival only after a boss stage
 * (7777[D000 - 1] != 0); from the courtyard, the hall and stage 2 it goes on. */
static const u8 stage_after_boss[19] = {
    0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1
};

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

static void enemy_loot_take(EnemyState *s,EnemyU8 kind);

static void new_game(void)
{
    vk_hud_score = 0;   /* 44C6 clears C405-C407 */
    vk_game_begin(&vk_game);
    vk_reset_enemies();
    vk_reset_enemy_clock();
    vk_health_begin(&health);
    vk_items_begin(&items);
    vk_candles_begin(&vk_candles);
    Enemies_loot_handler(enemy_loot_take);
    vk_wall_begin(&vk_walls);
    vk_set_whip_level(0);
    vk_weapons_reset(&vk_weapons);
    vk_room_freeze_clear(&room_freeze);
    show_courtyard(0);
    death_jingle_timer = 0;
    play_stage_music();
}

/* 4326: first clear starts the courtyard cycle; second clear returns to title. */
static void dracula_finish(void)
{
 const VKDracula *d=vk_dracula_state();const u8 difficulty=d->difficulty;
 const VKByte hearts=vk_candles.hearts,map=vk_candles.c701&0x80,uses=vk_candles.c70f,lives=health.lives;
 const EnemyU8 tick=vk_enemies.tick,phase=vk_enemies.itemPhase;
 vk_dracula_view_reset();
 if(d->return_title){vk_show_title();return;}
 vk_game_begin(&vk_game);vk_items_begin(&items);vk_reset_enemies();
 vk_enemies.tick=tick;vk_enemies.itemPhase=phase;
 vk_health_begin(&health);health.lives=lives;
 vk_candles_begin(&vk_candles);vk_candles.hearts=hearts;vk_candles.c701=map;vk_candles.c70f=uses;
 vk_wall_begin(&vk_walls);vk_walls.d012=difficulty;vk_set_whip_level(0);
 vk_weapons_reset(&vk_weapons);vk_room_freeze_clear(&room_freeze);
 show_courtyard(0);vk_hud_status(vk_hud_score,0,vk_candles.hearts,health.lives);death_jingle_timer=0;play_stage_music();
}

/* 5BB7 at a room load: the wall objects of the room, drawn again. */
static void wall_room(void)
{
    vk_secret_enter(&vk_secret, vk_candles_stage_of(&vk_game), vk_game.room);
    secret_up_pending = FALSE;
    vk_wall_enter(&vk_walls, vk_candles_stage_of(&vk_game), vk_game.room);
    vk_wall_view_room();
}

/* A courtyard screen, then its torches' objects and the heart count. */
static void show_courtyard(u16 room)
{
    vk_show_courtyard(room);
    vk_candle_view_room();
    wall_room();
}

/* A castle room of the current stage, then the key, door and HUD key box of
 * items.c and the candles of candle_view.c over it. */
static void show_hall_room(u16 room)
{
    vk_show_castle_room(vk_game.stage, room);
    vk_items_enter_room(&items);
    vk_item_view_room(&items, &vk_game);
    vk_candle_view_room();
    wall_room();
    if(vk_game.stage==18 && room==9)vk_step_enemies();
}

/* Frames left before next_stage() after a boss exit (B772: C409 = 1). State 8 (4324)
 * runs 47B9 first: the sprites go, display off and the page cleared by a VDP command
 * that holds the Z80 for the rest of the 28 frames (trace stage 6, C409 at 2897, state 4
 * at 2925, C000 = 8 from 2898 to 2924; research/stage6-boss.md). The screen is black
 * and nothing else runs, the music goes on. */
#define BOSS_EXIT_FRAMES 28
static u16 stage_blank;

static void next_stage(void)
{
    /* Stage 2 room 0. 63B7 (state 4) clears C420-C46F, C43B
     * included, but neither D010 nor C701: enemies frozen by
     * object 0A stay frozen until a death (70D3), and 7777
     * plays no music for stage 2, so the FB loop goes on.
     * C701 (vk_candles.c701) is not touched here, and the room
     * load below draws its box again (4513 -> 8EE6). C441 goes to 0
     * with the block while C701 bit 4 stays: the armor holds and its
     * next frontal hit wraps C441 to FF (819B). C415, below the block,
     * keeps Simon's energy (trace, research/castle-candles.md 9.8). */
    const EnemyU8 frozen = vk_enemies.frozen;
    const EnemyU8 energy = vk_enemies.energy;
    const VKByte next = (VKByte)(vk_game.stage + 1);
    vk_game_enter_stage(&vk_game, next);
    vk_items_enter_stage(&items);
    vk_reset_enemies();
    vk_enemies.frozen = frozen;
    vk_enemies.energy = energy;
    vk_weapons_reset(&vk_weapons);   /* 63B7, state 4 */
    vk_candles_reset_stage(&vk_candles, next);
    vk_wall_reset_stage(&vk_walls, next);
    vk_candles_hud(&vk_candles);   /* state 4 (4237): HUD drawn again */
    show_hall_room(0);
    if (stage_after_boss[next]) play_stage_music();
}

/* A candle put out by the whip or a thrown weapon: the view and items.c follow. */
static void candle_hit_view(int events, int hit)
{
    if (events & VK_CANDLE_EVENT_HIT)
    {
        vk_courtyard_flame_out(hit);
        vk_candle_view_out(hit);
        /* 7FD8: effect 0C on the contact; 87D8: effect 0E for a block (mode >= 2). */
        vk_psg_effect(VK_SFX_HIT);
        if (vk_candles_def(&vk_candles, hit)->mode >= 2) vk_psg_effect(VK_SFX_BLOCK);
    }
    if (events & VK_CANDLE_EVENT_WALL)   /* 8814 -> 9179 */
        vk_wall_block(&vk_walls, hit, vk_candles_def(&vk_candles, hit));
    if (events & VK_CANDLE_EVENT_WHITE_KEY)
    {
        items.white_key_revealed = 1;
        vk_item_view_room(&items, &vk_game);
    }
}

/* An object given to Simon: what it changes in the whip, enemies and sounds. */
static void candle_taken(int events, VKByte gold_keys, bool audio)
{
    const VKByte kind=vk_candles.picked;
    /* 7FFD: any object taken clears C433 (the block an axe touched). */
    if (events & (VK_CANDLE_EVENT_HEARTS | VK_CANDLE_EVENT_WEAPON | VK_CANDLE_EVENT_GOLD_KEY |
                  VK_CANDLE_EVENT_ITEM | VK_CANDLE_EVENT_PICK | VK_CANDLE_EVENT_BOX_ITEM))
        vk_weapons_pick(&vk_weapons);
    if (events & VK_CANDLE_EVENT_WEAPON) vk_set_whip_level(vk_candles.weapon);
    /* Energy, armor, timers C434/C43A/C440, enemy wipe (8D3E, enemies.c). */
    if (events & VK_CANDLE_EVENT_PICK)
    {
        Enemies_item(&vk_enemies, kind, vk_candles.c431);
        if (kind == 0x05) vk_wall_cross(&vk_walls);   /* 928D */
    }
    /* Pickup effects, returned by the 8D3E handlers to 509F: hearts 0F (8DAF,
     * 8DB7), most objects 12 (8D8A), 05: 1B, 08: 16, gold key 14 (8E5D); a chest
     * uses a key: 11 (8F7B). */
    if (audio && (events & VK_CANDLE_EVENT_HEARTS)) vk_psg_effect(VK_SFX_HEART);
    /* 8E42, 8E4B: DE = 5000h or 1000h, then 44EC adds it to the score (C405-C407). */
    if ((events & VK_CANDLE_EVENT_ITEM) && (kind == 0x13 || kind == 0x14))
        vk_hud_add_score(kind == 0x13 ? 5000 : 1000);
    if (audio && (events & (VK_CANDLE_EVENT_WEAPON | VK_CANDLE_EVENT_ITEM | VK_CANDLE_EVENT_PICK |
                   VK_CANDLE_EVENT_BOX_ITEM)) &&
        vk_candles_sound(kind))
        vk_psg_effect(vk_candles_sound(kind));
    if (audio && (events & VK_CANDLE_EVENT_GOLD_KEY))
        vk_psg_effect(vk_candles.gold_keys < gold_keys ? VK_SFX_CHEST : VK_SFX_KEY);
}

static void enemy_loot_take(EnemyState *s,EnemyU8 kind)
{
    const VKByte keys=vk_candles.gold_keys;
    (void)s;
    candle_taken(vk_candles_give(&vk_candles,kind),keys,TRUE);
}

/* One frame of candles.c: whip hits, drops and pickups, and what they change on
 * screen and in items.c (white key) and the whip (C416). */
static void step_candles(void)
{
    int hit;
    const VKByte gold_keys = vk_candles.gold_keys;
    int events, i;
    /* 9B90 / 9B13: what the enemies killed this frame leave (enemies_death.inc), made before the tick like a candle's drop. */
    if(Enemies_lootPick){
        int picked=vk_candles_give(&vk_candles,Enemies_lootPick);Enemies_lootPick=0;
        candle_taken(picked,gold_keys,TRUE);
    }
    for (i = 0; i < Enemies_dropCount; i++){
        if(Enemies_drops[i].landed)vk_candles_loot_rest(&vk_candles,Enemies_drops[i].kind,(VKByte)Enemies_drops[i].x,Enemies_drops[i].y,vk_enemies.tick);
        else vk_candles_death_drop(&vk_candles, Enemies_drops[i].kind, Enemies_drops[i].x, Enemies_drops[i].y);
    }
    Enemies_dropCount = 0;
    vk_candles_sync(vk_enemies.tick,(VKByte)!vk_enemies.itemPhase);
    events = vk_candles_tick(&vk_candles, &vk_game, &vk_whip, &hit);
    if (events & VK_CANDLE_EVENT_GHOST)   /* 8AD0: the flame of an object 15 went out, the enemy object of the stage is made */
        Enemies_candleObject(&vk_enemies, (EnemyU8)vk_game.stage, (EnemyU8)vk_game.room, vk_candles.ghost_x, vk_candles.ghost_y);
    candle_hit_view(events, hit);
    candle_taken(events, gold_keys, TRUE);
}

/* One frame of the wall objects: whip hits (80E5), then 91BE. */
static void step_walls(void)
{
    VKWallEvents ev;
    u16 i;
    for (i = 0; i < VK_WALL_RECORDS; i++)
        if (vk_wall_whip_touch(&vk_walls, (int)i, &vk_whip, vk_candles.weapon))
        {
            vk_wall_hit(&vk_walls, (int)i);
            vk_psg_effect(VK_SFX_HIT);           /* 8112 */
        }
    vk_walls.c003 = vk_enemies.tick; /* global C003, also used by 94BA */
    vk_wall_step(&vk_walls, &vk_candles, &ev);
    for (i = 0; i < ev.sounds; i++) vk_psg_effect(ev.sound[i]);
    if (ev.points) vk_hud_add_score(VK_WALL_POINTS);   /* 9372 */
}

/* Late rooms are connected by LOT11; full traversal scenarios remain LOT12;
 * the hook is validated through isolated RAM room entry.
 * 6DA9/70FB before75C0. Keep the player still, but the world and common
 * invulnerability timer continue. UP edges between30Hz passes are latched until
 * the next pass, preserving the crouched state that the ROM input read saw. */
static bool secret_controls(bool up_new, VKEvent *event)
{
    VKPlayer *p = &vk_game.player;
    const bool pass = vk_enemies.itemPhase == 1;
    *event = VK_EVENT_NONE;
    if (vk_secret.active)
    {
        if (pass && vk_secret_poll(&vk_secret, vk_enemies.invuln))
        {
            vk_secret_route(&vk_secret, &vk_game.room);
            *event = VK_EVENT_ROOM; /* B98C changes only D001, no X/Y teleport. */
        }
        return TRUE;
    }
    if (vk_game.area != VK_AREA_INTERIOR || vk_game.recoil || p->mode ||
        p->on_stairs || !p->crouched || !vk_secret_near(&vk_secret,p->y,p->x))
    { secret_up_pending = FALSE; return FALSE; }
    if (!secret_up_pending && !up_new) return FALSE;
    if (!pass) { secret_up_pending = TRUE; return TRUE; }
    secret_up_pending = FALSE;
    {
        const VKByte effect = vk_secret_begin(&vk_secret,2,p->y,p->x,1,&vk_enemies.invuln);
        if (!effect) return FALSE;
        vk_enemies.invulnPhase = 0; /* timer75C0 starts in this30Hz pass */
        vk_psg_effect(effect);
    }
    return TRUE;
}

static void hud_status(void);

/* State 0C: the merchant, one pass a frame; the game is frozen around it. */
static void step_shop(u16 keys)
{
    VKWallEvents ev;
    const VKByte state = vk_walls.shop;
    const VKByte gold_keys = vk_candles.gold_keys;
    const int events = vk_shop_step(&vk_walls, &vk_candles,
                                    (VKByte)(((keys & BUTTON_B) ? 1 : 0) | ((keys & BUTTON_C) ? 2 : 0)), &ev);
    u16 i;
    vk_enemies.tick = vk_walls.c003; /* C003 keeps advancing while the world is frozen. */
    if (state == VK_SHOP_OPEN) vk_shop_view_open();
    if (vk_walls.shop == VK_SHOP_NONE) vk_shop_view_close();
    candle_taken(events, gold_keys, FALSE); /* 8D30 returns A;94EF replaces it by12. */
    for (i = 0; i < ev.sounds; i++) vk_psg_effect(ev.sound[i]);
    hud_status();
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
 * the whole SAT). The next vk_show_player rebuilds the full list, without the
 * delayed Simon and enemies of the old room. */
static void hide_sprites(void)
{
    vk_actors_flush();
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
    vk_weapons_reset(&vk_weapons);   /* 639E: 7513 at the room load */
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
#define ROOM_DRAW_FRAMES 7
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
    vk_item_reveal_invalidate();
}

/* Top line of the HUD. Stage C411 = D000: 0 in the courtyard (44C6), +1 at each
 * stage exit (434F). */
static void hud_status(void)
{
    vk_hud_status(vk_hud_score, vk_game.area == VK_AREA_INTERIOR ? vk_game.stage : 0,
                  vk_candles.hearts, health.lives);
}

/* State 4 after a lost life: full energy, empty room, provisional restart place. */
static void respawn(void)
{
    vk_game_respawn(&vk_game);
    vk_reset_enemies();
    vk_items_new_life(&items);
    vk_candles_new_life(&vk_candles, vk_candles_stage_of(&vk_game));
    vk_wall_reset_stage(&vk_walls, vk_candles_stage_of(&vk_game));
    vk_candles_hud(&vk_candles);   /* 4237: 4513 draws the HUD again, gold key gone */
    vk_set_whip_level(0);
    vk_weapons_reset(&vk_weapons);   /* 63B7 clears C420-C46F */
    hud_status();   /* 4237: C410 already decremented when 4513 draws the HUD */
    if (vk_game.area == VK_AREA_INTERIOR) show_hall_room(vk_game.room);
    else show_courtyard(vk_game.room);
    /* 4242: C40D = 1 after a lost life, so 794F starts the stage music again. */
    play_stage_music();
}

/* State 7 (42E3): the screen is cleared (47B9), "GAME  OVER" written at (88, 88)
 * (text 4D3A) and the HUD drawn again (4513). The HUD rows 0..3 are kept here, as
 * 4513 redraws them as they were. "F5 _ CONTINUE" (4300) needs E600, set by 5C92
 * only with a second Konami cartridge in another slot: not on the Mega Drive.
 * The HUD gold key (8EB4 -> 8EC9 in 4513) is not drawn: the death (70F7) has
 * already set C700 to 0; 8EE6 in 4513 draws the item box of C701 &= 80h
 * (only object 0F can be left). */
static void show_game_over(void)
{
    vk_candles_hud(&vk_candles);
    vk_candles_item_box(&vk_candles);
    hide_sprites();
    VDP_clearTileMapRect(BG_A, 0, 4, 32, 24);
    VDP_clearTileMapRect(BG_B, 0, 0, 32, 28);
    vk_msx_text(BG_A, 88, 88, "GAME  OVER", MSX_TEXT_TILE_INDEX, FALSE);
}

/* Optional stage selection, reachable ONLY by the Konami code while paused. */
static bool stage_select;
static u16 selected_stage, selected_room;
static void stage_select_draw(void)
{
    /* 13 + 10 + 9 cells: within MSX_TEXT_TILE_MAX, the PAUSE text shares these tiles. */
    char line[] = "NIV 00 SAL 00";
    line[4] = '0' + selected_stage / 10;
    line[5] = '0' + selected_stage % 10;
    line[11] = '0' + selected_room / 10;
    line[12] = '0' + selected_room % 10;
    vk_msx_text_clear(BG_B, PAUSE_X, PAUSE_Y, 5);
    vk_msx_text_clear(BG_B, 40, 104, 24);
    vk_msx_text_clear(BG_B, 40, 120, 24);
    vk_msx_text(BG_B, 40, 104, line, MSX_TEXT_TILE_INDEX, TRUE);
    vk_msx_text(BG_B, 40, 120, "UD LR B OK", MSX_TEXT_TILE_INDEX + 13, TRUE);
    vk_msx_text_clear(BG_B, 40, 136, 24);
    vk_msx_text(BG_B, 40, 136, Enemies_god ? "A GOD ON" : "A GOD OFF", MSX_TEXT_TILE_INDEX + 23, TRUE);
}
static void stage_select_clear(void)
{
    vk_msx_text_clear(BG_B, 40, 104, 24);
    vk_msx_text_clear(BG_B, 40, 120, 24);
    vk_msx_text_clear(BG_B, 40, 136, 24);
    stage_select = FALSE;
}
static void jump_to_stage(u16 stage, u16 room)
{
    if (stage < 1 || stage > 18 || room >= vk_stage_room_count((VKByte)stage)) return;
    /* Full fresh game, then the selected playable room. */
    stage_select_clear();
    pause_end();
    new_game();
    vk_game_debug_start(&vk_game, (VKByte)stage, (VKByte)room);
    vk_items_enter_stage(&items);
    vk_reset_enemies();
    vk_reset_enemy_clock();
    vk_weapons_reset(&vk_weapons);
    vk_candles_reset_stage(&vk_candles, (VKByte)stage);
    vk_wall_reset_stage(&vk_walls, (VKByte)stage);
    vk_player = vk_game.player;
    show_hall_room(vk_game.room);
    play_stage_music();
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
    }
    /* 70BF: the death clears the gold keys (C700) before state 6. */
    if (life == VK_HEALTH_REFILL || life == VK_HEALTH_GAME_OVER)
    {
        vk_candles_death(&vk_candles);
        Enemies_death(&vk_enemies);   /* 70C7-70EE: C434, C43A, C440, C441 */
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
    hud_status();
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
    vk_psg_init();   /* the PSG player runs in V-Int from now on */
    vk_logo_start();
    while (TRUE)
    {
        u16 pressed;
        bool dirty = FALSE;
        int throw_launch = 0;
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
            else if (pressed & BUTTON_C)
            {
                /* Debug: C starts a game in stage 2 room 0, as through the room 7 door. */
                new_game();
                vk_game_enter_stage(&vk_game, 2);
                vk_items_enter_stage(&items);
                vk_player = vk_game.player;
                vk_reset_enemies();
                vk_candles_reset_stage(&vk_candles, 2);
                vk_wall_reset_stage(&vk_walls, 2);
                show_hall_room(0);
                play_stage_music();
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
            /* State 8 after a boss: no key is read, the screen is black. */
            if(vk_dracula_cinematic()){
                vk_step_enemies();
                if(vk_boss_take_exit())dracula_finish();
            }
            else if (stage_blank)
            {
                static const u16 black[64] = {0};
                if (--stage_blank == 0) next_stage();
                else if (stage_blank == BOSS_EXIT_FRAMES - 1) PAL_setColors(0, black, 64, DMA_QUEUE);
            }
            /* State 9: every key, START included, waits for the end of the freeze. */
            else if (vk_room_freeze_active(&room_freeze)) room_freeze_step();
            /* State 11 (43DA): only the pause key is read. */
            else if (paused)
            {
                if (stage_select)
                {
                    if (pressed & BUTTON_UP) { selected_stage = selected_stage == 1 ? 18 : selected_stage - 1; selected_room = 0; stage_select_draw(); }
                    else if (pressed & BUTTON_DOWN) { selected_stage = selected_stage == 18 ? 1 : selected_stage + 1; selected_room = 0; stage_select_draw(); }
                    else if (pressed & BUTTON_LEFT) { selected_room = selected_room ? selected_room - 1 : vk_stage_room_count((VKByte)selected_stage) - 1; stage_select_draw(); }
                    else if (pressed & BUTTON_RIGHT) { selected_room = (selected_room + 1) % vk_stage_room_count((VKByte)selected_stage); stage_select_draw(); }
                    else if (pressed & BUTTON_B) { jump_to_stage(selected_stage, selected_room); }
                    else if (pressed & BUTTON_A) { Enemies_god ^= 1; stage_select_draw(); }
                    else if (pressed & (BUTTON_C | BUTTON_START)) { stage_select_clear(); pause_end(); }
                }
                else if (vk_konami_step(pressed))
                {
                    selected_stage = vk_game.stage >= 1 && vk_game.stage <= 18 ? vk_game.stage : 1;
                    selected_room = vk_game.area == VK_AREA_INTERIOR && vk_game.room < vk_stage_room_count((VKByte)selected_stage) ? vk_game.room : 0;
                    stage_select = TRUE;
                    Enemies_god = 1;
                    stage_select_draw();
                }
                else if (pressed & BUTTON_START) pause_end();
            }
            /* 95B3: the map stays until the key is pressed again; the game is frozen. */
            else if (vk_stage_map_is_open())
            {
                map_a_pending |= !!(pressed & BUTTON_A);
                vk_enemies.itemPhase ^= 1; /* C003 advances; Simon/item timers stay frozen. */
                if(!vk_enemies.itemPhase){
                    vk_enemies.tick++;
                    vk_stage_map_tick(map_a_pending);
                    map_a_pending=FALSE;
                }
                if (vk_stage_map_is_open()) vk_candle_view_hud_only();
            }
            /* Debug: A+START goes back to the title. */
            else if ((pressed & BUTTON_START) && (input.keys & BUTTON_A))
            {
                if (vk_game_entrance_locked(&vk_game)) castle_door_front(FALSE);
                vk_show_title();
            }
            /* State 0C (43F0): no pause, only the answer to the merchant is read. */
            else if (vk_walls.shop != VK_SHOP_NONE) step_shop(input.keys);
            /* 9552, in state 5: F2 is A pressed alone. */
            else if ((pressed & BUTTON_A) && !(input.keys & ~BUTTON_A) &&
                     (health.phase == VK_LIFE_ALIVE || health.phase == VK_LIFE_DYING) &&
                     !vk_game_entrance_locked(&vk_game) && !vk_boss.in_room &&
                     (vk_candles.c701 & VK_ITEM_BIT_0F) && vk_candles.c70f)
            {
                vk_psg_effect(0x19);
                map_a_pending=FALSE;
                vk_enemies.itemPhase^=1;if(!vk_enemies.itemPhase)vk_enemies.tick++;
                vk_stage_map_open();
                vk_candle_view_hud_only();
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
                /* In play, the Konami code only counts while paused. */
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
                }
                vk_game.items = vk_candles.c431;
                vk_step_platforms();
                VKEvent event;
                if (!secret_controls(!!(pressed & BUTTON_UP), &event))
                    event = vk_game_tick_controls(&vk_game, direction,
                        jump, !!(held & BUTTON_UP), !!(held & BUTTON_DOWN));
                vk_player = vk_game.player;
                {
                    const bool idle = vk_whip.timer == 0;
                    /* No whip while knocked back: C420 = 5 ignores the controls. */
                    bool strike = (pressed & BUTTON_B) && !(held & BUTTON_A) && !vk_game_recoiling(&vk_game);
                    /* 7129-7132: with C416 >= 2 the stroke needs a free slot (71BE). */
                    if (strike && idle && vk_whip_level(&vk_whip) >= VK_WHIP_LEVEL_THROW)
                        strike = vk_weapons_reserve(&vk_weapons, (VKByte)vk_whip_level(&vk_whip));
                    throw_launch = vk_whip_tick(&vk_whip, &vk_player, strike);
                    /* 7225: effect 05 for the whips only (7213, C436 < 2). */
                    if (idle && vk_whip.timer == 14 && vk_whip_level(&vk_whip) < VK_WHIP_LEVEL_THROW)
                        vk_psg_effect(VK_SFX_WHIP);
                }
                if (event == VK_EVENT_ENTRANCE)
                {
                    castle_door_front(TRUE);
                    vk_whip_stop(&vk_whip);
                    vk_weapons_reset(&vk_weapons);   /* 774F: 7513 at the door */
                    vk_psg_effect(VK_SFX_DOOR);   /* 7774: C5AC = 5 */
                }
                if (event == VK_EVENT_LAND) vk_psg_effect(VK_SFX_LANDING);   /* 6F7E */
                if (event == VK_EVENT_ENTER) show_hall_room(0);
                else if (event == VK_EVENT_ROOM || event == VK_EVENT_VERTICAL) begin_room_freeze();
                else if (event == VK_EVENT_FALL)
                {
                    vk_whip_stop(&vk_whip);
                    vk_health_fall(&health, vk_game.area == VK_AREA_INTERIOR ? vk_game.stage : 0);
                    if (Enemies_god) health.lives++;   /* the respawn takes it back */
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
                    items.attacking = vk_whip.timer != 0;   /* C422, 7FE2 */
                    const VKItemEvent item = vk_items_tick(&items, &vk_game);
                    vk_player = vk_game.player;
                    if (item == VK_ITEM_STAGE_EXIT && vk_boss_door(vk_game.stage, vk_game.room))
                    {
                        /* 77D1, trace: the link of the side of the door (stage 3: right, X 10, Y 176;
                         * stage 6: left, X 246, the Y of the door room goes on), music 87. */
                        vk_game.room = (VKByte)vk_boss_room_of(vk_game.stage);
                        vk_game.player.x = (VKByte)vk_boss_entry_x(vk_game.stage);
                        if (vk_boss_entry_y(vk_game.stage) >= 0) vk_game.player.y = (VKByte)vk_boss_entry_y(vk_game.stage);
                        vk_player = vk_game.player;
                        show_hall_room((VKByte)vk_boss_room_of(vk_game.stage));
                        vk_psg_music(vk_game.stage==18?VK_DRACULA_ENTRY_MUSIC:VK_MUSIC_BOSS);
                        SYS_doVBlankProcess();
                        continue;
                    }
                    if (item == VK_ITEM_STAGE_EXIT)
                    {
                        next_stage();
                        SYS_doVBlankProcess();
                        continue;
                    }
                    vk_item_view_event(item, &items, &vk_game);
                    /* 8E68: white key 14. PROVISIONAL: the room 7 door gives 1A
                     * (7772: 15 only when C5AC reaches 5, at the castle door). */
                    if (item == VK_ITEM_KEY_TAKEN)
                    {
                        vk_psg_effect(VK_SFX_KEY);
                        vk_candles_white_key_taken(&vk_candles);   /* 8C0B */
                    }
                    else if (item == VK_ITEM_DOOR_OPENED)
                    {
                        /* 7763: the music fades at the door of a boss room. */
                        if (vk_boss_door(vk_game.stage, vk_game.room)) vk_psg_fade();
                        vk_psg_effect(VK_SFX_DOOR_1A);
                    }
                    {
                        /* 6F9A: effect 13 when a contact starts the recoil (C42D set). */
                        const EnemyU8 invuln = vk_enemies.invuln;
                        /* 6B21 -> 710D: object 0A before the timers of 6B27-6B2D. */
                        /* 713B: object 1E (714D) before object 0A (715F). */
                        vk_step_water(!!(pressed & BUTTON_LEFT), !!(pressed & BUTTON_RIGHT),
                                      !!(pressed & BUTTON_B));
                        vk_step_time_stop(!!(pressed & BUTTON_DOWN), !!(pressed & BUTTON_B));
                        vk_step_enemies();
                        if (!invuln && vk_enemies.invuln) vk_psg_effect(VK_SFX_DAMAGE);
                        if (vk_boss_take_exit())
                        {
                            if(vk_game.stage==18){dracula_finish();SYS_doVBlankProcess();continue;}
                            /* B772: C409 = 1, Simon out of the screen, then state 8. */
                            hide_sprites();
                            stage_blank = BOSS_EXIT_FRAMES;
                            SYS_doVBlankProcess();
                            continue;
                        }
                    }
                    {
                        int shot_events;
                        const int shot_hit = vk_step_weapons(throw_launch, &shot_events);
                        candle_hit_view(shot_events, shot_hit);
                    }
                    vk_step_projectile_shield();
                    step_candles();
                    step_walls();
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
            {
                if (stage <= 18 && room < vk_stage_room_count((VKByte)stage))
                    jump_to_stage(stage, room);
                else VDP_drawText("SALLE NON JOUABLE - VISUEL", 2, 2);
            }
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
        if(!paused)vk_item_reveal_refresh();
        SYS_doVBlankProcess();
        vk_show_backdrop();
#ifdef VK_MEASURE_PSG
        vk_psg_measure();   /* measurement build only (research/audio-driver.md) */
#endif
    }
    return 0;
}
