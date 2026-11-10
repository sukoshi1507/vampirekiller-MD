#ifndef VK_DRACULA_H
#define VK_DRACULA_H
/* Stage18 ROM objects and visual state, kept byte-for-byte for differential
 * validation. dracula_view renders the traced background copies and credits;
 * actors.c adapts damage, weapons, sound, score and the bar. The caller seeds
 * palette from the executed room entry and preserves CF32 across room resets.
 * Normal stage12..18 traversal is connected separately in LOT11. */
#define VK_DRACULA_STAGE 18
#define VK_DRACULA_ROOM 9
#define VK_DRACULA_DOOR_ROOM 8
#define VK_DRACULA_ENTRY_X 246
#define VK_DRACULA_ENTRY_MUSIC 0x86
typedef unsigned char VKDByte;
typedef struct {
 VKDByte object[7][32], shot[8][32];
 VKDByte ce[128], energy, difficulty, tick, frozen, shot_clock;
 VKDByte sound[8], sound_count;
 unsigned short sx,sy;
 unsigned long score;
 VKDByte exit_requested, credits_finished, credit_line, video_scroll;
 VKDByte palette[32], visible[7], return_title, completed_cycles, next_stage;
 VKDByte upper, lower, bust, bust_x, blank, paint, bust_layers[4], bust_count;
} VKDracula;
void vk_dracula_init(VKDracula *d);
void vk_dracula_pass(VKDracula *d,int sx,int sy,int difficulty,int tick,int frozen,int music_finished);
typedef struct {short x,y;unsigned short tile;VKDByte pattern;} VKDraculaSprite;
#define VK_DRACULA_MAX_SPRITES 36
int vk_dracula_sprites(const VKDracula *d,VKDraculaSprite *out);
void vk_dracula_transition(VKDracula *d,int completed_cycles);
int vk_dracula_target(const VKDracula *d,int slot,int *x,int *y,int *h,int *l);
int vk_dracula_box(const VKDracula *d,int slot,int *x,int *y,int *h,int *l);
int vk_dracula_contact(const VKDracula *d,int slot,int sx,int sy,int state);
int vk_dracula_whip(const VKDracula *d,int slot,int sx,int sy,int left,int state,int chain);
void vk_dracula_rearm(VKDracula *d);
int vk_dracula_strike(VKDracula *d,int slot,int weapon,int power);
void vk_dracula_projectile_contact(VKDracula *d,int slot);
void vk_dracula_projectile_shield(VKDracula *d,int slot);
int vk_dracula_hit(VKDracula *d,int weapon);
#endif
