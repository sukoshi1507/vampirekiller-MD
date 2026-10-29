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
    const int side_x = (int)x + (direction < 0 ? -8 : 8);
    /* Past the screen edge the probe would wrap to the opposite column. Room
     * edges are crossed by the room manager, so nothing blocks there. */
    if (side_x < 0 || side_x > 255) return 0;
    const VKByte side = (VKByte)side_x;
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
    p->stair_dir = p->stair_len = p->stair_glide = p->stair_move = 0;
    p->jump_frames = 0;
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
        if (next >= 225) { p->y = 225; return 0; }
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

/* Staircases are map tiles (research/stairs.md): tile 12 is a flight climbing
 * to the right, tile 13 a flight climbing to the left. Each step is one 8 px
 * cell, and a flight of N steps is travelled as 8N + 8 px on both axes. */
#define VK_STAIR_RIGHT 12
#define VK_STAIR_LEFT 13
/* Shorter runs of these tiles are decor, not stairs (research/stairs.md). */
#define VK_STAIR_MIN_STEPS 3

static int vk_stair_tile_at(const VKByte *map, int x, int y)
{
    if (x < 0 || x > 255 || y < 16) return 0;
    return vk_sample_tile(map, (VKByte)x, (VKByte)y);
}

/* Mount a flight from its foot with HAUT. The probes follow the original order:
 * tile 13 at x-8 then x, tile 12 at x+8 then x, all on the row above the feet. */
static int vk_stair_enter(VKPlayer *p, const VKByte *map)
{
    const int x = p->x;
    const int y = (int)p->y - 8;
    int cell_x, dir, steps, i;
    if (vk_stair_tile_at(map, x - 8, y) == VK_STAIR_LEFT) { cell_x = (x - 8) & ~7; dir = 0; }
    else if (vk_stair_tile_at(map, x, y) == VK_STAIR_LEFT) { cell_x = x & ~7; dir = 0; }
    else if (vk_stair_tile_at(map, x + 8, y) == VK_STAIR_RIGHT) { cell_x = (x + 8) & ~7; dir = 1; }
    else if (vk_stair_tile_at(map, x, y) == VK_STAIR_RIGHT) { cell_x = x & ~7; dir = 1; }
    else return 0;
    /* Count the steps: each row up moves the cell by 8 px in the flight direction. */
    steps = 0;
    for (i = 0; i < 24; i++)
    {
        const int step_y = (int)(y & ~7) - 8 * i;
        const int step_x = cell_x + (dir ? 8 : -8) * i;
        if (step_y < 16 || step_x < 0 || step_x > 248) break;
        if (vk_stair_tile_at(map, step_x, step_y) != (dir ? VK_STAIR_RIGHT : VK_STAIR_LEFT)) break;
        steps++;
    }
    if (steps < VK_STAIR_MIN_STEPS) return 0;
    /* Original alignment: right-up flights snap X to the cell, left-up flights to cell+8. */
    p->x = (VKByte)(dir ? cell_x : cell_x + 8);
    p->y = (VKByte)((y & ~7) + 8);
    p->stair_dir = (VKByte)dir;
    p->facing = (VKByte)!dir; /* facing 1 looks left, as in the walk code */
    p->stair_len = (VKByte)(8 * steps + 8);
    p->stair_anim = 0;
    p->stair_glide = 0;
    p->stair_move = 1; /* 6BEE: C435 := the key, the first cell is climbed */
    p->on_stairs = 1;
    p->mode = 0;
    p->phase = 0;
    p->crouched = 0;
    return 1;
}

/* Descend a flight from its top with BAS (research/down-stairs.md). The feet stand
 * on a solid tile 3 (down left, measured in room 7 at Y 96) or tile 4 (down right,
 * not yet measured). X is the tile cell + 8 for tile 3, as in the MSX alignment.
 * The flight below is counted in tiles 12 (down left) or 13 (down right). */
