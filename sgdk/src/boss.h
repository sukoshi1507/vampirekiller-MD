#ifndef VK_BOSS_H
#define VK_BOSS_H

/* Boss of stage 3, object 12h, and what follows its defeat. Logic only: the view is in
 * actors.c. ROM: run routine BE70, spawn BE35, end script B6AB, orb B777/B7A1; proved by
 * code and openMSX traces in research/stage3-boss.md. Everything steps once per pass, one
 * frame out of two, like the C800 objects of the MSX.
 * The boss of stage 6, object 13h (research/stage6-boss.md), shares the damage, the fireball,
 * the bar, the end script and the orb; its room script CE01, its run routine BD19 and its
 * orb floor are the ones of kind 1. */

typedef unsigned char VKBossU8;

#define VK_BOSS_STAGE 3
#define VK_BOSS6_STAGE 6
#define VK_BOSS9_STAGE 9
#define VK_BOSS15_STAGE 15
#define VK_BOSS15_ROOM 9
#define VK_BOSS15_DOOR_ROOM 8
#define VK_BOSS12_STAGE 12
#define VK_BOSS12_ROOM 6
#define VK_BOSS12_DOOR_ROOM 5
#define VK_BOSS_DOOR_ROOM 4
#define VK_BOSS_ROOM 5
#define VK_BOSS9_DOOR_ROOM 8
#define VK_BOSS9_ROOM 7
#define VK_BOSS9_WALKERS 2
#define VK_BOSS_ENERGY 0x80          /* C418 */
#define VK_BOSS_DAMAGE_CONTACT 4     /* 81CE: 2, doubled by 8193 without the armor */
#define VK_BOSS_SCORE 20             /* 81CE: 20h, in hundreds */
#define VK_BOSS_SHOTS 2              /* Legacy array: preserve existing offsets. */
#define VK_BOSS_MAX_SHOTS 8

/* Events of a frame. */
#define VK_BOSS_EV_ORB_TILES 0x01    /* 57C0: the orb patterns replace the boss patterns */
#define VK_BOSS_EV_MUSIC 0x02        /* B720: music 8C */
#define VK_BOSS_EV_HEAL 0x04         /* B742: energy + 1, effect 01 */
#define VK_BOSS_EV_EXIT 0x08         /* B772: C409 = 1, next stage */
#define VK_BOSS_EV_FLASH 0x10        /* 75D4: C445 ran out, C43E = 18h */
#define VK_BOSS_EV_FIRE 0x20         /* BF44: a shot was created (sound 0A only for kind 2, B9A9) */
#define VK_BOSS_EV_STOP 0x80         /* B725: explicit stop before music after orb pickup */
#define VK_BOSS_EV_ERASE 0x40        /* BD01, 490A: the bust (80h, 70h, 20h x 20h) is wiped to colour 0 */

typedef struct {
    VKBossU8 active;                 /* 0 absent, 1 shot, 2 stage12 harmless D700 burst; Y/X in 8.8 */
    VKBossU8 anim;                   /* +C of the D700 slot, kept from one shot to the next (9F6D does not set it); kind 1 pose 9E78 */
    unsigned short y, x;
    short vy, vx;
} VKBossShot;

/* One walker of the stage 9 boss, object 14h (B87C, B893): the +x bytes of the C800 block. */
typedef struct {
    VKBossU8 alive, armed;           /* armed: +E bit 0 (a thrown weapon touches once until 999F) */
    VKBossU8 state, moving, pose;    /* +1, +6, +B */
    VKBossU8 wait, timer;            /* +10, +11 */
    VKBossU8 facing, anim, idx, dir; /* +12 (1: left), +13, +14, +15 */
    VKBossU8 turns;                  /* +17 */
    unsigned short y, x;
    short vy, vx;
} VKBossWalker;

