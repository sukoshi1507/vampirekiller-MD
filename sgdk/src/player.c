#include "player.h"
#include "generated/player_assets.h"

/* Original 7D2F. Subtraction wraps to 8 bits before row calculation. */
VKByte vk_sample_tile(const VKByte *map, VKByte x, VKByte y)
{
    const VKWord index = ((VKWord)((VKByte)(y - 16) & 248) << 2) + (x >> 3);
    return index < 768 ? map[index] : 0;
}

/* Original 7C5E: carry is set for (tile - 1) < the stage-specific limit. */
int vk_tile_solid(VKByte tile, VKByte stage, VKByte special)
{
    VKByte limit = special ? 6 : vk_collision_limits[stage < 19 ? stage : 0];
    return (VKByte)(tile - 1) < limit;
}

/* Original 7B88: test two points underneath the player, except at y >= D0. */
int vk_grounded(const VKByte *map, VKByte x, VKByte y, VKByte stage, VKByte special)
{
    if (y >= 208) return 0;
    return vk_tile_solid(vk_sample_tile(map, (VKByte)(x + 5), y), stage, special) ||
           vk_tile_solid(vk_sample_tile(map, (VKByte)(x - 5), y), stage, special);
}

/* Original 7BA9/7C05 normal and crouched side probes. */
int vk_wall(const VKByte *map, VKByte x, VKByte y, int direction,
            VKByte crouched, VKByte stage, VKByte special)
{
    VKByte row;
    const VKByte count = crouched ? 3 : 4;
    const VKByte side = (VKByte)(x + (direction < 0 ? -8 : 8));
    for (row = 0; row < count; row++)
        if (vk_tile_solid(vk_sample_tile(map, side, (VKByte)(y - 2 - row * 8)), stage, special))
            return 1;
    return 0;
}

/* Original 6D4D with BC=6D95,D=19. Active jump phases never approach wraparound. */
void vk_jump_step(VKByte *y, VKByte *phase)
{
    VKByte sample;
    (*phase)++;
    sample = *phase > 19 ? 19 : *phase;
    if (sample == 0) sample = 19;
    *y = (VKByte)(*y + vk_jump_deltas[sample - 1]);
}

void vk_player_reset(VKPlayer *p)
{
    p->x = 16;
    p->y = 192;
    p->facing = p->mode = p->phase = p->clock = p->pose = 0;
    p->crouched = p->on_stairs = p->stair_anim = 0;
}

/* Gameplay controller: the original tile probes and delta tables above remain
 * unchanged for differential C/Z80 verification. Vertical movement is swept
 * one pixel at a time to prevent tunnelling through thin platforms/ceilings.
 * mode: 0 grounded, 1 jump, 4 falling. Other original states are not ported. */
static int vk_ceiling(const VKByte *map, VKByte x, VKByte feet,
                      VKByte stage)
{
    if (feet < 31) return 1; /* HUD / top-of-playfield boundary. */
    return vk_tile_solid(vk_sample_tile(map, (VKByte)(x - 5), (VKByte)(feet - 30)), stage, 0) ||
           vk_tile_solid(vk_sample_tile(map, (VKByte)(x + 5), (VKByte)(feet - 30)), stage, 0);
}

/* Apply signed pixel displacement, stopping at the first crossed surface.
 * Return -1 for a ceiling, +1 for a landing, 0 for unobstructed motion. */
static int vk_move_vertical(VKPlayer *p, const VKByte *map, VKByte stage,
                            int delta)
{
    int step = delta < 0 ? -1 : 1;
    int count = delta < 0 ? -delta : delta;
    while (count-- > 0)
    {
        int next = (int)p->y + step;
        if (next < 32) return -1;
        if (next >= 208) { p->y = 208; return 0; }
        if (step < 0 && vk_ceiling(map, p->x, (VKByte)next, stage)) return -1;
        if (step > 0 && vk_grounded(map, p->x, (VKByte)next, stage, 0))
        {
            /* Ground probe is defined at the feet coordinate. */
            p->y = (VKByte)next;
            return 1;
        }
        p->y = (VKByte)next;
    }
    return 0;
}

static int vk_stair_here(const VKStair *stairs, unsigned count, VKByte x, VKByte y)
{
    unsigned i;
    if (!stairs) return 0;
    for (i = 0; i < count; i++)
        if (x >= stairs[i].x_min && x <= stairs[i].x_max &&
            y >= stairs[i].y_min && y <= stairs[i].y_max) return 1;
    return 0;
}

