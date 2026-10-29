#include "gameplay.h"
/* Collision tables exported by the intro resource generator. */
extern const VKByte vk_courtyard_maps[3][768];
extern const VKByte vk_courtyard_neighbors[3][4];
#include "generated/player_assets.h"
#include "generated/stage1_collision.h"
#include "generated/stage2_maps.h"
#include "room_manager.h"

/* Collision map of a castle room (stage 1 hall or stage 2), NULL if none. */
static const VKByte *vk_castle_map(VKByte stage, VKByte room)
{
    if (stage == 1 && room < VK_STAGE1_ROOM_COUNT) return vk_stage1_collision[room];
    if (stage == 2 && room < VK_STAGE2_ROOM_COUNT) return vk_stage2_collision[room];
    return 0;
}

/* Castle room map in RAM, for the hidden blocks that candles.c writes in it
 * (86CA) and takes out again (8899). Used only while it belongs to the current
 * room; otherwise the ROM map is read directly. */
static VKByte room_map[768];
static VKByte room_map_stage = 0xFF, room_map_room = 0xFF;

static int room_map_current(const VKGame *game)
{
    return game->area == VK_AREA_INTERIOR && room_map_stage == game->stage &&
           room_map_room == game->room;
}

VKByte *vk_game_room_map(const VKGame *game)
{
    const VKByte *map = game->area == VK_AREA_INTERIOR ? vk_castle_map(game->stage, game->room) : 0;
    int i;
    if (!map) return 0;
    for (i = 0; i < 768; i++) room_map[i] = map[i];
    room_map_stage = game->stage;
    room_map_room = game->room;
    return room_map;
}

void vk_game_room_block(const VKGame *game, VKByte x, VKByte y, const VKByte *tiles)
{
    const VKByte *rom = vk_castle_map(game->stage, game->room);
    int row, col;
    if (!rom || !room_map_current(game)) return;
    for (row = 0; row < 4; row++)
        for (col = 0; col < 4; col++)
        {
            /* 7D2F: map row (Y - 16) / 8, column X / 8. */
            const int index = (((y - 16) >> 3) + row) * 32 + (x >> 3) + col;
            if (index < 0 || index >= 768) continue;
            room_map[index] = tiles ? tiles[row * 4 + col] : rom[index];
        }
}

/* Interior scene 6 is stage 1 / room 0 (extract_intro.py).
 * extract_player.py decodes the same original 768-byte tilemap as
 * vk_player_room. This is the actual hall geometry, not a fallback. */
const VKByte *vk_game_collision(const VKGame *game)
{
    if (game->area == VK_AREA_COURTYARD && game->room < 3)
        return vk_courtyard_maps[game->room];
    if (game->area == VK_AREA_INTERIOR)
    {
        const VKByte *map = vk_castle_map(game->stage, game->room);
        if (map) return room_map_current(game) ? room_map : map;
    }
    return vk_player_room; /* Fallback for invalid game state. */
}

/* Index of vk_collision_limits: 0 in the courtyard, the stage number inside. */
static VKByte vk_game_limit_stage(const VKGame *game)
{
    return game->area == VK_AREA_COURTYARD ? 0 : game->stage;
}

static void vk_entrance_clear(VKGame *game)
{
    game->entrance = VK_ENTRANCE_NONE;
    game->entrance_step = 0;
    game->entrance_timer = 0;
    game->recoil = VK_RECOIL_NONE;
    game->ground_fall = 0;
    game->jump_step = 0;
}

void vk_game_begin(VKGame *game)
{
    vk_player_reset(&game->player);
    game->area = VK_AREA_COURTYARD;
    game->room = 0;
    game->stage = 1;
    game->collision_verified = 1;
    game->player.y = 176;
    vk_entrance_clear(game);
}