typedef struct {
    VKBossU8 in_room;
    VKBossU8 phase;                  /* passes: one frame out of two */
    VKBossU8 tick;                   /* C003 for the heal */
    VKBossU8 alive;                  /* the C800 object 12h */
    VKBossU8 state, pose, anim;      /* +1, +B, +C */
    VKBossU8 moving;                 /* +6 */
    unsigned short y, x;             /* +2/+3, +4/+5 */
    short vy, vx;                    /* +7/+8, +9/+A */
    VKBossU8 wait, tgt_y, tgt_x;     /* +10, +11, +12 */
    VKBossU8 idle, count;            /* +13, +14 */
    VKBossU8 energy;                 /* C418 */
    VKBossU8 armed;                  /* +E bit 0 cleared by a thrown weapon (8083), set by 999F */
    VKBossU8 fired;
    VKBossU8 flash_wait;             /* C445 */
    VKBossU8 ending;                 /* CE0B */
    VKBossU8 script;                 /* CE10 */
    VKBossU8 script_wait;            /* CE02 */
    VKBossU8 orb, orb_state, orb_pose, orb_anim, orb_wait;
    VKBossU8 orb_y;
    VKBossU8 orb_landed;             /* CE14 */
    VKBossU8 orb_taken;              /* CE11 */
    VKBossShot shot[VK_BOSS_SHOTS];
    VKBossU8 kind;                   /* 0: stage3, 1: stage6, 2: stage9, 3: stage12, 4: stage15, 5: stage18 adapter (dracula.c) */
    VKBossU8 room_step, room_wait;   /* CE01, CE02 of the room script (kind 1) */
    VKBossU8 p12, p13, p14, p15;     /* +12, +13, +14, +15 of object 13h */
    /* Kind 2 (stage 9), after the fields the scenarios address by offset. */
    /* Kind3 reuses these slots: idx=+C, wait=+10, timer=+11, facing=+12,
     * turns=HP (+D), alive=2 while a death flame occupies that C800 slot.
     * p14 is CE07; p15 tracks the flame slots; vx is shared CE09. */
    VKBossWalker walker[VK_BOSS9_WALKERS];
    unsigned short shot_tgt[VK_BOSS_SHOTS], shot_slope[VK_BOSS_SHOTS];   /* D700 type 7, 9E90: +10/+11 (the line the shot swings about) and +12/+13 (its slope) */
    /* Appended: stage12 collision map (7B98); previous field offsets unchanged. */
    const unsigned char *collision;
    /* Appended for stage15: the first two slots above retain their offsets. */
    VKBossShot extra_shot[VK_BOSS_MAX_SHOTS-VK_BOSS_SHOTS];
    VKBossU8 shot_state[VK_BOSS_MAX_SHOTS], shot_pose[VK_BOSS_MAX_SHOTS];
    /* Kinds 0-2: passes left of the 2-entry 1E flame that 7806 -> 9A3A builds in the C800 block of each body killed (12h/13h: [0]; 14h: [0], [1]);
     * only the SAT entries it holds are modelled (actors.c), not its picture. */
    VKBossU8 flame[2];
} VKBoss;

/* One sprite: size 2 (16 x 16) or 4 (32 x 32), tile from the first boss or orb tile. */
typedef struct {
    short x, y;
    unsigned short tile;
    VKBossU8 size;
} VKBossSprite;
#define VK_BOSS_SHARED_FLAME_TILE 0x8000 /* F4/F8 in the common flame tile bank */
#define VK_BOSS_MAX_SPRITES (6 + VK_BOSS_MAX_SHOTS + 1)
/* Kind 1 tiles (boss6_md.c): 8 boss cells of 4 tiles (pose 2B: 0 to 3, pose 2C: 4 to 7), then the 4 fireball
 * cells (poses 27, 28, 29, 2A). */
#define VK_BOSS6_SHOT_TILE 32

/* Kind 2 sprites (boss9_md.c): cells (pose - 33) * 3 + part of 4 tiles, the fireball cells 39 and 3A after them, then
 * the blinking cells of the walkers (second layer in colour 08), computed when a walker blinks: 2 x 3 cells. */
#define VK_BOSS9_SHOT_TILE 72
#define VK_BOSS9_FLASH_TILE 80
#define VK_BOSS9_FLASH_TILES 24
/* Stage 9: tile k of the walker w that blinks is flash tile VK_BOSS9_FLASH_TILE + w * 12 + ... (see vk_boss_sprites). */

