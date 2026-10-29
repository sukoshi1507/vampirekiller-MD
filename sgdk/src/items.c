#include "items.h"

/* White key block and exit door of each castle stage (index D000 - 1).
 * Hall: white key at x 128..143, y 160..175, inside the hidden block of candle
 * slot C490 (01 A0 80 00 03 18); stage 2: block 0x18 of room 5 at (192,112).
 * The key shows once the whip hits its block (candles.c, 883E): 89FD creates it
 * at the X, Y of the block, without the + 16 of the other block objects.
 * Door: BB37 reads BB61 + 3 x D000 at each room load: 87 30 EC (stage 1, room
 * 7) and 81 30 EC (stage 2, room 1). Bit 7 = a key door (C5AC := FF, 53FC draws
 * the bar), C5AD = 48, C5AE = 236 in both: same bar, same place (trace K2 of
 * research/door-room7.md section 18: stage 2 room 1 C5AC FF -> 1 by 53FC). */
typedef struct {
    VKByte key_room, key_x, key_y;
    VKByte door_room;
} VKStageItems;
static const VKStageItems kStageItems[2] = {
    { 2, 128, 160, 7 },
    { 5, 192, 112, 1 },
};
/* Pickup rule of 847A: |X - (key X + 8)| < 13 and Y - 2 - key Y in [0, 42)
 * (K1: taken at Simon x 148, y 192, walking left). */
#define PICK_HALF_W 13
#define PICK_HEIGHT 42

/* Door room: the routine at 8580 opens the door when Simon faces right with
 * X in [228, 236) and Y in [42, 98) (C5AD = 48, C5AE = 236). The closed bar is a
 * wall at X 228 on that ledge (K4: 261 frames at X 228, no way through). */
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

static const VKStageItems *stage_items(const VKGame *game)
{
    if (game->area != VK_AREA_INTERIOR || game->stage < 1 || game->stage > 2) return 0;
    return &kStageItems[game->stage - 1];
}

void vk_items_begin(VKItems *items)
{
    items->keys = 0;
    items->white_key_taken = 0;
    items->white_key_revealed = 0;
    items->door = VK_DOOR_CLOSED;
    items->timer = 0;
}

void vk_items_new_life(VKItems *items)
{
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
    return items->white_key_revealed && !items->white_key_taken && vk_items_key_place(game, &x, &y);
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

static int on_door_ledge(const VKPlayer *p)
{
    return p->y >= DOOR_Y_MIN && p->y < DOOR_Y_END;
}

VKItemEvent vk_items_tick(VKItems *items, VKGame *game)
{
    VKPlayer *p = &game->player;
    VKByte key_x, key_y;
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
    if (vk_items_key_shown(items, game) && vk_items_key_place(game, &key_x, &key_y) &&
        touches_key(p, key_x, key_y))
    {
        items->white_key_taken = 1;
        items->keys |= VK_KEY_WHITE;
        return VK_ITEM_KEY_TAKEN;
    }
    if (vk_items_door_room(game) && items->door == VK_DOOR_CLOSED && on_door_ledge(p))
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
