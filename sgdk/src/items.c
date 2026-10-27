#include "items.h"

/* Hall room 2: white key on the floor, x 128..143, y 160..175 (object C520, X 80,
 * Y A0, type 18). Picked up by contact: in K1 it went at Simon x 148, y 192 while
 * he walked left, so Simon's side is taken as x-5..x+5 (the floor probe width),
 * height y-31..y. PROVISIONAL for the other approach directions. */
#define KEY_ROOM 2
#define KEY_LEFT 128
#define KEY_TOP 160
#define KEY_SIZE 16
#define SIMON_HALF_W 5
#define SIMON_H 31

/* Hall room 7: the routine at 8580 opens the door when Simon faces right with
 * X in [228, 236) and Y in [42, 98) (C5AD = 48, C5AE = 236). The closed bar is a
 * wall at X 228 on that ledge (K4: 261 frames at X 228, no way through). */
#define DOOR_ROOM 7
#define DOOR_X_MIN 228
#define DOOR_X_END 236
#define DOOR_Y_MIN 42
#define DOOR_Y_END 98
/* K3, frames from the opening (5799): C5AF = t / 2 lifts the bar by one pixel per
 * two frames; Simon starts walking at t 91 (X 229) and gains one pixel every four
 * frames up to X 248 at t 167; the stage changes at t 168 (D000 1 -> 2). */
#define DOOR_WALK_T0 87
#define DOOR_WALK_END_X 248
#define DOOR_EXIT_T 168

void vk_items_begin(VKItems *items)
{
    items->keys = 0;
    items->white_key_taken = 0;
    items->door = VK_DOOR_CLOSED;
    items->timer = 0;
}

static int in_hall_room(const VKGame *game, VKByte room)
{
    return game->area == VK_AREA_INTERIOR && game->room == room;
}

int vk_items_key_shown(const VKItems *items, const VKGame *game)
{
    return !items->white_key_taken && in_hall_room(game, KEY_ROOM);
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

static int touches_key(const VKPlayer *p)
{
    return p->x + SIMON_HALF_W >= KEY_LEFT && p->x - SIMON_HALF_W < KEY_LEFT + KEY_SIZE &&
           p->y >= KEY_TOP && p->y - SIMON_H < KEY_TOP + KEY_SIZE;
}

static int on_door_ledge(const VKPlayer *p)
{
    return p->y >= DOOR_Y_MIN && p->y < DOOR_Y_END;
}

VKItemEvent vk_items_tick(VKItems *items, VKGame *game)
{
    VKPlayer *p = &game->player;
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
            const int x = DOOR_X_MIN + (items->timer - DOOR_WALK_T0) / 4;
            items->door = VK_DOOR_WALKING;
            p->x = (VKByte)(x > DOOR_WALK_END_X ? DOOR_WALK_END_X : x);
        }
        return vk_items_door_lift(items) != before ? VK_ITEM_DOOR_STEP : VK_ITEM_NONE;
    }
    if (!in_hall_room(game, DOOR_ROOM) && !in_hall_room(game, KEY_ROOM)) return VK_ITEM_NONE;

    if (vk_items_key_shown(items, game) && touches_key(p))
    {
        items->white_key_taken = 1;
        items->keys |= VK_KEY_WHITE;
        return VK_ITEM_KEY_TAKEN;
    }
    if (in_hall_room(game, DOOR_ROOM) && items->door == VK_DOOR_CLOSED && on_door_ledge(p))
    {
        if (p->x > DOOR_X_MIN) p->x = DOOR_X_MIN;          /* the bar is a wall */
        if ((items->keys & VK_KEY_WHITE) && !p->facing && !p->on_stairs &&
            p->x >= DOOR_X_MIN && p->x < DOOR_X_END)
        {
            items->keys &= (VKByte)~VK_KEY_WHITE;          /* 7743: RES 0,(C701) */
            items->door = VK_DOOR_OPENING;
            items->timer = 0;
            return VK_ITEM_DOOR_OPENED;
        }
    }
    return VK_ITEM_NONE;
}