/* Stage with this boss ported: the door of its room leads to the boss room (7718, 77D1). */
int vk_boss_door(int stage, int room);
int vk_boss_room(int stage, int room);
/* The boss room of a stage with a boss door (3 and 6: 5, 9: 7). */
int vk_boss_room_of(int stage);
/* 77D1: Simon's place in the boss room (trace): X, and Y (-1: the Y of the door room goes on). */
int vk_boss_entry_x(int stage);
int vk_boss_entry_y(int stage);
void vk_boss_reset(VKBoss *b);
/* 6319-6333: the room is entered (stage 3). */
void vk_boss_begin(VKBoss *b);
void vk_boss_set_collision(VKBoss *b,const unsigned char *map);
void vk_boss_clock_init(void);
/* Optional shared allocation, outside VKBoss192. slot0 body,1..8 D700.
 * Returns1 reserved,0 SAT exhausted,-1 physical slot occupied (try next). */
extern int (*vk_boss15_pool)(int slot,int count);
void vk_boss_begin_stage(VKBoss *b, int stage);
/* One frame. d012: difficulty, it scales the aim (A0F1). energy: C415, for the heal. */
int vk_boss_frame(VKBoss *b, int simon_x, int simon_y, int d012, int frozen, int energy);
/* 83EA -> 847A, 83F2 -> 84A7, 83FA -> 84F0. state = C420. */
int vk_boss_touches(const VKBoss *b, int simon_x, int simon_y, int state);
/* The same: the X of the body that touches Simon, or -1 (kind 2 has two walkers). */
int vk_boss_touch_x(const VKBoss *b, int simon_x, int simon_y, int state);
/* Thrown weapons (801E, 8083, 999F): target i (0, or 0 and 1 for the walkers) that can still be hit, with its place and box. */
int vk_boss_target(const VKBoss *b, int i, int *x, int *y, int *h, int *l);
void vk_boss_set_armed(VKBoss *b, int i);
void vk_boss_rearm(VKBoss *b);
int vk_boss_whip_touches(const VKBoss *b, int simon_x, int simon_y, int facing_left, int state, int chain);
/* 7E2C: C416 = 0 or 2 -> 8, otherwise 12. Returns 1 when the boss is beaten (7E17). */
int vk_boss_hit(VKBoss *b, int c416);
int vk_boss_score(const VKBoss *b);
int vk_boss_touch_damage(const VKBoss *b,int sx,int sy,int state);
int vk_boss_rider_whip(const VKBoss *b,int sx,int sy,int left,int state,int chain);
int vk_boss_rider_hit(VKBoss *b,int power);
/* 8318 -> 847A, the shot that touches or -1; 8320 -> 84A7 for the whip. */
int vk_boss_shot_touches(const VKBoss *b, int simon_x, int simon_y, int state);
int vk_boss_shot_whip_touches(const VKBoss *b, int simon_x, int simon_y, int facing_left, int state, int chain);
/* 7F79 -> 8258 -> 8328/8348 -> 84F0: the box (X, Y, H, L) of the active fireball i for the thrown weapons, 0 if none;
 * vk_boss_shot_kill removes it (822A, 100 points; traced at 7F96 on the stages 3 and 6, with an axe). */
int vk_boss_shot_target(const VKBoss *b, int i, int *x, int *y, int *h, int *l);
void vk_boss_shot_kill(VKBoss *b, int i);
void vk_boss_shot_shield(VKBoss *b,int i);
int vk_boss_shot_count(const VKBoss *b);
const VKBossShot *vk_boss_shot_data(const VKBoss *b,int i);
/* 8390 -> 847A on the landed orb (B832: +E = 2). Sets CE11. */
int vk_boss_orb_touch(VKBoss *b, int simon_x, int simon_y, int state);
int vk_boss_sprites(const VKBoss *b, VKBossSprite out[VK_BOSS_MAX_SPRITES]);
/* boss_body_flames.c (MD build): the 1E flames of the bodies of stages 3, 6, 9 (research/boss-body-flame.md), timed on VBlanks (now = low byte of vtimer);
 * the pictures are appended, once per frame, to the n sprites of vk_boss_sprites (new count returned). */
#define VK_BOSS_FLAME_FRAMES 34   /* frames on screen */
int vk_boss_body_flames(VKBoss *b, VKBossSprite *out, int n, unsigned char now);

#endif
