#include "boss.h"
#include "player.h"
#include "enemies.h"   /* Enemies_randomR: the common LD A,R generator (research/hasard-R.md) */

/* A1A2: angle from the distances (index: (dy >> 2 & 38h) + (dx >> 5 & 7)); A1E2: sine. */
static const VKBossU8 angle_table[64] = {
    0x20,0x08,0x04,0x03,0x02,0x02,0x01,0x01,0x38,0x20,0x15,0x0F,0x0C,0x09,0x08,0x07,
    0x3B,0x2B,0x20,0x19,0x14,0x10,0x0E,0x0C,0x3D,0x31,0x27,0x20,0x1A,0x16,0x13,0x11,
    0x3D,0x34,0x2C,0x25,0x20,0x1C,0x18,0x15,0x3E,0x36,0x2F,0x29,0x24,0x20,0x1C,0x19,
    0x3E,0x38,0x32,0x2C,0x28,0x23,0x20,0x1D,0x3E,0x39,0x34,0x2F,0x2A,0x26,0x23,0x20
};
static const VKBossU8 sine_table[64] = {
    0x00,0x06,0x0C,0x12,0x19,0x1F,0x26,0x2C,0x32,0x38,0x3E,0x44,0x4A,0x50,0x56,0x5C,
    0x62,0x68,0x6D,0x73,0x79,0x7E,0x84,0x89,0x8E,0x93,0x99,0x9E,0xA2,0xA7,0xAC,0xB1,
    0xB5,0xB9,0xBE,0xC2,0xC6,0xCA,0xCE,0xD1,0xD5,0xD8,0xDC,0xDF,0xE2,0xE5,0xE7,0xEA,
    0xED,0xEF,0xF1,0xF3,0xF5,0xF7,0xF8,0xFA,0xFB,0xFC,0xFD,0xFE,0xFE,0xFF,0xFF,0xFF
};

int (*vk_boss15_pool)(int slot,int count);
static VKBossU8 boss15_clock;
void vk_boss_clock_init(void) { boss15_clock=0; }

#define HI(v) ((VKBossU8)((v) >> 8))
int vk_boss_shot_count(const VKBoss *b){return b->kind==4?VK_BOSS_MAX_SHOTS:VK_BOSS_SHOTS;}
const VKBossShot *vk_boss_shot_data(const VKBoss *b,int i)
{
    if(i<0 || i>=vk_boss_shot_count(b))return 0;
    return i<VK_BOSS_SHOTS?&b->shot[i]:&b->extra_shot[i-VK_BOSS_SHOTS];
}
static VKBossShot *boss_shot(VKBoss *b,int i)
{
    return i<VK_BOSS_SHOTS?&b->shot[i]:&b->extra_shot[i-VK_BOSS_SHOTS];
}
static int boss15_floor(const VKBoss *b,int x,int y);
static void boss15_shots_pass(VKBoss *b,int sx,int sy,int difficulty);

static int has_boss(int stage) { return stage == 18 || stage == VK_BOSS_STAGE || stage == VK_BOSS6_STAGE || stage == VK_BOSS9_STAGE || stage == VK_BOSS12_STAGE || stage == VK_BOSS15_STAGE; }
/* 636F: the boss room of the stage (CE00 = 1, 2, 3); 7718: the door room, whose link on the side of the door leads to it. */
int vk_boss_room_of(int stage) { return stage == 18 ? 9 : stage == VK_BOSS15_STAGE ? VK_BOSS15_ROOM : stage == VK_BOSS12_STAGE ? VK_BOSS12_ROOM : stage == VK_BOSS9_STAGE ? VK_BOSS9_ROOM : VK_BOSS_ROOM; }
int vk_boss_door(int stage, int room)
{
    return has_boss(stage) && room == (stage == 18 ? 8 : stage == VK_BOSS15_STAGE ? VK_BOSS15_DOOR_ROOM : stage == VK_BOSS12_STAGE ? VK_BOSS12_DOOR_ROOM : stage == VK_BOSS9_STAGE ? VK_BOSS9_DOOR_ROOM : VK_BOSS_DOOR_ROOM);
}
int vk_boss_room(int stage, int room) { return has_boss(stage) && room == vk_boss_room_of(stage); }
/* 77D1: the door of stage 3 is on the right (right link, X 10, trace: Y 176); the ones of stages 6 and 9 are on the left
 * (left link, X F6h = 246), and Y is not written (trace: 128, the floor of the door room, then the fall). */
int vk_boss_entry_x(int stage) { return (stage == 18 || stage == VK_BOSS6_STAGE || stage == VK_BOSS9_STAGE || stage == VK_BOSS15_STAGE) ? 246 : 10; }
int vk_boss_entry_y(int stage) { return (stage == 18 || stage == VK_BOSS6_STAGE || stage == VK_BOSS9_STAGE || stage == VK_BOSS12_STAGE || stage == VK_BOSS15_STAGE) ? -1 : 176; }

void vk_boss_reset(VKBoss *b)
{
    unsigned i;
    unsigned char *p = (unsigned char *)b;
    for (i = 0; i < sizeof(*b); i++) p[i] = 0;
    b->energy = VK_BOSS_ENERGY;
}

void vk_boss_begin_stage(VKBoss *b, int stage)
{
    vk_boss_reset(b);
    b->in_room = 1;
    b->kind = (VKBossU8)(stage == 18 ? 5 : stage == VK_BOSS15_STAGE ? 4 : stage == VK_BOSS12_STAGE ? 3 : stage == VK_BOSS9_STAGE ? 2 : stage == VK_BOSS6_STAGE);
    if(b->kind==4)b->p12=boss15_clock;
}

