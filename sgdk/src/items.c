#include "items.h"

#include "generated/stage_exits.h"

/* Pickup rule of 847A: |X - (key X + 8)| < 13 and Y - 2 - key Y in [0, 42)
 * (K1: taken at Simon x 148, y 192, walking left). */
#define PICK_HALF_W 13
#define PICK_HEIGHT 42

/* K3, frames from the opening (5799): C5AF = t / 2 lifts the bar by one pixel per
 * two frames; Simon starts walking at t 91 (X 229) and gains one pixel every four
 * frames up to X 248 at t 167; the stage changes at t 168 (D000 1 -> 2). */
#define DOOR_WALK_T0 87
#define DOOR_EXIT_T 168

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
        items->timer++;
        if (items->timer >= DOOR_EXIT_T)
        {
            items->door = VK_DOOR_DONE;
            return VK_ITEM_STAGE_EXIT;
        }
        if (items->timer > DOOR_WALK_T0)
        {
            const int left = s->door_x < 128;
            const int step = (items->timer - DOOR_WALK_T0) / 4;
            const int x = left ? 20 - step : 228 + step;
            /* 778A -> 6CA4: the same walk cycle as the castle entrance (poses 0, 1, 2, 1 held
             * 8 frames each), the routine that moves Simon by 80h a pass also animates him. */
            static const VKByte walk_frames[4] = {0, 1, 2, 1};
            p->pose = walk_frames[((items->timer - DOOR_WALK_T0) >> 3) & 3];
            items->door = VK_DOOR_WALKING;
            p->x = (VKByte)(x < 0 ? 0 : x > 248 ? 248 : x);
        }
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
        if ((items->keys & VK_KEY_WHITE) && p->facing == left && !p->on_stairs &&
            vk_items_door_touch(game))
        {
            items->keys &= (VKByte)~VK_KEY_WHITE;          /* 7743: RES 0,(C701) */
            items->door = VK_DOOR_OPENING;
            items->timer = 0;
            return VK_ITEM_DOOR_OPENED;
        }
    }
    return VK_ITEM_NONE;
}