/* Castle entrance, courtyard room 2 (research/castle-entrance.md, openMSX runs A
 * and B). Door zone of routine 8580 with C5AD = 144, C5AE = 224: facing right,
 * X in [216, 224) and Y in [138, 194). Then C5AC = 5 (7755): input is ignored and
 * routine 778A walks Simon right by half a pixel per 30 Hz tick while he is on
 * the ground (one pixel per four frames, X 216 -> 232 in 64 frames), with the
 * walk poses at their usual cadence. Sprites 0..7 (55EC) copy the door jamb at
 * X 224..239, Y 144..175 in front of Simon. The exit zone is X in [232, 240)
 * (77C4). Three frames later the display goes off for 36 frames; Simon is put in
 * hall room 0 at x 16, y 192 meanwhile, and play resumes when it comes back. */
#define ENTRANCE_ROOM 2
#define ENTRANCE_X_MIN 216
#define ENTRANCE_X_END 224
#define ENTRANCE_Y_MIN 138
#define ENTRANCE_Y_END 194
#define ENTRANCE_EXIT_X 232
#define ENTRANCE_DARK_T 3      /* frames from the exit to the display off */
#define ENTRANCE_HALL_T 39     /* frames from the exit to the hall on screen */

int vk_game_entrance_locked(const VKGame *game)
{
    return game->entrance != VK_ENTRANCE_NONE;
}

static int vk_entrance_zone(const VKGame *game)
{
    const VKPlayer *p = &game->player;
    /* PROVISIONAL: the facing-left zone of 8580 (X in [232, 240)) is read in the
     * code only; it is not ported (no traced way to stand there facing left). */
    return game->area == VK_AREA_COURTYARD && game->room == ENTRANCE_ROOM &&
           !p->facing && !p->on_stairs &&
           p->x >= ENTRANCE_X_MIN && p->x < ENTRANCE_X_END &&
           p->y >= ENTRANCE_Y_MIN && p->y < ENTRANCE_Y_END;
}

static VKEvent vk_entrance_tick(VKGame *game)
{
    VKPlayer *p = &game->player;
    if (game->entrance == VK_ENTRANCE_WALK)
    {
        if (p->mode != 0)
        {
            /* 778A returns while C420 != 0: no walk until Simon lands. */
            vk_player_tick_controls(p, vk_game_collision(game), 0, 0, 0, 0, 0);
            return VK_EVENT_NONE;
        }
        {
            /* MSX2 walk cycle (player.c): poses 0,1,2,1 held 8 frames each. */
            static const VKByte walk_frames[4] = {0, 1, 2, 1};
            p->clock++;
            p->pose = walk_frames[(p->clock >> 3) & 3];
        }
        game->entrance_step++;
        if ((game->entrance_step & 3) == 0) p->x++;
        if (p->x >= ENTRANCE_EXIT_X)
        {
            game->entrance = VK_ENTRANCE_EXIT;
            game->entrance_timer = 0;
        }
        return VK_EVENT_NONE;
    }
    game->entrance_timer++;
    if (game->entrance == VK_ENTRANCE_EXIT && game->entrance_timer >= ENTRANCE_DARK_T)
    {
        /* 4347: D000 0 -> 1, D001 = 0; Simon placed while the screen is off. */
        game->entrance = VK_ENTRANCE_DARK;
        game->area = VK_AREA_INTERIOR;
        game->room = 0;
        game->collision_verified = 1;
        vk_player_reset(p);
        p->x = 16;
        p->y = 192;   /* hall floor (row 22) */
        return VK_EVENT_ENTRANCE_DARK;
    }
    if (game->entrance == VK_ENTRANCE_DARK && game->entrance_timer >= ENTRANCE_HALL_T)
    {
        vk_entrance_clear(game);
        return VK_EVENT_ENTER;
    }
    return VK_EVENT_NONE;
}

void vk_game_enter_stage(VKGame *game, VKByte stage)
{
    vk_entrance_clear(game);
    game->area = VK_AREA_INTERIOR;
    game->stage = stage;
    game->room = 0;
    game->collision_verified = 0;   /* hidden rows zero-filled, as hall rooms 1-7 */
    vk_player_reset(&game->player);
    game->player.x = 16;
    game->player.y = 96;
}

/* Feet height of the lowest floor under x in a castle room map, or 0 when the
 * column has none. A floor is a solid row with an empty row above it. */