void vk_boss_set_collision(VKBoss *b,const unsigned char *map) { b->collision=map; }

void vk_boss_begin(VKBoss *b) { vk_boss_begin_stage(b, VK_BOSS_STAGE); }

/* BE50, BE35 with DE = 7040. */
static void spawn(VKBoss *b)
{
    b->alive = 1;
    b->y = 0x40 << 8;
    b->x = 0x70 << 8;
    b->pose = 0x4F;
    b->idle = 60;
    b->state = 0;
}

/* A0E5, A134, A184: speed = 80h + 8 D012, components scale * sine >> 5. */
static void aim_speed(int from_y, int from_x, int to_y, int to_x, int base, int d012, short *vy, short *vx)
{
    const unsigned speed = ((unsigned)base + 8u * (unsigned)d012) & 0xFF;
    const int dy = to_y - from_y, dx = to_x - from_x;
    const unsigned ady = (unsigned)(dy < 0 ? -dy : dy) & 0xFF, adx = (unsigned)(dx < 0 ? -dx : dx) & 0xFF;
    const unsigned a = angle_table[((ady >> 2) & 0x38) + ((adx >> 5) & 7)];
    const short y = (short)((speed * sine_table[a]) >> 5);
    const short x = (short)((speed * sine_table[0x3F - a]) >> 5);
    *vy = dy < 0 ? (short)-y : y;
    *vx = dx < 0 ? (short)-x : x;
}
static void aim(int from_y, int from_x, int to_y, int to_x, int d012, short *vy, short *vx)
{
    aim_speed(from_y, from_x, to_y, to_x, 0x80, d012, vy, vx);
}

/* BEA6. */
static void choose_target(VKBoss *b, int simon_x, int simon_y, int d012)
{
    const int above = simon_y - 0x30;
    b->tgt_y = (VKBossU8)(above < 0x30 ? 0x30 : above);
    b->tgt_x = (VKBossU8)(simon_x + ((b->count & 1) ? -0x48 : 0x48));
    aim(HI(b->y), HI(b->x), b->tgt_y, b->tgt_x, d012, &b->vy, &b->vx);
    b->state = 3;
}

/* BF44, 9F61: at (X, Y - 16), towards Simon. */
static void fire(VKBoss *b, int simon_x, int simon_y, int d012)
{
    int i;
    for (i = 0; i < VK_BOSS_SHOTS; i++)
    {
        VKBossShot *s = &b->shot[i];
        if (s->active) continue;
        s->active = 1;
        b->fired = 1;
        s->y = (unsigned short)(b->y - (16u << 8));
        s->x = b->x;
        if (b->kind == 1)                /* 9F61, 9FEA: Y and X without their fractions (trace: X C900h) */
        {
            s->y &= 0xFF00;
            s->x &= 0xFF00;
        }
        aim(HI(b->y), HI(b->x), simon_y, simon_x, d012, &s->vy, &s->vx);
        return;
    }
}

/* BE70. */
static void boss_pass(VKBoss *b, int simon_x, int simon_y, int d012)
{
    b->count++;
    b->anim++;
    b->pose = (b->anim & 8) ? 0x4F : 0x4E;
    switch (b->state)
    {
    case 0:                              /* BE95 */
        if (--b->idle == 0) { b->moving = 1; b->state = 1; }
        break;
    case 1:
    case 2:
        choose_target(b, simon_x, simon_y, d012);
        break;
    case 3:                              /* BEF0: until the target or the edge of the area */
    {
        const VKBossU8 y = HI(b->y), x = HI(b->x);
        int stop = (b->vy < 0 && y < 0x30) || y >= 0x90 || (VKBossU8)(x - 0x20) >= 0xB0;
        if (!stop)
        {
            if ((VKBossU8)(y + 8 - b->tgt_y) >= 0x10) break;
            stop = (VKBossU8)(x + 8 - b->tgt_x) < 0x10 || (VKBossU8)(x - 0x20) >= 0xC0;
            if (!stop) break;
        }
        b->vy = b->vx = 0;
        /* BF32: 40h - (R >> 2), R = 0..7Fh (60 traced calls, bit 7 never set, A >> 4 uniform): the common generator. */
        b->wait = (VKBossU8)(0x40 - (Enemies_randomR() >> 2));
        b->state = 4;
        break;
    }
    case 4:                              /* BF4A */
    {
        int dx;
        if (b->wait == 0x18) fire(b, simon_x, simon_y, d012);
        if (--b->wait) break;
        b->vy = 0x280;
        dx = simon_x - HI(b->x);
        b->vx = (short)((dx < 0 ? -dx : dx) << 3);
        if (dx < 0) b->vx = (short)-b->vx;
        b->state = 5;
        break;
    }
    default:                             /* BF92 */
    {
        const VKBossU8 x = HI(b->x);
        long vy = (long)b->vy - 16;
        if (vy >= 0x800) vy = 0x7FF;
        b->vy = (short)vy;
        if (x < 0x19) b->vx = 0x200;
        else if (x >= 0xE7) b->vx = -0x200;
        if (b->vy < 0 && HI(b->y) < 0x41) choose_target(b, simon_x, simon_y, d012);
        break;
    }
    }
    /* 99B9 */
    if (b->moving)
    {
        b->y = (unsigned short)(b->y + (unsigned short)b->vy);
        b->x = (unsigned short)(b->x + (unsigned short)b->vx);
    }
}