static int vk_stair_enter_top(VKPlayer *p, const VKByte *map)
{
    const int x = p->x;
    const int y = p->y;
    int cell_x, dir, steps, i;
    if (vk_stair_tile_at(map, x, y) == 3) { cell_x = x & ~7; dir = 1; }
    else if (vk_stair_tile_at(map, x - 8, y) == 3) { cell_x = (x - 8) & ~7; dir = 1; }
    else if (vk_stair_tile_at(map, x, y) == 4) { cell_x = x & ~7; dir = 0; }
    else if (vk_stair_tile_at(map, x + 8, y) == 4) { cell_x = (x + 8) & ~7; dir = 0; }
    else return 0;
    steps = 0;
    for (i = 0; i < 24; i++)
    {
        const int step_x = cell_x + (dir ? -8 : 8) * (i + 1);
        const int step_y = (y & ~7) + 8 * (i + 1);
        if (step_y > 200 || step_x < 0 || step_x > 248) break;
        if (vk_stair_tile_at(map, step_x, step_y) != (dir ? VK_STAIR_RIGHT : VK_STAIR_LEFT)) break;
        steps++;
    }
    if (steps < 1) return 0;
    p->x = (VKByte)(dir ? cell_x + 8 : cell_x);
    p->y = (VKByte)(y & ~7);
    p->stair_dir = (VKByte)dir;
    p->stair_len = (VKByte)(8 * steps + 8);
    p->stair_anim = p->stair_len; /* the progress starts at the top */
    p->stair_glide = 0;
    p->stair_move = 2; /* 6BEE: C435 := the key (BAS), the first cell is descended */
    p->facing = (VKByte)dir; /* entering from the top, Simon goes down */
    p->on_stairs = 1;
    p->mode = 0;
    p->phase = 0;
    p->crouched = 0;
    return 1;
}

/* Resume a flight at an arrival point that has no stair mode, such as a fall
 * through the bottom edge into the room below. The arrival cell is the tile row
 * above the feet, at the column nearest to X - 8. When it is a tile 13 (down
 * right) or tile 12 (down left) flight, Simon stands on it at its progress: the
 * flight runs from its foot along the diagonal, 8 px per cell. Returns 0 when no
 * flight passes the point, and the caller keeps its fall. */
int vk_stair_resume(VKPlayer *p, const VKByte *map)
{
    const int cy = ((int)p->y - 8) & ~7;
    const int cx = ((int)p->x - 4) & ~7;
    int dir, below, above, i, top;
    VKByte want;
    if (vk_stair_tile_at(map, cx, cy) == VK_STAIR_LEFT) dir = 0;
    else if (vk_stair_tile_at(map, cx, cy) == VK_STAIR_RIGHT) dir = 1;
    else return 0;
    want = (VKByte)(dir ? VK_STAIR_RIGHT : VK_STAIR_LEFT);
    /* Down step is (+8,+8) on a tile 13 flight and (-8,+8) on a tile 12 flight. */
    below = 0;
    for (i = 1; i < 24; i++)
    {
        const int step_y = cy + 8 * i;
        if (step_y > 200) break;
        if (vk_stair_tile_at(map, cx + (dir ? -8 : 8) * i, step_y) != want) break;
        below++;
    }
    above = 0;
    for (i = 1; i < 24; i++)
    {
        const int step_y = cy - 8 * i;
        if (step_y < 16) break;
        if (vk_stair_tile_at(map, cx - (dir ? -8 : 8) * i, step_y) != want) break;
        above++;
    }
    /* The arrival cell is the foot + below; the top cell is at foot + below + above. */
    if (below < 1 || below + above + 1 < VK_STAIR_MIN_STEPS) return 0;
    top = below + above;
    if (top > 29) top = 29; /* keeps stair_len within a byte */
    p->stair_dir = (VKByte)dir;
    p->stair_len = (VKByte)(8 * (top + 1) + 8);
    p->stair_anim = (VKByte)(8 * below);
    p->stair_glide = 0;
    p->stair_move = 0;
    p->on_stairs = 1;
    p->mode = 0;
    p->phase = 0;
    p->crouched = 0;
    p->facing = (VKByte)dir; /* arriving from above, Simon goes down */
    p->pose = (VKByte)(p->facing ? 3 : 0);
    return 1;
}

/* Climb or descend one pixel on each axis every two frames. stair_anim is the
 * progress along the flight: 0 at the foot, stair_len at the top. A step under
 * way runs to the next cell edge whatever the keys (6DE2: C435 is the direction
 * while C42B & 7 != 0; traced, scratchpad/trace3 runs out2 and out3: HAUT held 4
 * frames gives 8 steps). On a cell edge the keys are read in the order HAUT, BAS,
 * GAUCHE, DROITE (6DF6 / 6E07); with no key Simon stands on the step. Reaching
 * either end leaves the staircase. */
