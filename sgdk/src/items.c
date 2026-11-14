#include "items.h"

#include "generated/stage_exits.h"

/* Pickup rule of 847A: |X - (key X + 8)| < 13 and Y - 2 - key Y in [0, 42)
 * (K1: taken at Simon x 148, y 192, walking left). */
#define PICK_HALF_W 13
#define PICK_HEIGHT 42

/* K3: C5AF = t / 2 lifts the bar by one pixel per two frames; Simon walks alone from the frame 88 after the opening
 * (778A on the first frame of each pass), one pixel every four frames from the X of the opening. */
#define DOOR_WALK_T0 86      /* walk phase origin: the X steps fall on t = 2 (left) or 4 (right) mod 4 */
#define DOOR_WALK_FIRST 88   /* first 778A call, frame f = 88 after the opening */
static VKByte door_pose;   /* pose of Simon at the opening, kept during the lift */

static const VKStageItems *stage_items(const VKGame *game)
{
    if (game->area != VK_AREA_INTERIOR || game->stage < 1 || game->stage > 18) return 0;
    if(game->stage>=12)return game->room<12?&kLateRoomItems[game->stage-12][game->room]:0;
    return &kStageItems[game->stage - 1];
}

void vk_items_begin(VKItems *items)
{
    int i;for(i=0;i<7;i++)items->late_keys_taken[i]=0;
    items->keys = 0;
    items->white_key_taken = 0;
    items->white_key_revealed = 0;
    items->attacking = 0;
    items->door = VK_DOOR_CLOSED;
    items->timer = 0;
}

void vk_items_new_life(VKItems *items)
{
    int i;for(i=0;i<7;i++)items->late_keys_taken[i]=0;
    items->keys &= 0x80;
    if (items->door == VK_DOOR_CLOSED)
    {
        items->white_key_taken = 0;
        items->white_key_revealed = 0;
    }
}

void vk_items_enter_room(VKItems *items)
{
    items->white_key_revealed = 0;
}

void vk_items_enter_stage(VKItems *items)
{
    int i;for(i=0;i<7;i++)items->late_keys_taken[i]=0;
    items->white_key_taken = 0;
    items->white_key_revealed = 0;
    items->door = VK_DOOR_CLOSED;
    items->timer = 0;
}

int vk_items_key_place(const VKGame *game, VKByte *x, VKByte *y)
{
    const VKStageItems *s = stage_items(game);
    if (!s || game->room != s->key_room) return 0;
    *x = s->key_x;
    *y = s->key_y;
    return 1;
}

int vk_items_door_room(const VKGame *game)
{
    const VKStageItems *s = stage_items(game);
    return s && game->room == s->door_room;
}

int vk_items_key_shown(const VKItems *items, const VKGame *game)
{
    VKByte x, y;
    const VKStageItems *s = stage_items(game);
    return s && (!s->key_hidden || items->white_key_revealed) &&
        !(game->stage>=12 ? (items->late_keys_taken[game->stage-12]&(1u<<game->room)) : items->white_key_taken) && vk_items_key_place(game, &x, &y);
}

int vk_items_door_lift(const VKItems *items)
{
    int lift;
    if (items->door == VK_DOOR_CLOSED) return 0;
    lift = items->timer / 2;
    return lift > VK_DOOR_LIFT_MAX ? VK_DOOR_LIFT_MAX : lift;
}

int vk_items_controls_locked(const VKItems *items)
{
    return items->door == VK_DOOR_OPENING || items->door == VK_DOOR_WALKING;
}

static int touches_key(const VKPlayer *p, VKByte key_x, VKByte key_y)
{
    const int dx = p->x - (key_x + 8);
    const int dy = p->y - 2 - key_y;
    return dx > -PICK_HALF_W && dx < PICK_HALF_W && dy >= 0 && dy < PICK_HEIGHT;
}

int vk_items_door_place(const VKGame *game, VKByte *x, VKByte *y)
{
    const VKStageItems *s = stage_items(game);
    if (!s || game->room != s->door_room) return 0;
    *x = s->door_x; *y = s->door_y;
    return 1;
}

/* 7756-7765: table 7777 indexed by D000 (19 bytes, read in the ROM and executed
 * by tools/verify_rom_markers.py). Non zero: the door of the stage fades the music
 * (request FF) before its effect 1A. The same table restarts the music on arrival
 * (794F reads 7777[D000 - 1]). */
int vk_items_door_fades(int stage)
{
    static const VKByte table_7777[19] = {
        0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1
    };
    return stage >= 0 && stage < 19 && table_7777[stage];
}

/* Original 8580: unsigned byte comparisons, facing moves the probe by 8. */
int vk_items_door_touch(const VKGame *game)
{
    const VKStageItems *s = stage_items(game);
    const VKPlayer *p = &game->player;
    if (!s || game->room != s->door_room) return 0;
    return (VKByte)(p->y - 2 - (s->door_y - 8)) < 56 &&
           (VKByte)(p->x + (p->facing ? -8 : 8) - s->door_x) < 8;
}