/* BD46: the wing beat and the pull towards Y A0h (A549: +-20h on vy, 7FFh at most downwards). */
static void medusa_flap(VKBoss *b)
{
    unsigned short v = (unsigned short)b->vy;
    b->p14--;
    b->pose = (b->p14 & 8) ? 0x2C : 0x2B;
    v = (unsigned short)(v + (HI(b->y) < 0xA0 ? 0x20u : 0xFFE0u));
    if (!(v & 0x8000) && (v >> 8) >= 8) v = 0x7FF;
    b->vy = (short)v;
}

/* BD69, BDE0: at the edge of the room (X < 10h or X >= F0h) or within 8 of Simon's X. */
static int medusa_turn(const VKBoss *b, int simon_x)
{
    const int x = HI(b->x), d = simon_x - x;
    if ((VKBossU8)(x - 0x10) >= 0xE0) return 1;
    return (d < 0 ? -d : d) < 8;
}

static void medusa_reverse(VKBoss *b, VKBossU8 c12, VKBossU8 c13)
{
    b->vx = (short)-b->vx;
    b->p12 = c12;
    b->p13 = c13;
}

/* BD19 (init BD26, run states BD66 and BD99), the fire BE0D, the move 99B9. */
static void medusa_pass(VKBoss *b, int simon_x, int simon_y, int d012)
{
    if (--b->p15 == 0)
    {
        b->p15 = 0x3C;
        fire(b, simon_x, simon_y, d012);
    }
    if (b->state == 0)
    {
        medusa_flap(b);
        if (medusa_turn(b, simon_x))
        {
            medusa_reverse(b, 0x3F, 0x60);
            b->state = 1;
        }
    }
    else if (--b->p13 != 0)              /* BDDD */
    {
        medusa_flap(b);
        if (medusa_turn(b, simon_x)) medusa_reverse(b, 0x3F, 0x10);
    }
    else                                 /* BDA2: stopped until +12 runs out */
    {
        b->p13 = 1;
        b->moving = 0;
        if (--b->p12 == 0)
        {
            b->vx = (short)(simon_x < HI(b->x) ? -0xF8 : 0xF8);
            b->moving = 1;
            b->state = 0;
            b->p12 = 0x2F;
            b->p13 = 0x4F;
        }
    }
    if (b->moving)
    {
        b->y = (unsigned short)(b->y + (unsigned short)b->vy);
        b->x = (unsigned short)(b->x + (unsigned short)b->vx);
    }
}

/* CE01: 0 waits 78h passes, 1 counts them, 2 creates the object at (90h, 90h) and wipes the bust, 3 waits for
 * CE15 (the defeat), handled by the caller. The object runs in the pass that creates it (trace: CE01 = 3 and
 * the first move on the same frame). */
static int room_pass(VKBoss *b, int simon_x, int simon_y, int d012)
{
    int events = 0;
    switch (b->room_step)
    {
    case 0:                              /* BCE7 */
        b->room_wait = 0x78;
        b->room_step = 1;
        return 0;
    case 1:                              /* BCF1 */
        if (--b->room_wait == 0) b->room_step = 2;
        return 0;
    case 2:                              /* BCF9, BD19 init */
        b->alive = 1;
        b->y = 0x90 << 8;
        b->x = 0x90 << 8;
        b->vx = 0xF8;
        b->vy = 0x10;
        b->pose = 0x2B;
        b->moving = 1;
        b->state = 0;
        b->p15 = 0x3C;
        b->p14 = 8;
        b->room_step = 3;
        events = VK_BOSS_EV_ERASE;
        break;
    default:
        break;
    }
    if (b->alive) medusa_pass(b, simon_x, simon_y, d012);
    return events;
}

/* A549 with the clamp at 7FFh (positive only). */
static short add_vy(short vy, int de)
{
    unsigned short v = (unsigned short)(vy + de);
    if (!(v & 0x8000) && (v >> 8) >= 8) v = 0x7FF;
    return (short)v;
}

static void shots_pass(VKBoss *b)
{
    int i;
    for (i = 0; i < VK_BOSS_SHOTS; i++)
    {
        VKBossShot *s = &b->shot[i];
        if (s->active!=1) continue;
        if (b->kind == 2)                /* 9E90: the line moves by its slope, vy is pulled towards it by 30h (A549) */
        {
            b->shot_tgt[i] = (unsigned short)(b->shot_tgt[i] + b->shot_slope[i]);
            s->vy = add_vy(s->vy, HI(s->y) < HI(b->shot_tgt[i]) ? 0x30 : -0x30);
            s->anim++;
        }
        else if (b->kind == 1) s->anim++;     /* 9E78 */
        s->y = (unsigned short)(s->y + (unsigned short)s->vy);
        s->x = (unsigned short)(s->x + (unsigned short)s->vx);
        /* 9E31 (D700 loop: 9E58, 99B9, 6445, then 99E5) for every kind, the stage 3 ball included (type 01, 9E77 = RET): after the move
         * Y >= E4h, X >= F1h or X < 7 (Y wraps: a ball going up stays until it passes under 0). tests/test_boss3_ball.c runs the ROM loop in a Z80 emulator. */
        if (HI(s->y) >= 0xE4 || HI(s->x) >= 0xF1 || HI(s->x) < 7) s->active = 0;
    }
}

/* B7A1, B818: lands at Y C0h in this room (7B98 at X 78h, trace), stage 6: B0h (trace D, snapshots of the SAT).
 * Landed, the routine of state 2 is B83F, the pose goes on (B853: 8F 90 91 90; stage 6, trace C: poses 90 and 91
 * with the landed object); stage 3 had the pose frozen at 8F there. */
