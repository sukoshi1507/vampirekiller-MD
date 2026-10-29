#ifndef VK_ITEMS_H
#define VK_ITEMS_H

#include "gameplay.h"

/* White key and exit door of the castle stages: hall room 2 key and room 7 door
 * (research/door-room7.md, section 16, runs K1 to K4 in openMSX), stage 2 room 5
 * key and room 1 door (section 18, run K4 of the stage 2). No SGDK dependency:
 * tested natively. */

#define VK_KEY_WHITE 0x01            /* C701 bit 0 */
#define VK_DOOR_LIFT_MAX 43          /* bar offset caps at 43 px: 4 rows stay visible */

typedef enum {
    VK_DOOR_CLOSED = 0,
    VK_DOOR_OPENING = 1,             /* bar lifting (C5AC 2), Simon frozen */
    VK_DOOR_WALKING = 2,             /* bar up (C5AC 3), Simon walks out alone */
    VK_DOOR_DONE = 3                 /* stage left */
} VKDoorState;

typedef enum {
    VK_ITEM_NONE = 0,
    VK_ITEM_KEY_TAKEN = 1,           /* hide the floor key, show the HUD icon */
    VK_ITEM_DOOR_OPENED = 2,         /* hide the HUD icon, the bar starts to lift */
    VK_ITEM_DOOR_STEP = 3,           /* the bar moved: redraw it */
    VK_ITEM_STAGE_EXIT = 4           /* Simon left by the door: D000 + 1, room 0, x 16 */
} VKItemEvent;

typedef struct {
    VKByte keys;                     /* C701 */
    VKByte white_key_taken;          /* the key of the stage does not come back */
    VKByte door;                     /* VKDoorState */
    VKByte timer;                    /* frames since the door opened */
    VKByte white_key_revealed;       /* the whip opened its hidden block (candles.c) */
} VKItems;

void vk_items_begin(VKItems *items);
/* New life (state 4): 70DC keeps only bit 7 of C701, so a white key not used
 * yet is lost, and 62E6 decodes the stage stream again (5A49): the key is back
 * in its hidden block. */
void vk_items_new_life(VKItems *items);
/* A room is loaded (5B1B), or loaded again: a white key revealed but not taken
 * was a C500 object of the room left, and is gone. Its hidden block is back
 * (the stream flag is cleared only by the pickup, 8C0B; candles.c). */
void vk_items_enter_room(VKItems *items);
/* A new stage (state 10 -> 4): 5A49 decodes the new stage stream, with its key
 * block, and BB37 closes its door again (C5AC := FF at each room load). */
void vk_items_enter_stage(VKItems *items);
/* Room of the current stage holding the white key block: 1 and its X, Y. */
int vk_items_key_place(const VKGame *game, VKByte *x, VKByte *y);
/* The current room has the exit door of its stage (BB61 entry with bit 7). */
int vk_items_door_room(const VKGame *game);
/* The white key lies on the floor of the current room (revealed, not taken). */
int vk_items_key_shown(const VKItems *items, const VKGame *game);
/* Bar offset in pixels, 0 (closed) to VK_DOOR_LIFT_MAX. */
int vk_items_door_lift(const VKItems *items);
/* Opening the door or walking out: the player's controls are ignored. */
int vk_items_controls_locked(const VKItems *items);
/* One frame after the player tick: closed door wall, key pickup, door opening,
 * bar lift and the walk out. Moves Simon (door wall, walk out). */
VKItemEvent vk_items_tick(VKItems *items, VKGame *game);

#endif