VKItemEvent vk_items_tick(VKItems *items, VKGame *game)
{
    VKPlayer *p = &game->player;
    VKByte key_x, key_y;
    const VKStageItems *s = stage_items(game);
    if (!s) return VK_ITEM_NONE;
    if (items->door == VK_DOOR_OPENING || items->door == VK_DOOR_WALKING)
    {
        const int before = vk_items_door_lift(items);
        const int left = s->door_x < 128;
        items->timer++;
        /* 77BA: X - 8 >= F0, so X 248 on the right and X 7 on the left; 7802 one frame later (openMSX, doors of
         * the stages 1, 4 and 5: X 248 or 7 then C000 = 10 at the next frame). */
        if (items->door == VK_DOOR_WALKING && (left ? p->x <= 7 : p->x >= 248) && ++items->end_wait >= 2)
        {
            items->door = VK_DOOR_DONE;
            return VK_ITEM_STAGE_EXIT;
        }
        if (items->timer >= DOOR_WALK_FIRST)
        {
            /* 778A: 80h a pass from the X of the opening (6C94), with the walk cycle of 6CA4 (poses 0, 1, 2, 1 held
             * 8 frames each) and, facing left, the left compositions of 765F (C42E + 10: Simon looks at the door he
             * walks to). openMSX (research/door-pose-phase.md), f = frames after the opening frame (7752), writes of
             * C427 and C42E at f = 88, 90, 92...: 778A only runs on the first frame of each pass (C003 written in the
             * same frame, 8580 too). On the right X + 1 at f 90 then every 4 frames (229 -> 248 at f 162); on the left
             * the first half pixel already takes one (X.00 - 0.80), X - 1 at f 88 then every 4 frames (stage 4 room 3
             * and stage 5 room 5: 26 -> 25 at f 88, 7 at f 160; 77BA and 7802 in that frame, C000 = 10 at f 161, D000
             * + 1 at f 162). Row n of the notes is the state after f = n - 1. */
            const int t = items->timer - DOOR_WALK_T0;
            /* 6CA4 picks the pose from bits 3 and 2 of C003 (00 -> 2, 01 -> 1, 10 -> 0, 11 -> 1), so the cycle is in
             * phase with the global counter, not with the start of the walk: C003 of the frame f after the opening
             * pass is door_c003 + f / 2 (the opening is on a first frame, the next pass counts at f = 2). */
            static const VKByte walk_pose[4] = {2, 1, 0, 1};
            p->pose = (VKByte)(walk_pose[((items->door_c003 + (items->timer >> 1)) >> 2) & 3] + (p->facing ? 3 : 0));
            items->door = VK_DOOR_WALKING;
            if (left) { if ((t & 3) == 2 && p->x > 7) p->x--; }
            else if ((t & 3) == 0 && p->x < 248) p->x++;
        }
        else p->pose = door_pose;   /* C5AC = 2 (7805: RET): Simon keeps the pose of the opening pass while the bar lifts */
        return vk_items_door_lift(items) != before ? VK_ITEM_DOOR_STEP : VK_ITEM_NONE;
    }
    /* 7FE2: RET NZ while C422 != 0, so nothing is picked up during an attack. The key
     * block breaks at whip timer 11 and the key is taken when the timer reaches 0,
     * 11 frames later (trace research/stage6-boss.md: 957 -> 968). */
    if (!items->attacking && vk_items_key_shown(items, game) && vk_items_key_place(game, &key_x, &key_y) &&
        touches_key(p, key_x, key_y))
    {
        items->white_key_taken = 1;
        if(game->stage>=12)items->late_keys_taken[game->stage-12]|=1u<<game->room;
        items->keys |= VK_KEY_WHITE;
        return VK_ITEM_KEY_TAKEN;
    }
    if (vk_items_door_room(game) && items->door == VK_DOOR_CLOSED && (VKByte)(p->y - 2 - (s->door_y - 8)) < 56)
    {
        const int left = s->door_x < 128;
        if (left && p->x < 20) p->x = 20;
        if (!left && p->x > 228) p->x = 228;
        /* 8580 is called from the pass code, so only on the first frame of a 2-frame pass, the frame where 414D
         * counts C003: 209 calls of 16 openings of the stage 4 door, none on a second frame (openMSX,
         * research/door-pose-phase.md). A touch on a second frame opens the door on the next first frame. */
        if (!items->second_frame && (items->keys & VK_KEY_WHITE) && p->facing == left && !p->on_stairs &&
            vk_items_door_touch(game))
        {
            items->keys &= (VKByte)~VK_KEY_WHITE;          /* 7743: RES 0,(C701) */
            items->door = VK_DOOR_OPENING;
            items->timer = 0;
            items->end_wait = 0;
            door_pose = p->pose;
            items->door_c003 = items->c003;
            return VK_ITEM_DOOR_OPENED;
        }
    }
    return VK_ITEM_NONE;
}
