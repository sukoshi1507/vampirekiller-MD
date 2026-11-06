#include "actors.h"
#include "wall_view.h"
#include "frontend.h"
#include "room_view.h"
#include "generated/whip_level2_md.h"
#include "generated/enemy_sprites_md.h"
#include "hud.h"
#include "generated/boss_md.h"
#include "generated/boss6_md.h"
#include "generated/boss9_md.h"
#include "generated/stage4_enemies_md.h"
#include "generated/stage6_enemies_md.h"
#include "generated/stage8_enemies_md.h"
#include "generated/stage9_enemies_md.h"
#include "generated/ghost_md.h"
#include "generated/ghost_a_md.h"
#include "generated/death_flame_md.h"
#include "generated/door_key_md.h"
#include "generated/stage7_enemies_md.h"
#include "crushers.h"
#include "generated/stage5_platform_md.h"
#include "platforms.h"

_Static_assert(HUD_TEXT_TILE_INDEX + HUD_TEXT_TILE_COUNT <= DEATH_TILE_INDEX &&
               DEATH_TILE_INDEX + 2 * VK_PLAYER_DEATH_TILES <= FLAME_A_CLEAR_TILE_INDEX,
               "death pose tiles between the HUD text and the torch tiles");
_Static_assert(STAIR_TILE_INDEX + VK_PLAYER_STAIR_POSES * VK_PLAYER_TILES_PER_POSE <= FLAME_A_CLEAR_TILE_INDEX,
               "stair pose tiles before the torch tiles");
#include "candle_view.h"
#include "psg_player.h"
#include "generated/weapons_md.h"

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

_Static_assert(VK_BOSS_TILE_COUNT <= 128 && VK_ORB_TILE_COUNT <= 128, "boss and orb in the type 5 block");
_Static_assert(VK_STAGE7_ENEMY_TILE_COUNT <= 128, "stage 7 enemies in the type 5 block");
_Static_assert(VK_BOSS6_TILE_COUNT <= 128 && VK_ORB6_TILE_COUNT <= 128, "boss 6 and its orb in the type 5 block");
_Static_assert(VK_BOSS9_TILE_COUNT == VK_BOSS9_FLASH_TILE, "boss 9 tiles, then the blinking cells");
_Static_assert(VK_BOSS9_FLASH_TILE + VK_BOSS9_FLASH_TILES <= 128, "boss 9 and its blinking cells in the type 5 block");
_Static_assert(VK_BOSS_MAX_SPRITES <= ENEMY_MAX * ENEMY_SPRITES, "boss sprites in the enemy sprites");

VKBoss vk_boss;
static int boss_exit;
static void boss_struck(void);
#define VK_SFX_WALKER_FIRE 0x0A   /* A4E8 with A = 0Ah: the walker's fireball (B9A9), stage 9 trace */

/* Thrown weapons (weapons.c): one 16x16 sprite per slot (sprites 1 and 2, in Simon's
 * delayed group: the MSX shows them from D618-D627 with Simon's SAT), whose frame is
 * copied in 4 tiles after the crouch frames when it changes. */
#define SHOT_TILE_INDEX (CROUCH_TILE_INDEX + VK_CROUCH_TILE_COUNT)
_Static_assert(SHOT_TILE_INDEX + VK_WEAPON_SLOTS * VK_WEAPON_TILES_PER_FRAME <= 1440,
               "shot tiles before the SGDK font");
/* Death flame (object 1E, enemies_death.inc): the two cells F4 and F8 (4 tiles each), loaded once in the last tiles before the
 * type 5 block, after the key, HUD box and door tiles of item_view.c (KEY_TILE_INDEX 700: 708 + 2 x 15 + 12 = 750). */
_Static_assert(708 + 2 * VK_HUD_BOX_W * VK_HUD_BOX_H + 2 * (VK_DOOR_ROWS / 8) <= DEATH_FLAME_TILE_INDEX,
               "death flame tiles after the door tiles (item_view.c)");
VKWeapons vk_weapons;
static int shot_loaded[VK_WEAPON_SLOTS];

/* Simon, whip and hall enemies on screen. Their gameplay lives in player.c,
 * whip.c and enemies.c; this module only feeds them and draws the result. */
EnemyState vk_enemies;
/* Hall enemies (enemies.c). Their tiles use PAL2 indices given by the MSX2 colour table. */
static EnemySpawnTimer enemy_timer;
VKPlatforms vk_platforms;
VKCrushers vk_crushers;
static u8 platform_pass;
static u16 enemy_room_key = 0xFFFF;   /* room_key() while the enemies belong to it, else 0 */
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
 * load_simon_color below): Simon's tiles only use indices 1..3 (MSX colours 1, 2 and
 * their OR 3), so his sprites take PAL3 on the blink passes. */