static VKByte vk_floor_arrival(const VKByte *map, VKByte x, VKByte stage)
{
    int row;
    for (row = 23; row >= 2; row--)
    {
        const VKByte feet = (VKByte)(16 + 8 * row);
        if (vk_tile_solid(vk_sample_tile(map, x, feet), stage, 0) &&
            !vk_tile_solid(vk_sample_tile(map, x, (VKByte)(feet - 8)), stage, 0))
            return feet;
    }
    return 0;
}

/* Y steps of the recoil arc (707D): 16 px up, then down, 3 px a frame at the end. */
static const signed char vk_recoil_arc[VK_RECOIL_STEPS] = {
    -3, -3, -2, -2, -2, -1, -1, -1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3
};

void vk_game_hit(VKGame *game, int dir, int dead)
{
    VKPlayer *p = &game->player;
    /* 6FA2: on a staircase (C42B != 0) with energy left, C420 := 3 again at the next
     * pass, without moving (traced 5 times in hall room 1). The C42F := 2 pose of
     * 6FB4 is not ported: the port's stair poses are not the MSX ones. */
    if (p->on_stairs && !dead)
    {
        game->recoil = VK_RECOIL_STAIRS;
        game->recoil_count = VK_RECOIL_STAIRS_FRAMES;
        return;
    }
    /* 6FAD: with C415 = 0 the hit knocks Simon off the staircase (traced: arc from
     * the step at (64, 176) down to the floor). */
    p->on_stairs = 0;
    p->stair_anim = 0;
    p->stair_glide = 0;
    game->jump_step = 0;
    game->recoil = VK_RECOIL_START;
    game->recoil_dir = (signed char)(dir < 0 ? -1 : 1);
    game->recoil_land = (VKByte)(dead ? 16 : 4);
    game->recoil_count = 0;
}

int vk_game_recoiling(const VKGame *game)
{
    return game->recoil != VK_RECOIL_NONE;
}

/* 6C2F / 6C53: 2 px towards the push, unless a wall (7C05 / 7BA9) or, without
 * a room on that side, the screen side (X < 16 / X >= F0) stops it. The 2.5 px
 * step of C431 bit 3 is not ported. PROVISIONAL: with a room on that side the
 * port stops at its walk limits (8, 248) instead of changing room. */
static void vk_recoil_move(VKGame *game, const VKRoomLink *links)
{
    VKPlayer *p = &game->player;
    const int dir = game->recoil_dir;
    int open;
    int next;
    if (game->area == VK_AREA_COURTYARD)
        open = vk_courtyard_neighbors[game->room][dir > 0 ? 3 : 2] < 3;
    else
        open = links && (dir > 0 ? links[game->room].right : links[game->room].left) != VK_ROOM_NONE;
    p->facing = (VKByte)(dir < 0);
    if (!open && (dir < 0 ? p->x < 16 : p->x >= 0xF0)) return;
    if (vk_wall(vk_game_collision(game), p->x, p->y, dir, 0, vk_game_limit_stage(game), 0)) return;
    next = (int)p->x + 2 * dir;
    if (next < 8) next = 8;
    if (next > 248) next = 248;
    p->x = (VKByte)next;
}

/* One frame of C420 = 5 (6F85): the controls are ignored. */
static void vk_recoil_tick(VKGame *game, const VKRoomLink *links)
{
    VKPlayer *p = &game->player;
    /* Hit pose 07/03, +10/+15 when C43C = 1 (pushed right: Simon faces left). */
    const VKByte pose = (VKByte)(VK_PLAYER_RECOIL_POSE + (game->recoil_dir > 0));
    if (game->recoil == VK_RECOIL_STAIRS)
    {
        /* The pass of the hit: no stair step, no control. */
        if (--game->recoil_count == 0) game->recoil = VK_RECOIL_NONE;
        return;
    }
    if (game->recoil == VK_RECOIL_START)
    {
        p->mode = 0;
        p->phase = 0;
        p->crouched = 0;
        p->pose = pose;
        game->recoil = VK_RECOIL_AIR;
        return;
    }
    if (game->recoil == VK_RECOIL_AIR)
    {
        VKByte step;
        vk_recoil_move(game, links);
        step = ++game->recoil_count;
        if (step > VK_RECOIL_STEPS) step = VK_RECOIL_STEPS;
        p->y = (VKByte)(p->y + vk_recoil_arc[step - 1]);
        p->pose = pose;
        if (game->recoil_count < VK_RECOIL_LAND_TEST ||
            !vk_grounded(vk_game_collision(game), p->x, p->y, vk_game_limit_stage(game), 0))
            return;
        /* 6FF2: on the floor, Y to the tile row, landed pose (+1, +1). */
        p->y &= 0xF8;
        game->recoil = VK_RECOIL_FLOOR;
        game->recoil_count = game->recoil_land;
        p->pose = (VKByte)(pose + 2);
        return;
    }
    /* 7024: then 702A gives the controls back. */
    if (--game->recoil_count) return;
    game->recoil = VK_RECOIL_NONE;
    p->pose = (VKByte)(p->facing ? 3 : 0);
}