static void orb_pass(VKBoss *b)
{
    static const VKBossU8 frames[4] = { 0x8F, 0x90, 0x91, 0x90 };
    if (b->orb_state == 0)
    {
        if (--b->orb_wait == 0) {
            b->orb_state = 1;
            if(b->kind==3 || b->kind==4)b->orb_y++; /* B7AD enables 99B9 in this pass. */
        }
        return;
    }
    b->orb_pose = frames[(b->orb_anim >> 1) & 3];
    b->orb_anim++;
    if (b->orb_state != 1) return;
    if (b->kind==4 ? boss15_floor(b,0x78,b->orb_y) : b->orb_y >= (b->kind == 1 ? 0xB0 : 0xC0))
    {
        b->orb_pose = 0x8F;
        b->orb_landed = 1;
        b->orb_state = 2;
    }
    else b->orb_y++;
}

/* B6C5. */
static int script_pass(VKBoss *b, int energy)
{
    switch (b->script)
    {
    case 0:
        b->script_wait = 60;
        b->script++;
        return 0;
    case 1:
    case 4:
        if (--b->script_wait == 0) b->script++;
        return 0;
    case 2:                              /* B6ED */
        b->orb = 1;
        b->orb_state = 0;
        b->orb_pose = 0x8F;
        b->orb_anim = 0;
        b->orb_wait = 0x5A;
        b->orb_y = 0x40;
        b->orb_landed = b->orb_taken = 0;
        b->script_wait = 180;
        b->script = 3;
        return VK_BOSS_EV_ORB_TILES;
    case 3:                              /* B706 */
        if (b->orb_taken)
        {
            b->script_wait = 0x96;
            b->script = 4;
            return VK_BOSS_EV_MUSIC | ((b->kind==3 || b->kind==4) ? VK_BOSS_EV_STOP : 0);
        }
        if (!b->orb_landed || --b->script_wait) return 0;
        b->script = 6;                   /* too late: no healing */
        b->script_wait = 0x90;
        return VK_BOSS_EV_MUSIC;
    case 5:                              /* B736 */
        if (b->tick & 1) return 0;
        if (energy < 32) return VK_BOSS_EV_HEAL;
        b->script_wait = 60;
        b->script = 6;
        return 0;
    default:                             /* B751 */
        if (--b->script_wait) return 0;
        return VK_BOSS_EV_EXIT;
    }
}


/* B9B6: +10 = table[R & 7]. R from the common generator (230 traced calls: the 8 entries seen 42 26 22 34 26 25 27 28 times,
 * chi-square 9.8 for 7 degrees of freedom, bit 7 never set). */
static VKBossU8 rand_wait(VKBoss *b)
{
    static const VKBossU8 table[8] = { 0x40, 0x08, 0x30, 0x10, 0x20, 0x18, 0x38, 0x28 };
    (void)b;
    return table[Enemies_randomR() & 7];
}

/* B9CF: the pose cycles 33 34 35 (facing left) or 36 37 38 through +14 = 1, 0, 1, 2, 1, 0, ... every 8 passes. */
static void walker_anim(VKBossWalker *w)
{
    if (++w->anim == 8)
    {
        w->anim = 0;
        if (w->dir & 1)
        {
            if (++w->idx == 3) { w->idx = 1; w->dir = 0; }
        }
        else if ((VKBossU8)--w->idx == 0xFF) { w->idx = 1; w->dir = 1; }
    }
    w->pose = (VKBossU8)((w->facing & 1 ? 0x33 : 0x36) + w->idx);
}

/* B985: the fireball, from (X, Y - 38h) towards Simon at 40h + 8 D012, its X component pointing the way the walker faces. */
static void walker_fire(VKBoss *b, VKBossWalker *w, int simon_x, int simon_y, int d012)
{
    int i;
    for (i = 0; i < VK_BOSS_SHOTS; i++)
    {
        VKBossShot *s = &b->shot[i];
        short vy, vx;
        if (s->active) continue;
        aim_speed(HI(w->y) - 0x38, HI(w->x), simon_y, simon_x, 0x40, d012, &vy, &vx);
        if ((((w->facing & 1) << 7) ^ (VKBossU8)HI((unsigned short)vx)) & 0x80) vx = (short)-vx;   /* A17C */
        s->active = 1;
        s->y = (unsigned short)(((HI(w->y) - 0x18) & 0xFF) << 8);       /* 9F61 style: C = Y - 18h, no fractions */
        s->x = (unsigned short)(HI(w->x) << 8);
        s->vy = vy;
        s->vx = vx;
        b->shot_tgt[i] = (unsigned short)(HI(s->y) << 8);               /* A04D: +10 = 0, +11 = Y */
        b->shot_slope[i] = (unsigned short)(vy - 0x100);                /*       +12/+13 = vy - 100h */
        b->fired = 1;
        return;
    }
}

/* B87C: the creation (the generic part, 5F77, leaves +7/+8 alone and +9/+A as the block had them). */
static void walker_init(VKBossWalker *w, int x)
{
    w->alive = 1;
    w->armed = 0;
    w->state = 0;
    w->moving = 0;
    w->pose = 0x36;
    w->wait = 0x20;
    w->y = 0xC5 << 8;
    w->x = (unsigned short)(x << 8);
    w->vy = 0;
    w->idx = 2;                          /* +14 = 2, +15 = 1 */
    w->dir = 1;
}

static void set_x_hi(VKBossWalker *w, int x) { w->x = (unsigned short)((w->x & 0x00FF) | (x << 8)); }