#define SIMON_FLASH_FIRST 1
static const u16 attack_palette[16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x0CA0,0x0EEE
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
/* Unique key across stages including rooms 8: a stride of 8 collided with
 * room 0 of the next stage. Zero remains reserved for outside the castle. */
static u16 room_key(void)
{
    if (vk_front_mode != FRONT_INTERIOR) return 0;
    return (u16)(16 * (vk_game.stage - 1) + vk_game.room + 1);
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

/* Object 1E (714D, before 715F of object 0A): LEFT (7194) or RIGHT (71A1) newly
 * pressed in a jump (C420 = 1), not on a B press (7116), with C701 bit 3 and 5
 * hearts: the holy water leaves from C460. */
void vk_step_water(int left_pressed, int right_pressed, int whip_pressed)
{
    if (whip_pressed || (!left_pressed && !right_pressed) || simon_state() != ENEMY_SIMON_JUMP) return;
    if (!(vk_candles.c701 & VK_ITEM_BIT_1E)) return;
    vk_weapons_water(&vk_weapons, &vk_candles.hearts, left_pressed);
}

static int shot_touch(const void *ctx, int slot, int x, int y, int h, int l)
{
    return vk_shot_touches_slot((const VKWeapons *)ctx, slot, x, y, h, l);
}

int vk_step_weapons(int launch, int *candle_events)
{
    VKWeaponEvents ev;
    const VKWeaponSimon simon = { vk_game.player.x, vk_game.player.y, simon_state(),
                                  vk_game.player.facing };
    const u16 d000 = vk_game.area == VK_AREA_INTERIOR ? vk_game.stage : 0;
    int i, hit = -1;
    *candle_events = 0;
    if (launch)
    {
        vk_weapons_launch(&vk_weapons, vk_whip.level);
        Enemies_rearm(&vk_enemies);   /* 7286 */
        vk_boss_rearm(&vk_boss);
    }
    if (!vk_weapons_frame(&vk_weapons, &simon, vk_candles.weapon, vk_game_collision(&vk_game),
                          (VKByte)d000, &ev))
        return -1;
    for (i = 0; i < ev.sounds; i++) vk_psg_effect(ev.sound[i]);
    if (ev.rearm) { Enemies_rearm(&vk_enemies); vk_boss_rearm(&vk_boss); }
    if (ev.drop) vk_candles_drop(&vk_candles, ev.drop_x, ev.drop_y, 0x1C);   /* 8992 */
    if (ev.lost)
    {
        vk_candles_weapon_lost(&vk_candles);   /* 8E93 */
        vk_set_whip_level(0);
    }
    if (!vk_weapons.shot[0].state && !vk_weapons.shot[1].state) return -1;   /* 801E */
    /* 7F79: the thrown weapons against the fireballs of the boss (D700 objects): 84F0, slot 0 then 1; the ball is
     * destroyed (822A: 100 points, effect 0C), a dagger disappears (7F9E, 74EC); the weapon goes on otherwise. */
    if (vk_boss.in_room)
    {
        int ball, bx, by, bh, bl;
        for (ball = 0; ball < VK_BOSS_SHOTS; ball++)
        {
            int slot;
            if (!vk_boss_shot_target(&vk_boss, ball, &bx, &by, &bh, &bl)) continue;
            slot = shot_touch(&vk_weapons, 0, bx, by, bh, bl) ? 0 : shot_touch(&vk_weapons, 1, bx, by, bh, bl) ? 1 : -1;
            if (slot < 0) continue;
            vk_boss_shot_kill(&vk_boss, ball);
            vk_psg_effect(VK_SFX_HIT);
            vk_hud_add_score(100);
            vk_weapons_enemy_hit(&vk_weapons, slot);
        }
    }
    /* 801E: the first enemy touched by a shot, one per pass. */
    if (room_key() && enemy_room_key == room_key())
    {
        int slot = 0, score = 0;
        {
            /* 801E, HL = 180Ch (28h and 5 for the walkers of stage 9); 8083 clears +E bit 0, 999F sets it again: an axe
             * or a cross touches once going out and once coming back. One object per pass, the first touched. */
            int target, bx, by, bh, bl;
            for (target = 0; target < 2; target++)
            {
                int touched;
                if (!vk_boss_target(&vk_boss, target, &bx, &by, &bh, &bl)) continue;
                touched = shot_touch(&vk_weapons, 0, bx, by, bh, bl) ? 0 : shot_touch(&vk_weapons, 1, bx, by, bh, bl) ? 1 : -1;
                if (touched < 0) continue;
                boss_struck();
                vk_weapons_enemy_hit(&vk_weapons, touched);
                vk_boss_set_armed(&vk_boss, target);
                break;
            }
        }
        /* 8048, by shot type: dagger 1, axe 4, cross 2, water 2. */
        for (i = 0; i < 2; i++)
        {
            static const u8 power[4] = { 1, 4, 2, 2 };
            const int type = vk_weapons.shot[i].type;
            Enemies_shotPower[i] = (type >= VK_SHOT_DAGGER && type < VK_SHOT_DAGGER + 4)
                ? power[type - VK_SHOT_DAGGER] : 1;
        }
        if (Enemies_shotHit(&vk_enemies, shot_touch, &vk_weapons, &slot, &score))
        {
            vk_psg_effect(VK_SFX_HIT);          /* 804C */
            vk_hud_add_score((u32)score * 100); /* 81AB */
            vk_weapons_enemy_hit(&vk_weapons, slot);
        }
    }
    /* 80A6: the first lit candle touched (8460), one per pass; C433 = its mode. */
    for (i = 0; i < vk_candles.count; i++)
    {
        const VKCandleDef *def = vk_candles_def(&vk_candles, i);
        int slot;
        if (!vk_candles_lit(&vk_candles, i)) continue;
        slot = vk_shot_touches_candle(&vk_weapons, def->x, def->y, def->mode);
        if (slot < 0) continue;
        *candle_events = vk_candles_hit(&vk_candles, i);
        if (def->mode == VK_CANDLE_HIDDEN_BLOCK) vk_game_room_block(&vk_game, def->x, def->y, 0);
        vk_weapons_candle_hit(&vk_weapons, slot, def->mode);
        hit = i;
        break;
    }
    /* 80DC: the wall objects, once no candle was hit (the 80C2 path returns). */
    if (hit < 0)
        for (i = 0; i < VK_WALL_RECORDS; i++)
        {
            int slot;
            if (!vk_wall_hittable(&vk_walls, i)) continue;
            /* 83B3: box of 8460 for mode 0 at X + 8, Y + 16. */
            slot = vk_shot_touches_candle(&vk_weapons, (VKByte)(vk_walls.obj[i].x + 8),
                                          (VKByte)(vk_walls.obj[i].y + 16), 0);
            if (slot < 0) continue;
            vk_weapons_enemy_hit(&vk_weapons, slot);   /* 8103: a dagger goes */
            vk_wall_hit(&vk_walls, i);
            vk_psg_effect(VK_SFX_HIT);
        }
    return hit;
}

static void boss_struck(void)
{
    vk_psg_effect(VK_SFX_HIT);
    if (vk_boss_hit(&vk_boss, vk_candles.weapon))
    {
        vk_psg_effect(VK_SFX_BOSS_DEAD);                 /* 7E17 */
        vk_hud_add_score((u32)VK_BOSS_SCORE * 100);      /* 81AB */
    }
}

/* BD01, 490A: the bust at (80h, 70h), 32 x 32, is filled with colour 0 (HMMV), shown on the next frame:
 * the 4 x 4 cells of the room become the blank tile, queued for the VBlank like the sprites. */
static void boss_erase_bust(void)
{
    u16 cells[4] = { 0, 0, 0, 0 };
    u16 row;
    for (row = 14; row < 18; row++) VDP_setTileMapDataRow(BG_A, cells, row, 16, 4, DMA_QUEUE_COPY);
}

/* Stage 9: a walker that blinks (state 3, B96F-B981) shows the second layer of its cells in colour 08 without the CC bit,
 * so the pixels of both layers keep the colour of the first. The blinking cells are made from those of its pose, once
 * per pose, into the tiles after the boss cells; vk_boss_sprites picks them on the passes with +11 even. */
static u8 boss9_flash_pose[2];

static void boss9_flash_cells(void)
{
    int w, i;
    if (vk_boss.kind != 2) return;
    for (w = 0; w < VK_BOSS9_WALKERS; w++)
    {
        const VKBossWalker *k = &vk_boss.walker[w];
        u32 cells[12 * 8];
        if (!k->alive || k->state != 3 || boss9_flash_pose[w] == k->pose) continue;
        for (i = 0; i < 12 * 8; i++)
        {
            const u32 v = vk_boss9_tiles[((k->pose - 0x33) * 12) * 8 + i];
            u32 o = 0;
            int nib;
            for (nib = 0; nib < 8; nib++)
            {
                const u32 c = (v >> (4 * nib)) & 15;
                o |= (c == 12 ? 8u : c == 14 ? 2u : c) << (4 * nib);
            }
            cells[i] = o;
        }
        VDP_loadTileData(cells, (u16)(ENEMY5_TILE_INDEX + VK_BOSS9_FLASH_TILE + w * 12), 12, CPU);
        boss9_flash_pose[w] = k->pose;
    }
}

/* Boss of the room, after the enemies: BE70 or BD19, the D700 fireball, the end script B6AB. */
static void step_boss(const EnemyRoomVars *v)
{
    int events, hit = 0;
    if (!vk_boss.in_room) return;
    events = vk_boss_frame(&vk_boss, v->simonX, v->simonY, v->difficulty, vk_enemies.frozen,
                           vk_enemies.energy);
    boss9_flash_cells();
    if (events & VK_BOSS_EV_FIRE) vk_psg_effect(vk_boss.kind == 2 ? VK_SFX_WALKER_FIRE : VK_SFX_DAMAGE);   /* A4E8: 0A (stage 9) */
    if (events & VK_BOSS_EV_FLASH) vk_enemies.flash = ENEMY_FLASH_START;
    if (events & VK_BOSS_EV_ORB_TILES)
    {
        /* 57C0 and the room palette: entries 4 and 6 turn dark red (stage 3, snapshots 1161, 1282); stage 6:
         * entries 5 and 7 (trace D, snapshots before and after the orb). */
        static const u16 red[2] = { 0x006, 0x00A };
        const u16 first = vk_boss.kind ? 5 : 4;   /* stages 6 and 9: trace, entries 5 and 7 */
        u16 pal;
        if (vk_boss.kind) VDP_loadTileData(vk_orb6_tiles, ENEMY5_TILE_INDEX, VK_ORB6_TILE_COUNT, CPU);
        else VDP_loadTileData(vk_orb_tiles, ENEMY5_TILE_INDEX, VK_ORB_TILE_COUNT, CPU);
        for (pal = PAL0; pal <= PAL2; pal = (u16)(pal + PAL2))
        {
            PAL_setColor((u16)(pal * 16 + first), red[0]);
            PAL_setColor((u16)(pal * 16 + first + 2), red[1]);
        }
    }
    if (events & VK_BOSS_EV_ERASE) boss_erase_bust();
    if (events & VK_BOSS_EV_MUSIC) vk_psg_music(VK_MUSIC_BOSS_END);
    if (events & VK_BOSS_EV_HEAL)
    {
        vk_enemies.energy++;
        vk_psg_effect(VK_SFX_HEAL);
    }
    if (events & VK_BOSS_EV_EXIT) boss_exit = 1;
    vk_hud_enemy(vk_boss.energy);
    vk_boss_orb_touch(&vk_boss, v->simonX, v->simonY, v->simonState);
    if (vk_boss_touch_x(&vk_boss, v->simonX, v->simonY, v->simonState) >= 0)
        hit = Enemies_contact(&vk_enemies, v, VK_BOSS_DAMAGE_CONTACT,
                              vk_boss_touch_x(&vk_boss, v->simonX, v->simonY, v->simonState), 1);
    else
    {
        const int shot = vk_boss_shot_touches(&vk_boss, v->simonX, v->simonY, v->simonState);
        if (shot >= 0) hit = Enemies_contact(&vk_enemies, v, 1, vk_boss.shot[shot].x >> 8, 0);
    }
    if (hit)
    {
        vk_whip_stop(&vk_whip);
        vk_game_hit(&vk_game, vk_enemies.recoilDir, vk_enemies.energy == 0);
        vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    }
}

int vk_boss_take_exit(void)
{
    const int exit = boss_exit;
    boss_exit = 0;
    return exit;
}

void vk_step_platforms(void)
{
    VKPlayer *const p = &vk_game.player;
    if (!enemy_room_key || enemy_room_key != room_key())
    {
        vk_player_platform = 0;
        return;
    }
    platform_pass ^= 1;
    if (platform_pass)
    {
        vk_platforms_step(&vk_platforms);   /* 5C80 */
        vk_crushers_step(&vk_crushers);     /* 5C7D */
    }
    /* 6B39 -> 8524. The end of a jump is a mode 4 of the port but stays in state 1 on the MSX (6D1C): no wait. */
    vk_platforms_probe(&vk_platforms, p->x, p->y, p->mode == 4 && vk_game.ground_fall, p->phase);
    vk_player_platform = vk_platforms.under;
    /* 6B6B, 6DCC, 7021: on a platform Simon follows it unless a wall stops him (7BA9, 7C05). */
    if (platform_pass && p->mode == 0 && !p->on_stairs &&
        (vk_game.recoil == VK_RECOIL_NONE || vk_game.recoil == VK_RECOIL_FLOOR))
    {
        const int dx = vk_platforms_carry(&vk_platforms);
        if (dx && !vk_wall(vk_game_collision(&vk_game), p->x, p->y, dx, 0, vk_game.stage, 0))
            p->x = (VKByte)(p->x + dx);
    }
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
        /* 6395, 6398: the records, then a first step while the room loads. */
        vk_platforms_room(&vk_platforms, key ? vk_game.stage : 0, vk_game.room);
        vk_platforms_step(&vk_platforms);
        platform_pass = 0;
        vk_crushers_room(&vk_crushers, key ? vk_game.stage : 0, vk_game.room);
        if (key)
        {
            VDP_loadTileData((const u32 *)vk_enemy_tiles, ENEMY_TILE_INDEX, VK_ENEMY_TILE_COUNT, CPU);
            VDP_loadTileData((const u32 *)vk_enemy4_tiles, ENEMY4_TILE_INDEX, VK_ENEMY4_TILE_COUNT, CPU);
            /* Stage 2 rooms 4 and 5: the fishman and its splash take the type 5 block. */
            if (vk_game.stage == 4 || vk_game.stage == 5)
            {
                VDP_loadTileData(vk_stage4_enemy_tiles, ENEMY5_TILE_INDEX, VK_STAGE4_ENEMY_TILE_COUNT, CPU);
                if (vk_game.stage == 5)
                    VDP_loadTileData(vk_stage5_platform_tiles, ENEMY5_TILE_INDEX + VK_PLATFORM_PATTERN - VK_ENEMY5_FIRST_PATTERN,
                                     VK_STAGE5_PLATFORM_TILE_COUNT, CPU);
            }
            else if (vk_game.stage == 6)
                VDP_loadTileData(vk_stage6_enemy_tiles, ENEMY5_TILE_INDEX, VK_STAGE6_ENEMY_TILE_COUNT, CPU);
            else if (vk_game.stage == 9)
                VDP_loadTileData(vk_stage9_enemy_tiles, ENEMY5_TILE_INDEX, VK_STAGE9_ENEMY_TILE_COUNT, CPU);
            else if (vk_game.stage == 8)
                VDP_loadTileData(vk_stage8_enemy_tiles, ENEMY5_TILE_INDEX, VK_STAGE8_ENEMY_TILE_COUNT, CPU);
            else if (vk_game.stage == 7)
                VDP_loadTileData(vk_stage7_enemy_tiles, ENEMY5_TILE_INDEX, VK_STAGE7_ENEMY_TILE_COUNT, CPU);
            else if ((Enemies_roomFlags(vk_game.stage, vk_game.room) & 0x02))
                VDP_loadTileData((const u32 *)vk_enemy2_tiles, ENEMY5_TILE_INDEX, VK_ENEMY2_TILE_COUNT, CPU);
            else
                VDP_loadTileData((const u32 *)vk_enemy5_tiles, ENEMY5_TILE_INDEX, VK_ENEMY5_TILE_COUNT, CPU);
            /* Object 1C of the candles of object 15 (stages 7 to 9): the two poses of the room, cells B4 and B8 of the block. */
            if (vk_game.stage >= 7 && vk_game.stage <= 9)
            {
                const EnemyU8 base = Enemies_ghostBase((EnemyU8)vk_game.stage, (EnemyU8)vk_game.room);
                if (base)
                    VDP_loadTileData(vk_ghost_tiles[base == 0x9B ? 0 : base == 0x9F ? 1 : 2],
                                     ENEMY5_TILE_INDEX + (0xB4 - 0x40), VK_GHOST_TILE_COUNT, CPU);
            }
            /* Object 1A (stages 4 to 6): the same image in colour 15, whatever the pose base 9B, 9D or 9F of the room; cells 70 and 74
             * (stages 4, 5) or 80 and 84 (stage 6) of the block. */
            else if (vk_game.stage >= 4 && vk_game.stage <= 6 &&
                     Enemies_ghostBase((EnemyU8)vk_game.stage, (EnemyU8)vk_game.room))
                VDP_loadTileData(vk_ghost_a_tiles, ENEMY5_TILE_INDEX + (Enemies_ghostCell((EnemyU8)vk_game.stage) - 0x40),
                                 VK_GHOST_A_TILE_COUNT, CPU);
        }
        /* 6319-6333. PROVISIONAL: the bar is full again outside the boss room. */
        if (vk_boss_room(vk_game.stage, vk_game.room))
        {
            vk_boss_begin_stage(&vk_boss, vk_game.stage);
            if (vk_boss.kind == 2) VDP_loadTileData(vk_boss9_tiles, ENEMY5_TILE_INDEX, VK_BOSS9_TILE_COUNT, CPU);
            else if (vk_boss.kind) VDP_loadTileData(vk_boss6_tiles, ENEMY5_TILE_INDEX, VK_BOSS6_TILE_COUNT, CPU);
            else VDP_loadTileData(vk_boss_tiles, ENEMY5_TILE_INDEX, VK_BOSS_TILE_COUNT, CPU);
            boss9_flash_pose[0] = boss9_flash_pose[1] = 0;
        }
        else vk_boss_reset(&vk_boss);
        boss_exit = 0;
        vk_hud_enemy(VK_BOSS_ENERGY);
    }
    if (!key) return;
    vars.stage = vk_game.stage;
    vars.room = vk_game.room;
    vars.simonX = vk_game.player.x;
    vars.simonY = vk_game.player.y;
    vars.sideFlag = 0;
    vars.difficulty = vk_walls.d012;   /* D012, changed by the wall objects (9333, 933C) */
    vars.map = vk_game_collision(&vk_game);   /* floor for falling enemies */
    vars.simonState = simon_state();          /* C420 for the contact box 847A */
    vars.facingLeft = vk_game.player.facing;  /* C42C: armor of object 03 (8179) */
    vars.armorOn = (EnemyU8)!!(vk_candles.c701 & VK_ITEM_BIT_03);   /* 8179: C701 bit 4 */
    /* C420 = 5 while Simon is knocked back: no enemy contact (7EF4). */
    vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    /* 7064: the death (C420 = 6) clears C42D: no more blinking. */
    if (vk_game.recoil == VK_RECOIL_DEAD) vk_enemies.invuln = 0;
    const EnemyU8 armor = vk_enemies.armor;
    if (Enemies_frame(&vk_enemies, &enemy_timer, &vars))
    {
        /* 7EC1: the whip stops (C422 := 0) and Simon is knocked back. */
        vk_whip_stop(&vk_whip);
        vk_game_hit(&vk_game, vk_enemies.recoilDir, vk_enemies.energy == 0);
        vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
    }
    step_boss(&vars);
    /* 85A6: after the move of the crushers, the guards C420 = 6, C42D and C43A; no armor, damage 8 or 16. The knockback
     * keeps the side of the last hit (C43C is not written, trace: left from the start). */
    {
        const int damage = vk_crushers_hit(&vk_crushers, vars.simonX, vars.simonY);
        if (damage && vk_game.recoil != VK_RECOIL_DEAD)
        {
            const EnemyS8 side = vk_enemies.recoilDir ? vk_enemies.recoilDir : -1;
            if (Enemies_contact(&vk_enemies, &vars, (EnemyU8)damage, vars.simonX, 0))
            {
                vk_enemies.recoilDir = side;
                vk_whip_stop(&vk_whip);
                vk_game_hit(&vk_game, side, vk_enemies.energy == 0);
                vk_enemies.recoil = (EnemyU8)vk_game_recoiling(&vk_game);
            }
        }
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
        /* 84A7/84D9: the chain whip (C416 = 1) reaches 24 px and widens the box by 4. */
        const int chain = vk_candles.weapon == 1;
        vk_whip_bounds(&vk_whip, &left, &top, &right, &bottom);
        hits = Enemies_whipHit(&vk_enemies, left, top, right, bottom);
        /* 7DAD: one test per stroke, in the pass with C422 = 3 (vk_whip_active),
         * from Simon's place in that pass (C427/C425, whip.c). Type 5: */
        hits += Enemies_whipHitType5(&vk_enemies, vk_whip.x, vk_whip.y,
                                     vk_whip.facing, vk_whip.crouched, chain);
        /* Type 4 (bats): ROM box of class 2, on the side Simon faces (84A7). */
        hits += Enemies_whipHitType4(&vk_enemies, vk_whip.x, vk_whip.y,
                                     vk_whip.facing, vk_whip.crouched, chain);
        /* Stage 2 type 1: ROM box of class 1 (enemies.h). */
        hits += Enemies_whipHitType1Rom(&vk_enemies, vk_whip.x, vk_whip.y,
                                        vk_whip.facing, vk_whip.crouched, chain);
        /* Stage 2 type 2 (fishman): ROM box of class 1. */
        fish = Enemies_whipHitType2(&vk_enemies, vk_whip.x, vk_whip.y,
                                    vk_whip.facing, vk_whip.crouched, chain);
        /* 7DF9 -> 81AB: score of the killed enemy, table 81CE in hundreds: types
         * 1, 4 and 5 give 100, type 2 (fishman) 200. */
        if (hits || fish) vk_hud_add_score((u32)hits * 100 + (u32)fish * 200);
        hits += fish;
        {
            /* Objects 06 and 08: 200 points when killed; 7257 sets +E bit 0 again. */
            int touched;
            const int killed = Enemies_whipHitStage4(&vk_enemies, vk_whip.x, vk_whip.y,
                                                     vk_whip.facing, vk_whip.crouched, chain, &touched);
            if (killed) vk_hud_add_score((u32)Enemies_killScore * 100);
            hits += touched;
            Enemies_rearm(&vk_enemies);
            vk_boss_rearm(&vk_boss);
        }
        /* 7F65 (D700 loop) and 7DD4 (C800 objects): effect 0C on a whip contact. */
        if (hits) vk_psg_effect(VK_SFX_HIT);
        if (vk_boss.in_room)
        {
            /* 7DAD, 83F2: the boss; 7F65, 8320: its fireball, 100 points (822A). */
            const int state = vk_whip.crouched ? 2 : 0;
            const int shot = vk_boss_shot_whip_touches(&vk_boss, vk_whip.x, vk_whip.y,
                                                       vk_whip.facing, state, chain);
            if (vk_boss_whip_touches(&vk_boss, vk_whip.x, vk_whip.y, vk_whip.facing, state, chain))
                boss_struck();
            if (shot >= 0)
            {
                vk_boss.shot[shot].active = 0;
                vk_psg_effect(VK_SFX_HIT);
                vk_hud_add_score(100);
            }
        }
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
    /* Static, not on the stack: with Enemies_sprites inlined, GCC 16.1 (-O3, LTO) wrote
     * the fields of the first entry of an enemy 252 bytes too low, below the stack
     * (lea %sp@(0,%d0:l),%a3 then %a3@(-166), %a3@(-164)); the first layer of the panther
     * was then drawn from whatever the stack held. */
    static EnemySprite list[ENEMY_MAX * ENEMY_SPRITES + VK_PLATFORM_MAX * VK_PLATFORM_CELLS + VK_CRUSHER_MAX * 5];
    u16 count = 0;
    u16 i;
    if (!enemy_room_key || enemy_room_key != room_key()) return 0;
    for (i = 0; i < ENEMY_MAX; i++)
        count = (u16)(count + Enemies_sprites(&vk_enemies, (EnemyU8)i, &list[count]));
    /* 90D8, 9112: two cells per platform; the pattern takes a free slot of the block (patterns D0, D4 on the MSX). */
    for (i = 0; i < VK_PLATFORM_MAX; i++)
    {
        const VKPlatform *const s = &vk_platforms.slot[i];
        u16 c;
        if (!s->id) continue;
        for (c = 0; c < VK_PLATFORM_CELLS; c++, count++)
        {
            list[count].x = (EnemyS16)(s->x + c * 16);
            list[count].y = s->y;
            list[count].pattern = VK_PLATFORM_PATTERN;
            list[count].bank = 1;
        }
    }
    /* 8FEA, 4946: the 32 x 16 image is copied to (X, Y - 4) after each move; its first rows are a chain link, so
     * the chain grows from y 5Ch (the room draws the top of it) down to the body. Cells of 16 rows, the first one
     * cut to 4, 8 or 12 rows so that the chain starts at 5Ch. */
    for (i = 0; i < VK_CRUSHER_MAX; i++)
    {
        const VKCrusher *const c = &vk_crushers.slot[i];
        const int top = c->y - 4;
        int len, cut, at;
        if (!c->state) continue;
        len = top - 0x5C;
        cut = len & 15;
        at = 0x5C;
        if (cut)
        {
            list[count].x = (EnemyS16)(c->x + 8);
            list[count].y = (EnemyU8)at;
            list[count].pattern = (EnemyU8)(VK_CRUSHER_CHAIN_FULL + cut);
            list[count++].bank = 1;
            at += cut;
        }
        for (; at + 16 <= top; at += 16, count++)
        {
            list[count].x = (EnemyS16)(c->x + 8);
            list[count].y = (EnemyU8)at;
            list[count].pattern = VK_CRUSHER_CHAIN_FULL;
            list[count].bank = 1;
        }
        for (len = 0; len < 2; len++, count++)
        {
            list[count].x = (EnemyS16)(c->x + len * 16);
            list[count].y = (EnemyU8)top;
            list[count].pattern = (EnemyU8)(VK_CRUSHER_BODY + 4 * len);
            list[count].bank = 1;
        }
    }
    if (vk_boss.in_room && !count)
    {
        static VKBossSprite boss[VK_BOSS_MAX_SPRITES];
        const int n = vk_boss_sprites(&vk_boss, boss);
        for (i = 0; i < n; i++)
            VDP_setSpriteFull((u16)(ENEMY_SPRITE_BASE + i), boss[i].x, boss[i].y,
                SPRITE_SIZE(boss[i].size, boss[i].size),
                TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, (u16)(ENEMY5_TILE_INDEX + boss[i].tile)),
                (u16)(i + 1 < n ? ENEMY_SPRITE_BASE + i + 1 : tail));
        return (u16)n;
    }
    for (i = 0; i < count; i++)
    {
        const u8 pattern = list[i].pattern;
        const u16 tile = list[i].bank == 2
            ? (u16)(DEATH_FLAME_TILE_INDEX + pattern)
            : list[i].bank
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
 * The colour is the one of the last pass (enemies.c), so a pickup shows from the
 * next odd pass on, as on the MSX (openMSX trace). The MD sprites switch to PAL3, whose entries 1..3 then hold that colour of the
 * room palette (PAL2, the MSX palette). Returns the MSX colour, 0 for none. */
static u8 simon_color(void)
{
    return (u8)(room_key() ? Enemies_simonColor(&vk_enemies) : 0);
}

/* PAL3 entries 1..3 for the silhouette colour of the sprite table being sent. */
static void load_simon_color(u8 color)
{
    if (!color || color == simon_flash_color) return;
    /* Sent in the next VBlank with the sprite table (DMA queue). */
    static u16 rgb[3];
    rgb[0] = rgb[1] = rgb[2] = PAL_getColor((u16)(PAL2 * 16 + color));
    PAL_setColors((u16)(PAL3 * 16 + SIMON_FLASH_FIRST), rgb, 3, DMA_QUEUE);
    simon_flash_color = color;
}

/* Display delay (research/frame-timing.md). The MSX sends its sprite buffers (SAT
 * D600, colours D400) at the start of the next pass (6560/658A, ~1 ms after the
 * VBLANK): a sprite computed in a pass shows 2 frames after the pass starts, while
 * the bitmap (decor, HUD, candles, drops), the border (CHGCLR) and the sounds change
 * during the pass or at the next VBLANK. Here the sprite table of a loop turn goes
 * out at the next VBlank (1 frame): Simon, his whip and the enemies, the MSX
 * sprites, are therefore shown from the previous turn's entries (2 frames, as on
 * the MSX), with the shield tiles and silhouette colour they were built with. The
 * candles, drops, HUD objects and courtyard torches, MSX bitmap, stay at 1 frame. */
#define SIMON_SPRITES 7
#define ENEMY_SPRITE_MAX (ENEMY_MAX * ENEMY_SPRITES + VK_PLATFORM_MAX * VK_PLATFORM_CELLS + VK_CRUSHER_MAX * 5)
static VDPSprite late_simon[SIMON_SPRITES];
static VDPSprite late_enemies[ENEMY_SPRITE_MAX];
static u16 late_enemy_count;
static bool late_valid;
static int late_shield = -1;
static u8 late_color;
static int late_shot[VK_WEAPON_SLOTS] = { -1, -1 };

void vk_actors_flush(void)
{
    late_valid = FALSE;
    late_enemy_count = 0;
    late_shield = -1;
    late_color = 0;
    late_shot[0] = late_shot[1] = -1;
}

/* Exchanges the n entries at cache[first] (this turn) with the delayed ones. */
static void swap_entries(VDPSprite *late, u16 first, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++)
    {
        const VDPSprite now = vdpSpriteCache[first + i];
        vdpSpriteCache[first + i] = late[i];
        late[i] = now;
    }
}

/* 7604 (object 05, C43E): BDRCLR F3EB = 0Eh or 0 then CHGCLR. The MD backdrop
 * (register 7, PAL0 = the room palette) stands for the MSX border colour; it shows
 * wherever the planes and sprites are transparent. Called right after the VBlank,
 * so the whole picture changes at once, one frame after the pass. On the MSX,
 * CHGCLR (~8 ms into the pass) changes the border from line ~80 of that frame and
 * the whole next frame; the sprites come one frame later (research/frame-timing.md). */
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
     * tick C003 is set: 4 frames hidden, 4 shown, from the pass of the hit on
     * (openMSX trace, research/castle-candles.md 9.8). */
    if (Enemies_simonHidden(&vk_enemies)) parked.y = 255;
    const u8 color = simon_color();
    const u16 simon_pal = color ? PAL3 : PAL2;
    /* Enemy sprites come after the player chain: sprite 6 (or 7 when the target is
     * shown) links to ENEMY_SPRITE_BASE when there are enemies, then the candles and
     * objects of candle_view.c close the chain. The enemies shown are those of the
     * previous turn (late_enemy_count), see swap_entries. */
    const u16 candle_count = vk_candle_view_sprites();
    const u16 candle_link = candle_count ? CANDLE_SPRITE_BASE : 0;
    const u16 enemy_count = show_enemy_sprites(candle_link);
    const u16 shown_enemies = late_valid ? late_enemy_count : 0;
    const u16 enemy_link = shown_enemies ? ENEMY_SPRITE_BASE : candle_link;
    const bool target_visible = vk_targets_visible();
    const int attack_frame = vk_whip_arm_frame(&vk_whip);
    const int crouch_mode = !player->on_stairs && (player->crouched || (attack_frame && vk_whip.crouched));
    const u16 crouch_frame = attack_frame ? (u16)vk_whip_crouch_visual_step(&vk_whip) : 0;
    const u16 crouch_facing = attack_frame ? vk_whip.facing : player->facing;
    const int shield = shield_pose(player, crouch_mode, attack_frame);
    int shot_frame[VK_WEAPON_SLOTS];
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
    /* Sprites 1 and 2: the shots of C450 and C460 (7535: SAT D618/D620 at X - 8,
     * Y - 16, shown one line lower, the two layers composed in one PAL2 frame by
     * tools/build_weapons_md.py). Sprites 3 and 4 stay parked: the ROM compositions
     * already hold the lash and the arm. */
    {
        u16 k;
        for (k = 0; k < VK_WEAPON_SLOTS; k++)
        {
            const VKShot *shot = &vk_weapons.shot[k];
            const int frame = shot_frame[k] = vk_weapons_frame_index(shot);
            VDP_setSpriteFull((u16)(1 + k), frame < 0 ? -32 : (s16)shot->x - 8,
                frame < 0 ? -32 : (s16)shot->y - 15, SPRITE_SIZE(2, 2),
                TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, SHOT_TILE_INDEX + k * VK_WEAPON_TILES_PER_FRAME),
                (u16)(2 + k));
        }
        for (k = 3; k <= 4; k++)
            VDP_setSpriteFull(k, -32, -32, SPRITE_SIZE(1, 1), 0, (u16)(k + 1));
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
    /* This turn's Simon and enemies go to late_*, the previous turn's to the table,
     * with the shield tiles and the silhouette colour they need (same VBlank). */
    {
        const int shield_now = shield;
        swap_entries(late_simon, 0, SIMON_SPRITES);
        swap_entries(late_enemies, ENEMY_SPRITE_BASE,
                     enemy_count > shown_enemies ? enemy_count : shown_enemies);
        if (!late_valid)
            for (u16 i = 0; i < SIMON_SPRITES; i++)
                VDP_setSpriteFull(i, -32, -32, SPRITE_SIZE(1, 1), 0, (u8)(i + 1));
        VDP_setSpriteLink(SIMON_SPRITES - 1, target_visible ? 7 : enemy_link);
        if (shown_enemies) VDP_setSpriteLink((u16)(ENEMY_SPRITE_BASE + shown_enemies - 1), candle_link);
        if (late_valid && late_shield >= 0 && late_shield != shield_loaded)
        {
            VDP_loadTileData((const u32 *)vk_player_shield_tiles[late_shield], SHIELD_TILE_INDEX,
                             VK_PLAYER_TILES_PER_POSE, DMA_QUEUE);
            shield_loaded = late_shield;
        }
        if (late_valid) load_simon_color(late_color);
        if (late_valid)
            for (u16 k = 0; k < VK_WEAPON_SLOTS; k++)
                if (late_shot[k] >= 0 && late_shot[k] != shot_loaded[k])
                {
                    VDP_loadTileData(&vk_weapon_tiles[late_shot[k] * VK_WEAPON_TILES_PER_FRAME * 8],
                                     (u16)(SHOT_TILE_INDEX + k * VK_WEAPON_TILES_PER_FRAME),
                                     VK_WEAPON_TILES_PER_FRAME, DMA_QUEUE);
                    shot_loaded[k] = late_shot[k];
                }
        late_shot[0] = shot_frame[0];
        late_shot[1] = shot_frame[1];
        late_enemy_count = enemy_count;
        late_shield = shield_now;
        late_color = color;
        late_valid = TRUE;
    }
    /* Courtyard flames or training target on sprites 7.. (room_view.c). */
    if (vk_draw_targets(enemy_link))
    {
        VDP_updateSprites(candle_count ? CANDLE_SPRITE_BASE + candle_count : 7 + COURTYARD_MAX_FLAMES,
                          DMA_QUEUE);
        return;
    }
    VDP_updateSprites(candle_count ? CANDLE_SPRITE_BASE + candle_count
                      : shown_enemies ? ENEMY_SPRITE_BASE + shown_enemies : (target_visible ? 8 : 7), DMA_QUEUE);
}

/* Whip level given by an object (C416, candles.c): loads its standing frames, the
 * level 2 frames (throw pose, upper compositions 12-14) for the thrown weapons 2..4. */
static void load_whip_level(int level)
{
    if (level == 1)
        VDP_loadTileData((const u32 *)vk_whip_level1_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
    else if (level >= VK_WHIP_LEVEL_THROW)
        VDP_loadTileData((const u32 *)vk_whip_level2_tiles, WHIP_LEVEL_TILE_INDEX, WHIP_LEVEL_TILE_COUNT, CPU);
}

void vk_set_whip_level(int level)
{
    if (vk_whip_set_level(&vk_whip, level) < 0) return;
    whip_level_choice = level;
    load_whip_level(level);
}

void vk_load_whip_graphics(void)
{
    VDP_loadTileData(whip_tiles, WHIP_TILE_INDEX, 3, CPU);
    VDP_loadTileData(arm_tiles, ARM_TILE_INDEX, 4, CPU);
    VDP_loadTileData((const u32 *) vk_simon_attack_tiles, ATTACK_TILE_INDEX,
                     VK_ATTACK_FRAME_COUNT * VK_ATTACK_TILES_PER_FRAME, CPU);
    PAL_setPalette(PAL3, attack_palette, CPU);
    VDP_loadTileData(target_tiles, TARGET_TILE_INDEX, 4, CPU);
    /* MSX colour numbers: shown in PAL2, the room palette, as the VRAM copies of the MSX. */
    VDP_loadTileData(vk_flame_fx_tiles, FLAME_TILE_INDEX,
                     (VK_FLAME_FRAMES + VK_SPARK_FRAMES) * VK_FLAME_TILES_PER_FRAME, CPU);
    simon_flash_color = 0;
    vk_actors_flush();
    VDP_loadTileData((const u32 *)vk_simon_crouch_tiles, CROUCH_TILE_INDEX, VK_CROUCH_TILE_COUNT, CPU);
    vk_whip_reset(&vk_whip);
    vk_whip_set_level(&vk_whip, whip_level_choice);
    load_whip_level(vk_whip_level(&vk_whip));
    shot_loaded[0] = shot_loaded[1] = -1;
    VDP_loadTileData(vk_death_flame_tiles, DEATH_FLAME_TILE_INDEX, VK_DEATH_FLAME_TILE_COUNT, CPU);
    vk_hide_targets();
    shield_loaded = -1;              /* the scene tiles may have covered the slot */
}