VKEvent vk_game_tick_controls(VKGame *game, int direction, int jump, int up, int down)
{
    VKPlayer *p = &game->player;
    const VKRoomLink *links = game->area == VK_AREA_INTERIOR ? vk_stage_links(game->stage) : 0;
    const int recoiling = game->recoil != VK_RECOIL_NONE;
    int landed = 0;
    /* Courtyard room 2 -> hall room 0 through the castle door (see above). UP at
     * x=192..216 does not change room. */
    if (game->entrance) return vk_entrance_tick(game);
    /* After a fall Simon stays below the screen until the respawn (C420 = 6:
     * 767B and the player routines return at once). */
    if (p->y == VK_FALL_Y) return VK_EVENT_NONE;
    if (recoiling) vk_recoil_tick(game, links);
    else if (vk_entrance_zone(game))
    {
        game->entrance = VK_ENTRANCE_WALK;
        game->entrance_step = 0;
        game->entrance_timer = 0;
        p->crouched = 0;
        return VK_EVENT_ENTRANCE;
    }

    if (!recoiling)
    {
        const VKByte mode = p->mode;
        const VKByte phase = p->phase;
        vk_player_tick_controls(p, vk_game_collision(game), vk_game_limit_stage(game),
                                direction, jump, down, up);
        /* The MSX falls in state 4 only from the ground (6BA5); its jump lands in
         * state 1 (6D26), with effect 07 only when CFF0 != 0, i.e. at C428 >= 21
         * (6D1C: C428 - 2 >= 13h calls 6D6D, CFF0 := 1; traced: 07 at steps 21, 23
         * and 26, none at step 19). The port leaves its jump table at step 19 for a
         * mode 4 fall; the MSX keeps stepping (+6 a 30 Hz tick), so C428 is counted
         * from the frames of that fall: 19 + ceil(frames / 2). */
        if (mode == 0 && p->mode == 4) { game->ground_fall = 1; game->jump_step = 0; }
        else if (p->mode == 1) game->ground_fall = 0;
        else if (mode == 4 && p->mode == 0 && game->ground_fall)
        {
            game->ground_fall = 0;
            landed = 1;
        }
        if (mode == 1 && p->mode == 4) game->jump_step = (VKByte)(2 * (phase + 1));
        else if (mode == 4 && game->jump_step)
        {
            if (game->jump_step < 254) game->jump_step++;
            if (p->mode == 0)
            {
                if ((game->jump_step + 1) / 2 >= VK_JUMP_THUD_STEP) landed = 1;
                game->jump_step = 0;
            }
        }
    }

    if (!recoiling && game->area == VK_AREA_COURTYARD && direction != 0 &&
        ((direction > 0 && p->x >= 248) || (direction < 0 && p->x <= 8)))
    {
        const VKByte neighbor = vk_courtyard_neighbors[game->room][direction > 0 ? 3 : 2];
        if (neighbor != 255 && neighbor < 3)
        {
            const VKByte airborne = p->mode != 0;
            game->room = neighbor;
            p->x = direction > 0 ? 10 : 246;
            /* Keep height, facing, jump phase, clock and motion across rooms. */
            if (!airborne && !vk_grounded(vk_game_collision(game), p->x, p->y, 0, 0))
            { p->mode = 4; p->phase = 0; }
            return VK_EVENT_ROOM;
        }
    }
    /* Bottom edge (Y 225, measured in openMSX): a hall room with a down link sends
     * Simon to the room below, at its top. Otherwise he falls out of play (76BC):
     * C420 := 6, Y := FA, and health.c counts down to the lost life. */
    if (p->y >= 225)
    {
        const VKByte target = links ? links[game->room].down : VK_ROOM_NONE;
        if (target != VK_ROOM_NONE)
        {
            /* From an arrival glide, the MSX exit shifts X by 16 (207 -> 192). */
            if (p->stair_glide) p->x = (VKByte)(p->stair_dir ? p->x + 16 : p->x - 16);
            p->stair_glide = 0;
            game->room = target;
            p->y = 48;
            p->on_stairs = 0;
            p->stair_anim = 0;
            p->mode = 4;
            p->phase = 0;
            /* A flight of the room below through this point takes Simon on its stairs
             * instead of the fall (player.c). Otherwise he keeps falling. */
            {
                int vk_stair_resume(VKPlayer *player, const VKByte *map);
                vk_stair_resume(p, vk_game_collision(game));
            }
            return VK_EVENT_VERTICAL;
        }
        p->y = VK_FALL_Y;
        p->mode = 0;
        p->on_stairs = 0;
        p->crouched = 0;
        return VK_EVENT_FALL;
    }
    /* MSX up exit: on a flight with HAUT at Y < 48 (research/vertical-links.md).
     * Simon lands at X +16 (-16 on a right-up flight), Y 224, and glides back up
     * the 32 px diagonal to the floor of the room above. */
    if (links && p->on_stairs && p->y < 48 && links[game->room].up != VK_ROOM_NONE)
    {
        const int dir = p->stair_dir;
        game->room = links[game->room].up;
        p->x = (VKByte)(dir ? p->x - 16 : p->x + 16);
        p->y = 224;
        p->stair_len = 32;
        p->stair_anim = 0;
        p->stair_glide = 1;
        p->pose = 0;
        return VK_EVENT_VERTICAL;
    }
    /* Hall vertical link: leaving the top of the playfield (a stair exit at y 24
     * counts) enters the room above, on its lowest floor in the same column. */
    if (links && p->y < 32)
    {
        const VKByte target = links[game->room].up;
        if (target == VK_ROOM_NONE)
        {
            p->y = 32;
            return VK_EVENT_NONE;
        }
        game->room = target;
        p->y = vk_floor_arrival(vk_castle_map(game->stage, target), p->x, game->stage);
        p->on_stairs = 0;
        p->stair_anim = 0;
        p->mode = p->y ? 0 : 4;
        p->phase = 0;
        if (!p->y) p->y = 200;
        return VK_EVENT_VERTICAL;
    }
    return landed ? VK_EVENT_LAND : VK_EVENT_NONE;
}

VKEvent vk_game_tick(VKGame *game, int direction, int jump, int up)
{
    return vk_game_tick_controls(game, direction, jump, up, 0);
}

void vk_game_respawn(VKGame *game)
{
    if (game->area != VK_AREA_INTERIOR)
    {
        vk_game_begin(game);
        return;
    }
    if (game->stage != 1)
    {
        vk_game_enter_stage(game, game->stage);
        return;
    }
    vk_entrance_clear(game);
    game->room = 0;
    game->collision_verified = 1;
    vk_player_reset(&game->player);
    game->player.x = 16;
    game->player.y = 192;
}

int vk_game_debug_room(VKGame *game, VKByte room)
{
    if (game->area != VK_AREA_INTERIOR) game->stage = 1;
    if (room >= vk_stage_room_count(game->stage)) return 0;
    vk_entrance_clear(game);
    game->area = VK_AREA_INTERIOR;
    game->room = room;
    /* Hidden rows of hall rooms 1-7 and of stage 2 are unverified. */
    game->collision_verified = (game->stage == 1 && room == 0);
    vk_player_reset(&game->player);
    return 1;
}
