#include "gameplay.h"
/* Collision tables exported by the intro resource generator. */
extern const VKByte vk_courtyard_maps[3][768];
extern const VKByte vk_courtyard_neighbors[3][4];
#include "generated/player_assets.h"
#include "generated/stage1_collision.h"
#include "room_manager.h"

/* Interior scene 6 is stage 1 / room 0 (extract_intro.py).
 * extract_player.py decodes the same original 768-byte tilemap as
 * vk_player_room. This is the actual hall geometry, not a fallback. */
const VKByte *vk_game_collision(const VKGame *game)
{
    if (game->area == VK_AREA_COURTYARD && game->room < 3)
        return vk_courtyard_maps[game->room];
    if (game->area == VK_AREA_INTERIOR && game->room < 8)
        return vk_stage1_collision[game->room];
    return vk_player_room; /* Fallback for invalid game state. */
}

void vk_game_begin(VKGame *game)
{
    vk_player_reset(&game->player);
    game->area = VK_AREA_COURTYARD;
    game->room = 0;
    game->collision_verified = 1;
    game->player.y = 176;
}

/* Feet height of the lowest floor under x in a stage-1 room map, or 0 when the
 * column has none. A floor is a solid row with an empty row above it. */
static VKByte vk_floor_arrival(const VKByte *map, VKByte x)
{
    int row;
    for (row = 23; row >= 2; row--)
    {
        const VKByte feet = (VKByte)(16 + 8 * row);
        if (vk_tile_solid(vk_sample_tile(map, x, feet), 1, 0) &&
            !vk_tile_solid(vk_sample_tile(map, x, (VKByte)(feet - 8)), 1, 0))
            return feet;
    }
    return 0;
}

VKEvent vk_game_tick_controls(VKGame *game, int direction, int jump, int up, int down)
{
    VKPlayer *p = &game->player;
    /* Courtyard room 2 -> hall room 0: walking RIGHT at x=232 (openMSX run 6).
     * UP at x=192..216 does not change room. */
    if (game->area == VK_AREA_COURTYARD && game->room == 2 &&
        direction > 0 && p->x >= 232)
    {
        game->area = VK_AREA_INTERIOR;
        game->room = 0;
        game->collision_verified = 1;
        vk_player_reset(p);
        p->x = 232;
        p->y = 176;
        return VK_EVENT_ENTER;
    }

    vk_player_tick_controls(p, vk_game_collision(game),
                            game->area == VK_AREA_COURTYARD ? 0 : 1,
                            direction, jump, down, up);

    if (game->area == VK_AREA_COURTYARD && direction != 0 &&
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
     * Simon to the room below, at its top. Otherwise he is restarted as before. */
    if (p->y >= 225)
    {
        const VKByte target = game->area == VK_AREA_INTERIOR ?
            vk_stage1_links[game->room].down : VK_ROOM_NONE;
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
        vk_player_reset(p);
        if (game->area == VK_AREA_COURTYARD) p->y = 176;
        return VK_EVENT_NONE;
    }
    /* MSX up exit: on a flight with HAUT at Y < 48 (research/vertical-links.md).
     * Simon lands at X +16 (-16 on a right-up flight), Y 224, and glides back up
     * the 32 px diagonal to the floor of the room above. */
    if (game->area == VK_AREA_INTERIOR && p->on_stairs && p->y < 48 &&
        vk_stage1_links[game->room].up != VK_ROOM_NONE)
    {
        const int dir = p->stair_dir;
        game->room = vk_stage1_links[game->room].up;
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
    if (game->area == VK_AREA_INTERIOR && p->y < 32)
    {
        const VKByte target = vk_stage1_links[game->room].up;
        if (target == VK_ROOM_NONE)
        {
            p->y = 32;
            return VK_EVENT_NONE;
        }
        game->room = target;
        p->y = vk_floor_arrival(vk_stage1_collision[target], p->x);
        p->on_stairs = 0;
        p->stair_anim = 0;
        p->mode = p->y ? 0 : 4;
        p->phase = 0;
        if (!p->y) p->y = 200;
        return VK_EVENT_VERTICAL;
    }
    return VK_EVENT_NONE;
}

VKEvent vk_game_tick(VKGame *game, int direction, int jump, int up)
{
    return vk_game_tick_controls(game, direction, jump, up, 0);
}

int vk_game_debug_room(VKGame *game, VKByte room)
{
    if (room >= 8) return 0;
    game->area = VK_AREA_INTERIOR;
    game->room = room;
    game->collision_verified = (room == 0); /* hidden rows 1-7 unverified */
    vk_player_reset(&game->player);
    return 1;
}