void vk_player_tick_controls(VKPlayer *p, const VKByte *map, VKByte stage,
                             int direction, int jump_pressed, int down, int up,
                             const VKStair *stairs, unsigned stair_count)
{
    int climb = (up ? -1 : 0) + (down ? 1 : 0);
    int stair_near = vk_stair_here(stairs, stair_count, p->x, p->y);
    /* Stair mode is opt-in and only works inside explicitly authored areas.
     * Do not interpret generic solid tiles as stairs. */
    if (p->on_stairs && !stair_near) p->on_stairs = 0;
    if (!p->on_stairs && climb && stair_near && !jump_pressed)
    { p->on_stairs = 1; p->mode = 0; p->phase = 0; p->stair_anim = 0; }
    if (p->on_stairs)
    {
        if (jump_pressed) { p->on_stairs = 0; p->mode = 1; p->phase = 0; }
        else
        {
            int next_y = (int)p->y + climb;
            int next_x = (int)p->x - climb; /* demo incline rises to the right */
            p->crouched = 0;
            if (climb && next_y >= 32 && next_y < 208 &&
                next_x >= 8 && next_x <= 248 &&
                vk_stair_here(stairs, stair_count, (VKByte)next_x, (VKByte)next_y))
            {
                p->x = (VKByte)next_x;
                p->y = (VKByte)next_y;
                p->clock++;
                p->stair_anim++; /* advance only when feet actually move */
            }
            /* At either staircase endpoint hold position rather than dropping
             * into gravity on a visually unsupported tile. Jump dismounts. */
            /* Match direction of travel: up is right, down is left.
             * Original ROM walk frames, not a synthetic jump silhouette. */
            if (climb) p->facing = (VKByte)(climb > 0);
            /* Four-step walk cycle at eight travelled pixels per keyframe.
             * Use a dedicated progress counter: clock-based animation used to
             * flicker between mismatched frames, including at stair endpoints.
             * The current assets are still walk poses, not recovered MSX2
             * stair poses. The fixed sprite anchor stays unchanged. */
            {
                static const VKByte stair_frames[4] = {0, 1, 2, 1};
                VKByte frame = climb ? stair_frames[(p->stair_anim >> 3) & 3] : 0;
                p->pose = (VKByte)(frame + (p->facing ? 3 : 0));
            }
            return;
        }
    }
    p->crouched = (VKByte)(down && p->mode == 0 && !up);
    if (p->crouched) direction = 0;

    VKByte walking = 0;
    int vertical = 0;
    int hit;
    int n;
    p->clock++;
    if (direction)
    {
        const int dx = direction < 0 ? -1 : 1;
        p->facing = dx < 0;
        for (n = 0; n < 2; n++)
        {
            int next = (int)p->x + dx;
            if (next < 8 || next > 248 ||
                vk_wall(map, (VKByte)next, p->y, dx, p->crouched, stage, 0)) break;
            p->x = (VKByte)next;
            walking = 1;
        }
    }
    if (p->mode == 0)
    {
        if (!vk_grounded(map, p->x, p->y, stage, 0))
        { p->mode = 4; p->phase = 0; }
        else if (jump_pressed && !p->crouched)
        { p->mode = 1; p->phase = 0; }
    }
    else if (p->mode == 1)
    {
        VKByte next_phase = (VKByte)(p->phase + 1);
        if (next_phase > 19) next_phase = 19;
        p->phase = next_phase;
        vertical = vk_jump_deltas[next_phase - 1];
        hit = vk_move_vertical(p, map, stage, vertical);
        if (hit == -1 || hit == 1 || p->phase >= 19)
        {
            if (hit == 1) { p->mode = 0; p->phase = 0; }
            else { p->mode = 4; p->phase = 0; }
        }
    }
    else if (p->mode == 4)
    {
        /* Check before falling, so standing on an existing floor is stable. */
        if (vk_grounded(map, p->x, p->y, stage, 0))
        { p->mode = 0; p->phase = 0; }
        else
        {
            vertical = vk_fall_deltas[p->phase < 4 ? p->phase : 3];
            hit = vk_move_vertical(p, map, stage, vertical);
            if (hit == 1) { p->mode = 0; p->phase = 0; }
            else if (p->phase < 3) p->phase++;
        }
    }
    if (p->y >= 208) { vk_player_reset(p); if (stage == 0) p->y = 176; }
    if (p->mode != 0) p->crouched = 0;
    if (p->mode == 1 || p->mode == 4) p->pose = p->facing ? 7 : 6;
    else
    {
        VKByte frame = walking ? ((p->clock & 4) ? 1 : ((p->clock & 8) ? 0 : 2)) : 0;
        p->pose = frame + (p->facing ? 3 : 0);
    }
}

void vk_player_tick_map(VKPlayer *p, const VKByte *map, VKByte stage,
                        int direction, int jump_pressed)
{
    vk_player_tick_controls(p, map, stage, direction, jump_pressed,
                            0, 0, 0, 0);
}

void vk_player_tick(VKPlayer *p, int direction, int jump_pressed)
{
    vk_player_tick_map(p, vk_player_room, 1, direction, jump_pressed);
}
