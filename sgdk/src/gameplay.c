#include "gameplay.h"
/* Collision tables exported by the intro resource generator. */
extern const VKByte vk_courtyard_maps[3][768];
extern const VKByte vk_courtyard_neighbors[3][4];
#include "generated/player_assets.h"
#include "generated/stage1_collision.h"

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

    /* No verified stair volumes are available for these rooms yet.
     * The controller supports volumes, but they must be mapped from ROM data. */
    /* Explicit debug volume: NOT an original MSX2 stair coordinate. */
    static const VKStair sandbox_stairs[] = {{120, 208, 96, 184}};
    const int demo = game->area == VK_AREA_INTERIOR && game->room == 1;
    vk_player_tick_controls(p, vk_game_collision(game),
                            game->area == VK_AREA_COURTYARD ? 0 : 1,
                            direction, jump, down, up,
                            demo ? sandbox_stairs : 0, demo ? 1u : 0u);

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