/* B893: the four states (0 waits, 1 picks a way, 2 walks, 3 blinks and fires). The move (99B9) is the caller's. */
static void walker_pass(VKBoss *b, VKBossWalker *w, int simon_x, int simon_y, int d012)
{
    const VKBossU8 x = HI(w->x);
    if (w->state == 0)                   /* B8A1 */
    {
        if (--w->wait == 0) { w->state = 1; w->moving = 1; }
        return;
    }
    if (w->state == 1)                   /* B8AD */
    {
        w->facing = 1;
        w->vx = (short)0xFDD0;
        if (!(simon_x < x)) { w->vx = 0x230; w->facing = 0; }
        w->state = 2;
        w->anim = 7;
        w->wait = rand_wait(b);
        return;
    }
    if (w->state == 3)                   /* B960: the layers blink with the bit 0 of +11 (vk_boss_sprites) */
    {
        if (--w->timer) return;
        walker_fire(b, w, simon_x, simon_y, d012);   /* B985 */
        w->state = 2;
        w->moving = 1;
        return;
    }
    walker_anim(w);                      /* B8D0 */
    if (--w->wait != 0)                  /* B930: at the edge of the area it turns round */
    {
        const int negative = w->vx < 0;
        if (x >= 0x20 && x < 0xE1) return;
        w->vx = 0x230;
        w->facing = 0;
        set_x_hi(w, 0x20);
        if (!negative)
        {
            w->vx = (short)0xFDD0;
            set_x_hi(w, 0xE0);
            w->facing++;
        }
        walker_anim(w);
        w->wait = rand_wait(b);          /* B95E: JR B9B6 */
        return;
    }
    w->wait = rand_wait(b);              /* B8D8 */
    w->anim = 7;
    w->turns++;
    if (w->turns & 1)
    {
        if (w->vx < 0) goto right;       /* B8E8 */
        goto left;
    }
    if (simon_x >= x) goto right;        /* B8F0 */
left:                                    /* B8F8 */
    w->vx = (short)0xFDD0;
    w->facing = 1;
    if (simon_x < x) goto stop;
    walker_anim(w);
right:                                   /* B90D */
    w->vx = 0x230;
    w->facing = 0;
    if (simon_x >= x) goto stop;
    walker_anim(w);
    return;
stop:                                    /* B922 */
    w->state = 3;
    w->moving = 0;
    w->timer = 0x30;
    walker_anim(w);
}

/* B857: CE01 = 0 creates the two walkers at (30h, C5h) and (D0h, C5h), then the room waits for CE15 (the defeat, BE47).
 * The walkers run in the pass that creates them (trace: the first sample has +10 = 1Fh). */
static int room9_pass(VKBoss *b, int simon_x, int simon_y, int d012)
{
    int i;
    if (b->room_step == 0)
    {
        walker_init(&b->walker[0], 0x30);
        walker_init(&b->walker[1], 0xD0);
        b->room_step = 1;
    }
    for (i = 0; i < VK_BOSS9_WALKERS; i++)
    {
        VKBossWalker *w = &b->walker[i];
        if (!w->alive) continue;
        walker_pass(b, w, simon_x, simon_y, d012);
        if (w->moving)                   /* 99B9 */
        {
            w->y = (unsigned short)(w->y + (unsigned short)w->vy);
            w->x = (unsigned short)(w->x + (unsigned short)w->vx);
        }
    }
    b->alive = (VKBossU8)(b->walker[0].alive | b->walker[1].alive);
    return 0;
}

/* 9B71: the death flame of a body of stages 3, 6, 9 ends after 16 passes. */
static void boss_body_flames_pass(VKBoss *b){int i;for(i=0;i<2;i++)if(b->flame[i])b->flame[i]--;}
#include "boss12.inc"
#include "boss15.inc"

int vk_boss_frame(VKBoss *b, int simon_x, int simon_y, int d012, int frozen, int energy)
{
    int events = 0;
    if (!b->in_room || b->kind==5) return 0; /* Stage18 is stepped by dracula.c through actors.c. */
    b->phase ^= 1;
    if (b->phase) return 0;
    b->tick++;
    if (b->flash_wait && --b->flash_wait == 0) events |= VK_BOSS_EV_FLASH;
    if (b->ending)
    {
        if(b->kind<3)boss12_bursts_pass(b),boss_body_flames_pass(b);
        if(b->kind==3 || b->kind==4) {
            int script_events;
            if(b->script==0) {
                int i;boss12_flame(b,0,0);boss12_flame(b,1,0);
                for(i=0;i<vk_boss_shot_count(b);i++)boss12_shot_disappear(b,i);
            }
            script_events=script_pass(b,energy);
            boss12_flames_pass(b);boss12_bursts_pass(b);
            if(b->orb)orb_pass(b); /* Orb +7E=0: it is not frozen by D010. */
            if(script_events&VK_BOSS_EV_EXIT) {b->ending=0;b->orb_taken=0;b->in_room=0;}
            return events|script_events;
        }
        if (b->orb && !frozen) orb_pass(b);
        return events | script_pass(b, energy);
    }
    if (b->energy == 0)                  /* BE42: CE15, next pass of the room script */
    {
        b->ending = 1;
        b->script = 0;
        if(b->kind<3)boss12_bursts_pass(b),boss_body_flames_pass(b);
        if(b->kind==3 || b->kind==4) {
            if(b->kind==3 && b->walker[1].alive==1)boss12_rider(b,simon_x,simon_y,d012);
            boss12_flames_pass(b);boss12_bursts_pass(b);
        }
        return events;
    }
    if(b->kind==3 && !b->room_step)boss12_init(b); /* BA26 runs before 98E5. */
    if(b->kind==4)boss15_room(b); /* Room script runs even during D010. */
    boss12_bursts_pass(b); /* +7E=0, including while D010 freezes the fighters. */
    if (frozen) return events;           /* 98E5 */
    if(b->kind==4)boss15_body(b);
    else if (b->kind == 3) events |= room12_pass(b,simon_x,simon_y,d012);
    else if (b->kind == 2) events |= room9_pass(b, simon_x, simon_y, d012);
    else if (b->kind) events |= room_pass(b, simon_x, simon_y, d012);
    else if (!b->alive) spawn(b);
    else boss_pass(b, simon_x, simon_y, d012);
    if(b->kind==4)boss15_shots_pass(b,simon_x,simon_y,d012);else shots_pass(b);
    if (b->fired) { b->fired = 0; events |= VK_BOSS_EV_FIRE; }
    return events;
}