static void vk_stair_tick(VKPlayer *p, int direction, int down, int up)
{
    /* Right-up flights climb with HAUT or DROITE, left-up flights with HAUT or GAUCHE. */
    int want_up, want_down;
    const int sx = p->stair_dir ? 1 : -1; /* horizontal sign of a climb */
    int moved = 0;
    static const VKByte stair_frames[4] = {0, 1, 2, 1};
    if (p->stair_move && !p->stair_glide)
    {
        want_up = p->stair_move == 1;
        want_down = !want_up;
    }
    else if (up || down)
    {
        want_up = up;
        want_down = !up;
    }
    else
    {
        want_up = direction && ((direction > 0) == (p->stair_dir != 0));
        want_down = direction && !want_up;
    }
    p->crouched = 0;
    p->clock++;
    if ((p->clock & 1) == 0)
    {
        if (want_up && p->stair_anim < p->stair_len)
        {
            p->x = (VKByte)((int)p->x + sx);
            p->y = (VKByte)(p->y - 1);
            p->stair_anim++;
            moved = 1;
        }
        else if (want_down && p->stair_anim > 0)
        {
            p->x = (VKByte)((int)p->x - sx);
            p->y = (VKByte)(p->y + 1);
            p->stair_anim--;
            moved = 1;
        }
        else if (want_down && p->stair_glide && p->y < 225)
        {
            /* A glide foot at Y 224 drops through the bottom edge (Y 225). */
            p->y++;
            moved = 1;
        }
    }
    /* Simon looks along the direction of travel: up is !dir, down is dir. */
    if (moved) p->facing = (VKByte)(want_up ? !p->stair_dir : p->stair_dir);
    if (moved && !p->stair_glide)
        p->stair_move = (VKByte)((p->stair_anim & 7) ? (want_up ? 1 : 2) : 0);
    if (p->stair_anim == p->stair_len) p->stair_glide = 0;
    if (moved && (p->stair_anim == p->stair_len || p->y >= 225 ||
                  (p->stair_anim == 0 && !p->stair_glide)))
    { p->on_stairs = 0; p->mode = 0; p->phase = 0; }
    /* Four-step walk cycle, one pose per eight pixels of progress. gameplay.c
     * (vk_stair_pose) replaces it with the stair poses of the ROM in play. */
    p->pose = (VKByte)(stair_frames[(p->stair_anim >> 3) & 3] + (p->facing ? 3 : 0));
}

void vk_player_tick_controls(VKPlayer *p, const VKByte *map, VKByte stage,
                             int direction, int jump_pressed, int down, int up)
{
    /* Staircases exist only inside the interior areas (stage 1). */
    if (p->on_stairs && jump_pressed) { p->on_stairs = 0; p->mode = 1; p->phase = 0; p->jump_frames = 0; }
    else if (p->on_stairs) { vk_stair_tick(p, direction, down, up); return; }
    else if (stage && up && !jump_pressed && vk_stair_enter(p, map))
    {
        p->pose = (VKByte)(p->facing ? 3 : 0);
        return;
    }
    else if (stage && down && !up && !jump_pressed && p->mode == 0 &&
             vk_stair_enter_top(p, map))
    {
        p->pose = (VKByte)(p->facing ? 3 : 0);
        return;
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
        /* MSX2 measured in openMSX: C427 advances 1 px per VBLANK (60 px/s). */
        for (n = 0; n < 1; n++)
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
        { p->mode = 1; p->phase = 0; p->jump_frames = 0; }
    }
    else if (p->mode == 1)
    {
        /* The MSX arc advances once per 30 Hz logic tick, so every second frame, with
         * the first step on frame 4 (openMSX, research/jump-compare.md). */
        p->jump_frames++;
        if (p->jump_frames >= 4 && (p->jump_frames & 1) == 0)
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
    if (p->mode != 0) p->crouched = 0;
    if (p->mode == 1 || p->mode == 4) p->pose = p->facing ? 7 : 6;
    else
    {
        /* MSX2 walk cycle measured in openMSX: poses 0,1,2,1 held 8 VBLANKs each. */
        static const VKByte walk_frames[4] = {0, 1, 2, 1};
        VKByte frame = walking ? walk_frames[(p->clock >> 3) & 3] : 0;
        p->pose = frame + (p->facing ? 3 : 0);
    }
}

void vk_player_tick_map(VKPlayer *p, const VKByte *map, VKByte stage,
                        int direction, int jump_pressed)
{
    vk_player_tick_controls(p, map, stage, direction, jump_pressed, 0, 0);
    /* Falling out of the playfield restarts the test player. Gameplay checks the
     * bottom edge itself, because a hall room may send Simon down a link. */
    if (p->y >= 225) { vk_player_reset(p); if (stage == 0) p->y = 176; }
}

void vk_player_tick(VKPlayer *p, int direction, int jump_pressed)
{
    vk_player_tick_map(p, vk_player_room, 1, direction, jump_pressed);
}
