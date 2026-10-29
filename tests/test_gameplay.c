/* Native smoke tests: gcc -std=c99 -Wall -Wextra -Werror ... */
#include <assert.h>
#include <string.h>
#include "generated/stage1_collision.h"
#include "gameplay.h"
#include "room_manager.h"
#include "generated/player_assets.h"

/* Synthetic, empty collision maps isolate scene routing from graphical assets. */
const VKByte vk_courtyard_maps[3][768] = {{0}};
const VKByte vk_courtyard_neighbors[3][4] = {
    {255,255,255,1}, {255,255,0,2}, {255,255,1,255}
};

int main(void)
{
    VKGame g;
    vk_game_begin(&g);
    assert(g.area == VK_AREA_COURTYARD && g.room == 0 && g.collision_verified);
    assert(vk_game_collision(&g) == vk_courtyard_maps[0]);

    /* Force right-side boundary and verify continuity. */
    g.player.x = 248;
    g.player.y = 100;
    g.player.mode = 1;
    g.player.phase = 3;
    g.player.facing = 0;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_ROOM);
    assert(g.room == 1 && g.player.x == 10);
    assert(g.player.mode == 1 && g.player.phase != 0);
    assert(vk_game_collision(&g) == vk_courtyard_maps[1]);

    /* Courtyard room 2: UP at x=192..216 does not change room (openMSX). */
    g.room = 2;
    g.player.x = 200;
    g.player.y = 176;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2);
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2);
    /* Courtyard room 2 -> hall room 0 through the castle door (openMSX runs A and
     * B, research/castle-entrance.md): facing right at x 216 starts the entrance. */
    g.player.x = 216;
    g.player.y = 176;
    g.player.mode = 0;
    g.player.facing = 0;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_ENTRANCE);
    assert(g.entrance == VK_ENTRANCE_WALK && vk_game_entrance_locked(&g));
    {
        /* Input is ignored; one pixel per four frames: x 217 at t 4, 232 at t 64. */
        int t;
        for (t = 1; t <= 64; t++)
        {
            assert(vk_game_tick(&g, t & 1 ? -1 : 0, 1, 0) == VK_EVENT_NONE);
            assert(g.player.x == 216 + t / 4);
            assert(g.area == VK_AREA_COURTYARD && g.player.y == 176);
            if (t == 8) assert(g.player.pose == 1);   /* walk cycle 0,1,2,1 */
        }
        assert(g.entrance == VK_ENTRANCE_EXIT);
        /* Courtyard shown 2 more frames, then 36 black frames, then the hall. */
        assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_NONE);
        assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_NONE);
        assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_ENTRANCE_DARK);
        assert(g.area == VK_AREA_INTERIOR && g.room == 0);
        for (t = 68; t < 103; t++) assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_NONE);
        assert(g.player.x == 16);
    }
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_ENTER);
    assert(!vk_game_entrance_locked(&g));
    assert(g.area == VK_AREA_INTERIOR && g.room == 0 && g.collision_verified);
    assert(g.player.x == 16 && g.player.y == 192);   /* frame 1555, on the floor */
    assert(vk_grounded(vk_game_collision(&g), g.player.x, g.player.y, 1, 0));
    assert(memcmp(vk_game_collision(&g), vk_player_room, 768) == 0);
    assert(vk_game_debug_room(&g,1));
    assert(g.room == 1 && !g.collision_verified);
    assert(vk_game_collision(&g) == vk_stage1_collision[1]);
    assert(!vk_game_debug_room(&g, 8));
    assert(g.room == 1);
    /* Stage 1 room 1, flight A (tile 12, 7 steps): HAUT at the foot (48,192)
     * climbs to the landing (112,128), the 64 px of research/stairs.md. */
    {
        int frames;
        g.player.x = 48;
        g.player.y = 192;
        g.player.on_stairs = 0;
        g.player.mode = 0;
        assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE);
        assert(g.player.on_stairs && g.player.x == 48 && g.player.y == 192);
        assert(g.player.stair_dir == 1 && g.player.stair_len == 64);
        /* Releasing the key holds Simon on the step. */
        for (frames = 0; frames < 6; frames++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.player.on_stairs && g.player.x == 48 && g.player.y == 192);
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++) {
            vk_game_tick_controls(&g, 0, 0, 1, 0);
            assert(g.player.x - 48 == 192 - g.player.y);
        }
        assert(!g.player.on_stairs && g.player.x == 112 && g.player.y == 128);
        /* Descending with BAS from the same foot returns to (48,192). */
        g.player.x = 48;
        g.player.y = 192;
        assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE && g.player.on_stairs);
        for (frames = 0; frames < 200; frames++) {
            if (g.player.stair_anim == 16) break;
            vk_game_tick_controls(&g, 0, 0, 1, 0);
        }
        assert(g.player.stair_anim == 16);
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(!g.player.on_stairs && g.player.x == 48 && g.player.y == 192);
    }
    /* X=36 is outside the entry window of that foot: no staircase. */
    g.player.x = 36;
    g.player.y = 192;
    g.player.mode = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE && !g.player.on_stairs);
    /* Flight C (tile 13, 8 steps) from its foot (240,96) climbs past the top of the
     * playfield: the hall link 1 -> 5 moves Simon to room 5, on a floor. */
    assert(vk_game_debug_room(&g, 1));
    g.player.x = 240;
    g.player.y = 96;
    g.player.mode = 0;
    {
        VKEvent event = VK_EVENT_NONE;
        int frames;
        for (frames = 0; frames < 300 && event != VK_EVENT_VERTICAL; frames++)
            event = vk_game_tick_controls(&g, 0, 0, 1, 0);
        /* The MSX exit fires at Y < 48 (x 191): Simon lands at (207,224), gliding. */
        assert(event == VK_EVENT_VERTICAL && g.room == 5 && g.player.on_stairs);
        assert(g.player.stair_glide && g.player.x == 207 && g.player.y == 224);
        /* HAUT glides him up the 32 px diagonal to the floor at (175,192). */
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 1, 0);
        assert(!g.player.on_stairs && g.player.x == 175 && g.player.y == 192);
    }
    assert(vk_stage1_links[1].up == 5 && vk_stage1_links[5].down == 1);
    assert(vk_stage1_links[3].up == 7 && vk_stage1_links[7].down == 3);
    /* Stage 2 room 4 has no room below: falling into the water at x 172 is a
     * fall out of play (76BC), Simon parked at Y FA and frozen there. */
    {
        VKGame w;
        int frames;
        VKEvent ev = VK_EVENT_NONE;
        vk_game_begin(&w);
        vk_game_enter_stage(&w, 2);
        w.room = 4;
        w.player.x = 172; w.player.y = 200; w.player.mode = 4; w.player.phase = 0;
        for (frames = 0; frames < 60 && ev != VK_EVENT_FALL; frames++)
            ev = vk_game_tick_controls(&w, 0, 0, 0, 0);
        assert(ev == VK_EVENT_FALL && w.player.y == VK_FALL_Y && w.room == 4);
        assert(vk_game_tick_controls(&w, 1, 1, 0, 0) == VK_EVENT_NONE);
        assert(w.player.y == VK_FALL_Y && w.player.x == 172);
    }
    /* Bottom edge Y 225 in room 5 sends Simon down to room 1 at its top. */
    g.player.x = 192;
    g.player.y = 225;
    g.player.mode = 4;
    g.player.on_stairs = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 0, 0) == VK_EVENT_VERTICAL);
    assert(g.room == 1 && g.player.y == 48 && g.player.x == 192);
    /* From the room 5 glide foot (207,224), BAS drops through Y 225 into room 1
     * at X 191, the -16 of the MSX exit. */
    assert(vk_game_debug_room(&g, 5));
    g.player.x = 207; g.player.y = 224; g.player.on_stairs = 1;
    g.player.stair_dir = 0; g.player.stair_len = 32; g.player.stair_anim = 0;
    g.player.stair_glide = 1;
    {
        VKEvent event = VK_EVENT_NONE;
        int frames;
        for (frames = 0; frames < 20 && event != VK_EVENT_VERTICAL; frames++)
            event = vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(event == VK_EVENT_VERTICAL && g.room == 1 && g.player.x == 191 && g.player.y == 48);
        /* Room 1 flight C (tile 13) passes (184,40): Simon is put back on the stairs
         * at progress 48, six cells above its foot, instead of falling. */
        assert(g.player.on_stairs && g.player.mode == 0 && g.player.stair_dir == 0);
        assert(g.player.stair_anim == 48 && g.player.stair_len == 72);
        /* Holding BAS descends the flight to its foot and leaves the stairs there.
         * The arrival X is 191, one pixel left of the diagonal, so the foot is (239,96). */
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(!g.player.on_stairs && g.player.x == 239 && g.player.y == 96);
    }
    /* Stage 1 room 7: BAS on the tile 3 platform at (168..180, 96) enters the top of
     * the flight, and holding BAS descends it to the foot (144,128), 32 px down-left
     * (research/down-stairs.md, measured by trace). */
    assert(vk_game_debug_room(&g, 7));
    g.player.x = 176; g.player.y = 96; g.player.mode = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.player.on_stairs && g.player.x == 176 && g.player.y == 96 && !g.player.crouched);
    /* Going down a right-up flight Simon looks down-left, that is left (facing 1). */
    assert(g.player.facing == 1);
    {
        int frames;
        for (frames = 0; frames < 200 && g.player.on_stairs; frames++)
            vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(!g.player.on_stairs && g.player.x == 144 && g.player.y == 128);
    }
    assert(vk_game_debug_room(&g, 0) && g.collision_verified);
    assert(g.player.x == 16 && g.player.y == 192);

    /* x 232 is not a door, the old UP door is gone, and facing left in the zone
     * does nothing (the left-facing zone of 8580 is not ported). */
    vk_game_begin(&g);
    g.room = 2;
    g.player.x = 232;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 1) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2);
    g.player.x = 200;
    assert(vk_game_tick(&g, 1, 0, 0) == VK_EVENT_NONE);
    assert(g.area == VK_AREA_COURTYARD && g.room == 2 && g.player.x < 232);
    g.player.x = 218;
    g.player.facing = 1;
    assert(vk_game_tick(&g, 0, 0, 0) == VK_EVENT_NONE && !g.entrance);
    /* Out of the Y band [138, 194) nothing starts either. */
    g.player.facing = 0;
    g.player.y = 120;
    g.player.mode = 0;
    assert(vk_game_tick(&g, 0, 0, 0) != VK_EVENT_ENTRANCE && !g.entrance);
    /* Airborne in the zone: the entrance starts, but the walk waits for the
     * landing (778A returns while C420 != 0). The test maps have no floor. */
    vk_game_begin(&g);
    g.room = 2;
    g.player.x = 216;
    g.player.y = 150;
    g.player.mode = 4;
    assert(vk_game_tick(&g, 0, 0, 0) == VK_EVENT_ENTRANCE);
    vk_game_tick(&g, 0, 0, 0);
    vk_game_tick(&g, 0, 0, 0);
    assert(g.player.x == 216 && g.player.y > 150 && g.entrance_step == 0);
    vk_game_begin(&g);
    assert(!g.entrance);

    /* Enemy contact recoil (6F85), one pass every 2 frames: the hit pose, then the
     * 707D arc at 2 px a pass, landing after 22 steps (44 px, as traced: X 136 -> 92),
     * 4 passes (8 frames) on the floor, controls ignored all along. */
    vk_game_debug_room(&g, 0);
    g.player.x = 120;
    g.player.y = 192;
    assert(vk_grounded(vk_game_collision(&g), 120, 192, 1, 0));
    vk_game_hit(&g, -1, 0);
    assert(vk_game_recoiling(&g));
    vk_game_tick_controls(&g, 1, 1, 0, 0);
    assert(g.player.x == 120 && g.player.y == 192 && g.player.pose == VK_PLAYER_RECOIL_POSE);
    vk_game_tick_controls(&g, 1, 1, 0, 0);
    vk_game_tick_controls(&g, 1, 1, 0, 0);
    assert(g.player.x == 118 && g.player.y == 189);       /* first step, frame 2 */
    vk_game_tick_controls(&g, 1, 1, 0, 0);
    assert(g.player.x == 118 && g.player.y == 189);       /* the frame between two passes */
    {
        int f, top = 189;
        for (f = 0; f < 41; f++)
        {
            vk_game_tick_controls(&g, 1, 1, 0, 0);
            if (g.player.y < top) top = g.player.y;
        }
        assert(top == 176);
    }
    assert(g.player.x == 76 && g.player.y == 192 && g.recoil == VK_RECOIL_FLOOR);
    assert(g.player.pose == VK_PLAYER_RECOIL_POSE + 2 && g.player.facing == 1);
    {
        int f;
        for (f = 0; f < 7; f++) vk_game_tick_controls(&g, 1, 1, 0, 0);
    }
    assert(vk_game_recoiling(&g));
    vk_game_tick_controls(&g, 1, 1, 0, 0);
    assert(!vk_game_recoiling(&g) && g.player.x == 76);
    /* Pushed right: the left-facing hit pose (+10/+15), 16 floor passes at energy 0. */
    vk_game_hit(&g, 1, 1);
    vk_game_tick_controls(&g, 0, 0, 0, 0);
    assert(g.player.pose == VK_PLAYER_RECOIL_POSE + 1);
    {
        int f;
        for (f = 0; f < 44; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.player.x == 120 && g.recoil == VK_RECOIL_FLOOR && g.recoil_count == 16);
    }
    /* No room on that side: no step left of X 16 (6C3A). Nothing on a staircase. */
    vk_game_debug_room(&g, 4);
    g.player.x = 15;
    g.player.y = 192;
    vk_game_hit(&g, -1, 0);
    {
        int f;
        for (f = 0; f < 6; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
    }
    assert(g.player.x == 15);
    /* With a room on that side nothing bounds X, and the exit test 767B also runs in
     * C420 = 5: hall room 1 pushed left from X 20 enters room 0 at X F6 when X < 8,
     * and the recoil goes on there. */
    vk_game_debug_room(&g, 1);
    g.player.x = 20;
    g.player.y = 192;
    g.x_frac = 0;
    vk_game_hit(&g, -1, 0);
    {
        int f;
        VKEvent e = VK_EVENT_NONE;
        for (f = 0; f < 40 && e != VK_EVENT_ROOM; f++) e = vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(e == VK_EVENT_ROOM && g.room == 0 && g.player.x == 0xF6);
        assert(g.recoil == VK_RECOIL_AIR && g.recoil_count == 7);   /* X 20 - 7 x 2 = 6 < 8 */
        for (f = 0; f < 80 && g.recoil != VK_RECOIL_FLOOR; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.recoil == VK_RECOIL_FLOOR && g.player.y == 192 && g.player.x == 0xF6 - 2 * 15);
        for (f = 0; f < 10; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(!vk_game_recoiling(&g));
    }
    /* C431 bit 3 (object 0C): steps of 2.5 px in 8.8 (FD80, 6C4E), 55 px in 22 steps. */
    vk_game_debug_room(&g, 0);
    g.player.x = 120;
    g.player.y = 192;
    g.x_frac = 0;
    g.items = VK_ITEM_FAST_RECOIL;
    vk_game_hit(&g, -1, 0);
    {
        int f;
        for (f = 0; f < 3; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.player.x == 117 && g.x_frac == 0x80);
        vk_game_tick_controls(&g, 0, 0, 0, 0);
        vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.player.x == 115 && g.x_frac == 0);
        for (f = 0; f < 80 && g.recoil != VK_RECOIL_FLOOR; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.recoil == VK_RECOIL_FLOOR && g.player.x == 65);
        for (f = 0; f < 10; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
    }
    g.items = 0;
    /* Stair poses of the ROM (6E84, 6F24): climbing right, 04/01 for C42B 0..3, then
     * 05/02 for 4..7; still between steps; a hit gives 04/02 (6FB4, C42F := 2). */
    vk_game_debug_room(&g, 1);
    g.player.x = 48;
    g.player.y = 192;
    g.player.mode = 0;
    g.stair_steps = 0;
    vk_game_tick_controls(&g, 0, 0, 1, 0);
    assert(g.player.on_stairs);
    {
        int f, steps = 0;
        VKByte y = g.player.y;
        for (f = 0; f < 40 && steps < 8; f++)
        {
            vk_game_tick_controls(&g, 0, 0, 1, 0);
            if (g.player.y == y) continue;
            y = g.player.y;
            assert(g.player.pose == VK_PLAYER_STAIR_POSE + (steps < 4 ? 0 : 1));
            steps++;
        }
        assert(steps == 8 && g.stair_steps == 8);
        for (f = 0; f < 6; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.player.pose == VK_PLAYER_STAIR_POSE + 1 && g.player.y == y);
        /* Down from C42B 8: 04/01, then 03/02 from C42B 12. */
        for (f = 0; f < 40 && g.stair_steps < 13; f++) vk_game_tick_controls(&g, 0, 0, 0, 1);
        assert(g.player.pose == VK_PLAYER_STAIR_POSE + 4 + 2 && g.player.facing == 1);
        for (f = 0; f < 40 && g.stair_steps < 17; f++) vk_game_tick_controls(&g, 0, 0, 1, 0);
        assert(g.player.pose == VK_PLAYER_STAIR_POSE);
        vk_game_hit(&g, -1, 0);
        assert(g.player.pose == VK_PLAYER_STAIR_POSE + 3);
        for (f = 0; f < 4; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
    }
    /* Hit on a staircase with energy left (6FA2-6FB9, traced 5 times in hall room 1):
     * no recoil, C420 = 5 for one pass (2 frames) then 3 again, Simon keeps his step
     * and the climb resumes. Hall room 1, flight A from (48,192). */
    vk_game_debug_room(&g, 1);
    g.player.x = 48;
    g.player.y = 192;
    g.player.mode = 0;
    assert(vk_game_tick_controls(&g, 0, 0, 1, 0) == VK_EVENT_NONE && g.player.on_stairs);
    {
        int f;
        VKByte x, y;
        for (f = 0; f < 16; f++) vk_game_tick_controls(&g, 0, 0, 1, 0);
        x = g.player.x;
        y = g.player.y;
        assert(g.player.on_stairs && x > 48 && x - 48 == 192 - y);
        vk_game_hit(&g, -1, 0);
        assert(vk_game_recoiling(&g) && g.recoil == VK_RECOIL_STAIRS);
        vk_game_tick_controls(&g, 0, 1, 1, 0);
        vk_game_tick_controls(&g, 0, 1, 1, 0);
        assert(!vk_game_recoiling(&g) && g.player.on_stairs && g.player.mode == 0);
        assert(g.player.x == x && g.player.y == y);
        for (f = 0; f < 4; f++) vk_game_tick_controls(&g, 0, 0, 1, 0);
        assert(g.player.on_stairs && g.player.x == x + 2 && g.player.y == y - 2);
        /* With no energy left (6FAD -> 6FBC) he is knocked off the staircase, as on
         * the ground (traced: from (64,176) down to the floor of Y 192 at X 8). */
        for (f = 0; f < 64 && g.player.x < 64; f++) vk_game_tick_controls(&g, 0, 0, 1, 0);
        assert(g.player.on_stairs && g.player.x == 64 && g.player.y == 176);
        vk_game_hit(&g, -1, 1);
        assert(vk_game_recoiling(&g) && g.recoil == VK_RECOIL_START && !g.player.on_stairs);
        for (f = 0; f < 160 && g.recoil != VK_RECOIL_FLOOR; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        assert(g.recoil == VK_RECOIL_FLOOR && g.player.y == 192 && g.recoil_count == 16);
        assert(g.player.x == 8 && g.room == 1);
        for (f = 0; f < 32; f++) vk_game_tick_controls(&g, 0, 0, 0, 0);
        /* 7053-707A: then C420 = 6, the death pose (facing back towards the hit,
         * 704B), held until the respawn. */
        assert(g.recoil == VK_RECOIL_DEAD && !g.player.on_stairs && g.player.facing == 0);
        assert(g.player.pose == VK_PLAYER_DEATH_POSE && g.player.x == 16);
        for (f = 0; f < 20; f++) vk_game_tick_controls(&g, 1, 1, 0, 0);
        assert(g.recoil == VK_RECOIL_DEAD && g.player.pose == VK_PLAYER_DEATH_POSE && g.player.x == 16);
    }

    /* Effect 07 (6F7E): a fall begun on the ground ends with VK_EVENT_LAND; a
     * jump landing at its take-off height (C428 = 19, CFF0 = 0) lands without it. */
    vk_game_debug_room(&g, 0);
    g.player.x = 120;
    g.player.y = 150;
    {
        int f, lands = 0;
        for (f = 0; f < 120; f++)
            if (vk_game_tick_controls(&g, 0, 0, 0, 0) == VK_EVENT_LAND) lands++;
        assert(lands == 1 && g.player.y == 192 && g.player.mode == 0);
        assert(vk_game_tick_controls(&g, 0, 1, 0, 0) == VK_EVENT_NONE && g.player.mode == 1);
        for (f = 0; f < 120; f++)
            if (vk_game_tick_controls(&g, 0, 0, 0, 0) == VK_EVENT_LAND) lands++;
        assert(lands == 1 && g.player.y == 192 && g.player.mode == 0);
    }
    /* A jump landing lower than its take-off (traced with Simon lifted by 4, 12, 24
     * and 40 px at the take-off: 07 at C428 = 21, 23, 26, none at 19 or 20). */
    {
        static const struct { VKByte lift; int lands; } kLift[4] = {
            { 4, 0 }, { 12, 1 }, { 24, 1 }, { 40, 1 } };
        int n;
        for (n = 0; n < 4; n++)
        {
            int f, lands = 0;
            g.player.x = 120;
            g.player.y = 192;
            g.player.mode = 0;
            assert(vk_game_tick_controls(&g, 0, 1, 0, 0) == VK_EVENT_NONE && g.player.mode == 1);
            g.player.y = (VKByte)(g.player.y - kLift[n].lift);
            for (f = 0; f < 120; f++)
                if (vk_game_tick_controls(&g, 0, 0, 0, 0) == VK_EVENT_LAND) lands++;
            assert(lands == kLift[n].lands && g.player.y == 192 && g.player.mode == 0);
            assert(g.jump_step == 0);
        }
    }

    /* Stage 2 after the door of hall room 7: room 0 at x 16, y 96, on a floor of
     * its own map; the six rooms and the ROM links (U2 D4 R1 for room 0). */
    vk_game_begin(&g);
    vk_game_enter_stage(&g, 2);
    assert(g.area == VK_AREA_INTERIOR && g.stage == 2 && g.room == 0);
    assert(g.player.x == 16 && g.player.y == 96);
    assert(vk_grounded(vk_game_collision(&g), g.player.x, g.player.y, 2, 0));
    assert(vk_stage_room_count(2) == 6);
    assert(vk_stage_links(2)[0].up == 2 && vk_stage_links(2)[0].down == 4 &&
           vk_stage_links(2)[0].left == VK_ROOM_NONE && vk_stage_links(2)[0].right == 1);
    assert(vk_game_debug_room(&g, 5) && g.stage == 2 && !vk_game_debug_room(&g, 6));
    g.player.x = 248;
    g.room = 0;
    assert(vk_room_try_edge(g.stage, &g.room, &g.player, 1) && g.room == 1);
    vk_game_respawn(&g);
    assert(g.stage == 2 && g.room == 0 && g.player.x == 16 && g.player.y == 96);
    return 0;
}