static int contact_box(int simon_x, int simon_y, int state, int x, int y, int h, int l)
{
    const int dx = simon_x - x;
    const int high = h + ((state == 1 || state == 2) ? 0x12 : 0x1A);
    const int top = simon_y - (state == 1 ? 8 : 0);
    if ((dx < 0 ? -dx : dx) >= l + 5) return 0;
    return (VKBossU8)(top - 2 - y + h) < high;
}

static int whip_box(int simon_x, int simon_y, int facing_left, int state, int chain, int x, int y, int h, int l)
{
    const int reach = chain ? 0x18 : 0x14;
    const int whip = facing_left ? simon_x - reach : simon_x + reach;
    const int half = l + (chain ? 0x10 : 0x0C);
    const int dx = whip - x;
    const int line = state == 2 ? 0x0A : 0x12;
    if (whip < 0 || whip > 255) return 0;        /* 84B6 */
    if ((dx < 0 ? -dx : dx) >= half) return 0;
    return (VKBossU8)(simon_y - line - y + h) < h + 4;
}

/* Kind 2: the walkers are the objects of class 6 (82CB[14h] = 6): box (H = 28h, L = 5) for the body (8420), the whip (8428)
 * and the thrown weapons (8430); their fireball D700 type 7 is of class 1 (830D[6]): (6, 6), 8338; the fireball of the stage 6 boss is of class 1 too (traced: HL = 0606 at 847A), only the stage 3 one is (6, 3). */
#define W9_H 0x28
#define W9_L 5

int vk_boss_touch_x(const VKBoss *b, int simon_x, int simon_y, int state)
{
    int i;
    if (b->ending || !b->energy) return -1;
    if(b->kind==4)return b->walker[0].alive && contact_box(simon_x,simon_y,state,HI(b->walker[0].x),HI(b->walker[0].y),0x30,8)?HI(b->walker[0].x):-1;
    if(b->kind==3) {
        for(i=0;i<2;i++)if(b->walker[i].alive &&
            contact_box(simon_x,simon_y,state,HI(b->walker[i].x),HI(b->walker[i].y),i?0x10:0x28,i?8:5))
            return HI(b->walker[i].x);
        return -1;
    }
    if (b->kind == 2)
    {
        for (i = 0; i < VK_BOSS9_WALKERS; i++)
            if (b->walker[i].alive &&
                contact_box(simon_x, simon_y, state, HI(b->walker[i].x), HI(b->walker[i].y), W9_H, W9_L))
                return HI(b->walker[i].x);
        return -1;
    }
    if (!b->alive) return -1;
    return contact_box(simon_x, simon_y, state, HI(b->x), HI(b->y), 0x18, 0x0C) ? HI(b->x) : -1;
}

int vk_boss_touches(const VKBoss *b, int simon_x, int simon_y, int state)
{
    return vk_boss_touch_x(b, simon_x, simon_y, state) >= 0;
}

int vk_boss_whip_touches(const VKBoss *b, int simon_x, int simon_y, int facing_left, int state, int chain)
{
    int i;
    if (b->ending || !b->energy) return 0;
    if (b->kind == 2)
    {
        for (i = 0; i < VK_BOSS9_WALKERS; i++)
            if (b->walker[i].alive &&
                whip_box(simon_x, simon_y, facing_left, state, chain, HI(b->walker[i].x), HI(b->walker[i].y), W9_H, W9_L))
                return 1;
        return 0;
    }
    if(b->kind==4)return b->walker[0].alive && whip_box(simon_x,simon_y,facing_left,state,chain,HI(b->walker[0].x),HI(b->walker[0].y),0x30,8);
    if(b->kind==3)return b->walker[0].alive && whip_box(simon_x,simon_y,facing_left,state,chain,HI(b->walker[0].x),HI(b->walker[0].y),0x28,5);
    if (!b->alive) return 0;
    return whip_box(simon_x, simon_y, facing_left, state, chain, HI(b->x), HI(b->y), 0x18, 0x0C);
}

int vk_boss_target(const VKBoss *b, int i, int *x, int *y, int *h, int *l)
{
    if (b->ending || !b->energy) return 0;
    if (b->kind == 2)
    {
        if (i >= VK_BOSS9_WALKERS || !b->walker[i].alive || b->walker[i].armed) return 0;
        *x = HI(b->walker[i].x); *y = HI(b->walker[i].y); *h = W9_H; *l = W9_L;
        return 1;
    }
    /* LOT06 provisoire: full thrown-weapon path against stage15 has not
     * been retraced; shared target adapter uses the traced class7 dimensions. */
    if(b->kind==4) {
        if(i || !b->walker[0].alive || b->walker[0].armed)return 0;
        *x=HI(b->walker[0].x);*y=HI(b->walker[0].y);*h=0x30;*l=8;return 1;
    }
    if(b->kind==3) {
        if(i<0 || i>=2 || !b->walker[i].alive || b->walker[i].armed)return 0;
        *x=HI(b->walker[i].x);*y=HI(b->walker[i].y);*h=i?0x10:0x28;*l=i?8:5;return 1;
    }
    if (i || !b->alive || b->armed) return 0;
    *x = HI(b->x); *y = HI(b->y); *h = 0x18; *l = 0x0C;
    return 1;
}

