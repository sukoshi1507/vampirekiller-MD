#ifndef VK_ITEM_VIEW_H
#define VK_ITEM_VIEW_H

#include <genesis.h>
#include "items.h"

/* Key, HUD key box and door bar of items.c, drawn as plane A tiles in the room
 * palette (PAL0). */

/* After a hall room is shown: loads the tiles, draws the HUD box, the floor key
 * (room 2) and the door bar at its current lift (room 7). */
void vk_item_view_room(const VKItems *items, const VKGame *game);
/* 16-bit graphics: the courtyard screens bake the MSX frame of the HUD boxes, which the 16-bit mode replaces (hud.c):
 * this loads and draws the bevelled box cells of vk_item_view_room, and nothing else. */
void vk_item_view_hud_box(const VKItems *items);
void vk_item_view_key(const VKItems *items, const VKGame *game);
/* Redraws what an event of vk_items_tick changed. */
void vk_item_view_event(VKItemEvent event, const VKItems *items, const VKGame *game);

#endif
