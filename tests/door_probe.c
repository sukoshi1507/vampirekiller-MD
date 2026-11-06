#include "items.h"
int door_probe(int stage,int room,int x,int y,int facing) {
    VKGame g={0};g.area=VK_AREA_INTERIOR;g.stage=stage;g.room=room;
    g.player.x=x;g.player.y=y;g.player.facing=facing;
    return vk_items_door_touch(&g);
}