void vk_boss_set_armed(VKBoss *b, int i)
{
    if (b->kind == 2 || b->kind == 3 || b->kind==4) { if (i >= 0 && i < VK_BOSS9_WALKERS) b->walker[i].armed = 1; }
    else b->armed = 1;
}

void vk_boss_rearm(VKBoss *b)
{
    b->armed = 0;
    b->walker[0].armed = b->walker[1].armed = 0;
}

int vk_boss_hit(VKBoss *b, int c416)
{
    /* 7E2C: table 7E59 (C416 = 0 or 2) or 7E60 [type - 11h]: 12h and 13h 8 and 0Ch, 14h 4 and 6. */
    const int damage = (b->kind == 2 || b->kind == 3 || b->kind==4) ? ((c416 == 0 || c416 == 2) ? 4 : 6) : ((c416 == 0 || c416 == 2) ? 8 : 12);
    int i;
    if (!b->alive) return 0;
    b->energy = (VKBossU8)(b->energy > damage ? b->energy - damage : 0);
    if (b->energy && !(b->energy & 0x80)) return 0;
    /* 7E17: C445 = 14h, 7806 empties the objects. */
    b->energy = 0;
    if(b->kind<3){   /* 7806 -> 9A3A: 99F6 frees the body, CE00 (1, 2, 3 in these rooms) != 0: a 1E flame of 2 entries, 16 passes (9B71) */
        if(b->kind==2){b->flame[0]=b->walker[0].alive?16:0;b->flame[1]=b->walker[1].alive?16:0;}
        else b->flame[0]=16;
    }
    b->alive = 0;
    if(b->kind==3 || b->kind==4) {
        boss12_flame(b,0,24);
        if(b->kind==3 && b->walker[1].alive==1)b->walker[1].wait=8; /* 9AB3 before CE0B is set. */
    } else b->walker[0].alive = b->walker[1].alive = 0;
    b->flash_wait = 0x14;
    for (i = 0; i < vk_boss_shot_count(b); i++) {
        if(b->kind==3 || b->kind==4)boss12_shot_disappear(b,i);else b->shot[i].active=0;
    }
    return 1;
}

int vk_boss_shot_touches(const VKBoss *b,int sx,int sy,int state)
{
    int i;for(i=0;i<vk_boss_shot_count(b);i++) {
        const VKBossShot *s=vk_boss_shot_data(b,i);
        if(s->active==1 && contact_box(sx,sy,state,HI(s->x),HI(s->y),b->kind==4?12:6,(b->kind==0||b->kind==3)?3:6))return i;
    }
    return -1;
}
int vk_boss_shot_whip_touches(const VKBoss *b,int sx,int sy,int left,int state,int chain)
{
    int i;for(i=0;i<vk_boss_shot_count(b);i++) {
        const VKBossShot *s=vk_boss_shot_data(b,i);
        if(s->active==1 && whip_box(sx,sy,left,state,chain,HI(s->x),HI(s->y),b->kind==4?12:6,(b->kind==0||b->kind==3)?3:6))return i;
    }
    return -1;
}
int vk_boss_shot_target(const VKBoss *b,int i,int *x,int *y,int *h,int *l)
{
    const VKBossShot *s=vk_boss_shot_data(b,i);if(!s || s->active!=1)return 0;
    *x=HI(s->x);*y=HI(s->y);*h=b->kind==4?12:6;*l=(b->kind==0||b->kind==3)?3:6;return 1;
}
void vk_boss_shot_kill(VKBoss *b,int i)
{
    if(i<0 || i>=vk_boss_shot_count(b))return;
    if(b->kind==3||b->kind==4)boss12_shot_disappear(b,i);else boss_shot(b,i)->active=0;
}

int vk_boss_orb_touch(VKBoss *b, int simon_x, int simon_y, int state)
{
    if (!b->orb || !b->orb_landed || b->orb_taken) return 0;
    if (!contact_box(simon_x, simon_y, state, 0x78, b->orb_y, 0x10, 8)) return 0;
    b->orb_taken = 1;
    b->orb = 0;
    return 1;
}

