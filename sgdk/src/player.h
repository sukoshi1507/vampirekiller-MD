#ifndef VK_PLAYER_H
#define VK_PLAYER_H

/* Plain integer types keep the ROM primitives testable on a host compiler. */
typedef unsigned char VKByte;
typedef unsigned short VKWord;

typedef struct {
    VKByte x, y, facing, mode, phase, clock, pose;
    VKByte crouched, on_stairs, stair_anim;
    VKByte stair_dir, stair_len; /* flight direction (1 = up right) and length in px */
    VKByte stair_glide;          /* arrival slide from a hall link, not a tile flight */
    VKByte stair_move;           /* C435: step under way, 1 up / 2 down, 0 on a cell edge */
    VKByte jump_frames;          /* frames since the jump began, paces the arc at 30 Hz */
} VKPlayer;

VKByte vk_sample_tile(const VKByte *map, VKByte x, VKByte y);
int vk_tile_solid(VKByte tile, VKByte stage, VKByte special);
int vk_grounded(const VKByte *map, VKByte x, VKByte y, VKByte stage, VKByte special);
int vk_wall(const VKByte *map, VKByte x, VKByte y, int direction,
            VKByte crouched, VKByte stage, VKByte special);
void vk_jump_step(VKByte *y, VKByte *phase);
void vk_player_reset(VKPlayer *player);
void vk_player_tick(VKPlayer *player, int direction, int jump_pressed);
void vk_player_tick_controls(VKPlayer *player, const VKByte *map, VKByte stage,
                             int direction, int jump_pressed, int down, int up);
/* Same, with the jump table chosen by C431 bit 4 (6CE7): high_jump != 0 takes the
 * 21 steps of 6D94 (object 0D), else the 19 steps of 6D95. */
#define VK_JUMP_STEPS 19
#define VK_JUMP_STEPS_HIGH 21
void vk_player_tick_jump(VKPlayer *player, const VKByte *map, VKByte stage,
                         int direction, int jump_pressed, int down, int up, int high_jump);
/* Bottom exit of a flight (76EB): Simon goes on along it in the room below, whose
 * map is given (room change by the caller). */
void vk_stair_resume(VKPlayer *player, const VKByte *map, VKByte stage);
/* Top exit of a flight (76A4): steps from the arrival (X, Y) up the diagonal (dir 1
 * climbs to the right) to the floor of the room above (probe 7B68). */
VKByte vk_stair_climb_steps(const VKByte *map, VKByte x, VKByte y, int dir, VKByte stage);
void vk_player_tick_map(VKPlayer *player, const VKByte *map, VKByte stage,
                        int direction, int jump_pressed);

#endif