static int shared_flames(const VKBoss *b,VKBossSprite *out,int n)
{
    int i;
    if(b->kind>2)return n;
    for(i=0;i<vk_boss_shot_count(b);i++){
        const VKBossShot *s=vk_boss_shot_data(b,i);const int second=((s->anim+1)&4)!=0;
        if(s->active!=2)continue;
        out[n].x=(short)((s->x>>8)-8);out[n].y=(short)((s->y>>8)-(second?14:15));
        out[n].tile=(unsigned short)(VK_BOSS_SHARED_FLAME_TILE+(second?4:0));out[n].size=2;n++;
    }
    return n;
}
int vk_boss_sprites(const VKBoss *b, VKBossSprite out[VK_BOSS_MAX_SPRITES])
{
    /* Cells: 40, 48, 50, 58 (pose 4E), 60, 68, 70 (pose 4F); offsets of the SAT in the
     * trace, screen Y = SAT + 1. Tiles: 4 per cell, the fireball after them. */
    static const signed char cells[7][3] = {
        { -20, -24, 0 }, { -20, -8, 4 }, { -4, -8, 8 }, { -20, 8, 12 },
        { -20, -16, 16 }, { -20, 0, 20 }, { -4, -8, 24 }
    };
    int n = 0, i;
    if(b->kind==4 && !b->orb)return boss15_sprites(b,out);
    if(b->kind==3 && !b->orb)return boss12_sprites(b,out);
    if (b->kind == 2 && !b->orb)
    {
        /* Cells at (X - 8, Y - 46 + 16 part) (SAT Y - 47, + 1), 3 per walker; fireball in (X - 6, Y - 14). */
        int w;
        for (w = 0; w < VK_BOSS9_WALKERS; w++)
        {
            const VKBossWalker *k = &b->walker[w];
            const int blink = k->state == 3 && !(k->timer & 1);
            if (!k->alive) continue;
            for (i = 0; i < 3; i++)
            {
                out[n].x = (short)(HI(k->x) - 8);
                out[n].y = (short)(HI(k->y) - 46 + 16 * i);
                out[n].tile = (unsigned short)(blink ? VK_BOSS9_FLASH_TILE + w * 12 + i * 4 : ((k->pose - 0x33) * 3 + i) * 4);
                out[n].size = 2;
                n++;
            }
        }
        for (i = 0; i < VK_BOSS_SHOTS; i++)
            if (b->shot[i].active==1)
            {
                out[n].x = (short)(HI(b->shot[i].x) - 6);
                out[n].y = (short)(HI(b->shot[i].y) - 14);
                out[n].tile = (unsigned short)(VK_BOSS9_SHOT_TILE + (((b->shot[i].anim - 1) & 4) ? 4 : 0));
                out[n].size = 2;
                n++;
            }
        return shared_flames(b,out,n);
    }
    if (b->kind == 1 && !b->orb)
    {
        /* SAT of the trace (+ 1: screen row = SAT + 1), one pass behind: cells at (X - 16, Y - 30) and (X, Y - 30), 16 rows below. */
        const int set = b->pose == 0x2C ? 4 : 0;
        for (i = 0; i < 4 && b->alive; i++)
        {
            out[n].x = (short)(HI(b->x) + ((i & 2) ? 0 : -16));
            out[n].y = (short)(HI(b->y) + ((i & 1) ? -14 : -30));
            out[n].tile = (unsigned short)((set + i) * 4);
            out[n].size = 2;
            n++;
        }
        for (i = 0; i < VK_BOSS_SHOTS; i++)
            if (b->shot[i].active==1)
            {
                const int pose = 0x27 + ((b->shot[i].anim & 2) ? 1 : 0) + ((b->shot[i].vx < 0) ? 0 : 2);
                out[n].x = (short)(HI(b->shot[i].x) - 8);
                out[n].y = (short)(HI(b->shot[i].y) - 14);
                out[n].tile = (unsigned short)(VK_BOSS6_SHOT_TILE + (pose - 0x27) * 4);
                out[n].size = 2;
                n++;
            }
        return shared_flames(b,out,n);
    }
    if (b->orb)
    {
        /* B7AD: colours off on the odd passes before the fall. */
        if (b->orb_state != 0 || (b->tick & 1))
        {
            out[n].x = 0x78 - 16;
            out[n].y = (short)(b->orb_y - 16 + 1);
            out[n].tile = (unsigned short)((b->orb_pose - 0x8F) * 16);
            out[n].size = 4;
            n++;
        }
        return shared_flames(b,out,n);
    }
    if (b->alive)
    {
        const int first = b->pose == 0x4E ? 0 : 4, count = b->pose == 0x4E ? 4 : 3;
        for (i = 0; i < count; i++)
        {
            out[n].x = (short)(HI(b->x) + cells[first + i][1]);
            out[n].y = (short)(HI(b->y) + cells[first + i][0]);
            out[n].tile = (unsigned char)cells[first + i][2];
            out[n].size = 2;
            n++;
        }
    }
    for (i = 0; i < VK_BOSS_SHOTS; i++)
        if (b->shot[i].active==1)
        {
            out[n].x = (short)(HI(b->shot[i].x) - 8);
            out[n].y = (short)(HI(b->shot[i].y) - 12);
            out[n].tile = 28;
            out[n].size = 2;
            n++;
        }
    return shared_flames(b,out,n);
}

/* 81CE: body15h gives 3000 points; class6 contact doubles base3 to 6. */
int vk_boss_score(const VKBoss *b) {return b->kind==4?70:b->kind==3?30:VK_BOSS_SCORE;}
int vk_boss_touch_damage(const VKBoss *b,int sx,int sy,int state)
{
    if(b->kind==4)return 6;
    if(b->kind!=3)return VK_BOSS_DAMAGE_CONTACT;
    if(b->walker[0].alive && contact_box(sx,sy,state,HI(b->walker[0].x),HI(b->walker[0].y),0x28,5))return 6;
    return 2;
}
int vk_boss_rider_whip(const VKBoss *b,int sx,int sy,int left,int state,int chain)
{
    const VKBossWalker *w=&b->walker[1];
    return b->kind==3 && !b->ending && b->energy && w->alive && whip_box(sx,sy,left,state,chain,HI(w->x),HI(w->y),0x10,8);
}
/* 7DE6/8066 then 9AB3: rider18h recoils for eight passes, it is not deleted. */
int vk_boss_rider_hit(VKBoss *b,int power)
{
    VKBossWalker *w=&b->walker[1];
    if(b->kind!=3 || w->alive!=1 || b->ending || !b->energy)return 0;
    w->armed=1;w->turns=(VKBossU8)(w->turns-power);
    if(w->turns==0 || (w->turns&0x80)) {w->wait=8;return 2;}
    return 0;
}

void vk_boss_shot_shield(VKBoss *b,int i)
{
    if(i>=0 && i<vk_boss_shot_count(b))boss12_shot_disappear(b,i);
}
