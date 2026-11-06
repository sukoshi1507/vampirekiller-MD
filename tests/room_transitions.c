#include <assert.h>
#include <stdio.h>
#include "gameplay.h"
#include "room_manager.h"
#include "items.h"
#include "generated/stage_exits.h"
static const unsigned char expected[11][9][4] = {
    {{255,255,255,1},{255,255,0,2},{255,255,1,255}},
    {{255,255,3,1},{5,255,0,2},{255,255,1,3},{7,255,2,0},{255,255,255,5},{255,1,4,6},{255,255,5,7},{255,3,6,255}},
    {{2,4,255,1},{255,5,0,255},{255,0,3,3},{255,255,2,2},{0,255,255,5},{1,255,4,255}},
    {{255,4,3,1},{255,255,0,2},{255,255,1,3},{255,255,2,0},{0,255,255,5},{255,255,255,255}},
    {{255,255,2,1},{4,255,0,2},{5,255,1,0},{255,0,255,4},{255,1,3,5},{255,2,4,255}},
    {{255,255,1,255},{255,255,2,0},{5,255,255,1},{255,255,4,255},{255,1,5,3},{255,2,255,4}},
    {{3,255,1,255},{255,255,2,0},{255,255,255,1},{255,0,4,255},{255,255,5,3},{255,255,255,255}},
    {{255,255,1,255},{255,255,2,0},{5,255,255,1},{255,255,4,255},{255,255,5,3},{8,2,255,4},{255,3,7,255},{255,4,8,6},{255,5,255,7}},
    {{255,255,255,1},{255,255,0,2},{5,255,1,255},{255,255,255,4},{255,7,3,255},{255,2,255,6},{255,3,5,7},{255,4,6,255}},
    {{255,255,255,1},{255,255,0,2},{255,255,1,3},{255,255,2,4},{255,255,3,5},{6,8,4,255},{255,5,255,255},{255,255,255,255},{5,255,7,255}},
    {{255,255,5,1},{6,255,0,2},{255,255,1,3},{255,255,2,4},{255,255,3,5},{8,255,4,0},{255,1,7,7},{255,255,6,6},{255,5,255,255}},
};
int main(void) {
 int st,room,dir,checks=0,edges=0;
 int spawns=0, stair_mounts=0, stair_crossings=0;
 for(st=1;st<=10;st++) for(room=0;room<vk_stage_room_count(st);room++) {
  const VKRoomLink *links=vk_stage_links(st);
  assert(links[room].up==expected[st][room][0]);
  assert(links[room].down==expected[st][room][1]);
  assert(links[room].left==expected[st][room][2]);
  assert(links[room].right==expected[st][room][3]);
  for(dir=0;dir<4;dir++) {
   VKGame g={0};int target=expected[st][room][dir];
   vk_game_debug_start(&g,st,room);vk_game_room_map(&g);
   if(dir>=2) {
    int moved;g.player.x=dir==2?8:248;g.player.y=96;
    moved=vk_room_try_edge(st,&g.room,&g.player,dir==2?-1:1);
    assert(moved==(target!=255));
    assert(g.room==(target==255?room:target));
    assert(g.player.x==(target==255?(dir==2?8:248):(dir==2?246:10)));
   } else {
    VKEvent ev;g.player.x=128;g.player.y=dir==0?24:225;g.player.mode=4;
    ev=vk_game_tick_controls(&g,0,0,0,0);
    if(target!=255) {assert(ev==VK_EVENT_VERTICAL);assert(g.room==target);}
    else {assert(g.room==room);assert(ev==(dir==0?VK_EVENT_NONE:VK_EVENT_FALL));}
   }
   checks++;edges+=target!=255;
  }
 }
 /* Test every debug arrival for two seconds with no input. */
 for(st=1;st<=10;st++) for(room=0;room<vk_stage_room_count(st);room++) {
  VKGame g={0};int frame,x,y;
  assert(vk_game_debug_start(&g,st,room));vk_game_room_map(&g);
  assert(!vk_wall(vk_game_collision(&g),g.player.x,g.player.y,1,0,st,0));
  assert(!vk_wall(vk_game_collision(&g),g.player.x,g.player.y,-1,0,st,0));
  for(frame=0;frame<120;frame++) {
   assert(vk_game_tick_controls(&g,0,0,0,0)!=VK_EVENT_FALL);
   assert(g.room==room);
  }
  spawns++;
  /* Enumerate staircase mount probes and exercise held-UP movement. These are
   * local movement checks, not a proof of reachability from the level entry. */
  for(y=64;y<=192;y+=8) for(x=24;x<=232;x+=8) {
   vk_game_debug_start(&g,st,room);vk_game_room_map(&g);
   vk_player_reset(&g.player);g.player.x=x;g.player.y=y;
   vk_game_tick_controls(&g,0,0,1,0);
   if(!g.player.on_stairs) continue;
   stair_mounts++;
   for(frame=0;frame<400 && g.player.on_stairs;frame++) {
    VKEvent ev=vk_game_tick_controls(&g,0,0,1,0);
    assert(g.room<vk_stage_room_count(st));
    if(ev==VK_EVENT_VERTICAL) { stair_crossings++;break; }
    if(ev==VK_EVENT_FALL) break;
   }
  }
 }
 printf("Safe arrivals %d/73; staircase mounts exercised %d; vertical crossings %d\n",spawns,stair_mounts,stair_crossings);
 printf("73 rooms: %d directional entries checked, %d directed passages exercised PASS\n",checks,edges);
 /* Approach the ten level exits using the actual maps and movement controller,
  * then run the full opening and advance to the next stage. */
 for(st=1;st<=10;st++) {
  VKGame g={0};VKItems it;const VKStageItems *s=&kStageItems[st-1];int n,exit=0;
  vk_game_debug_start(&g,st,s->door_room);vk_game_room_map(&g);
  g.player.x=s->door_x<128?32:228;g.player.y=s->door_y+48;
  vk_items_begin(&it);it.keys=VK_KEY_WHITE;
  for(n=0;n<360;n++) {
   int direction=vk_items_controls_locked(&it)?0:s->door_x<128?-1:1;
   VKEvent e=vk_game_tick_controls(&g,direction,0,0,0);
   assert(e!=VK_EVENT_FALL&&e!=VK_EVENT_VERTICAL&&e!=VK_EVENT_ROOM);
   assert(!direction||!vk_room_try_edge(st,&g.room,&g.player,direction));
   if(vk_items_tick(&it,&g)==VK_ITEM_STAGE_EXIT){exit=1;break;}
  }
  assert(exit);
  if(st<10) {
   vk_game_enter_stage(&g,st+1);vk_items_enter_stage(&it);vk_game_room_map(&g);
   assert(g.stage==st+1&&g.room==0&&it.door==VK_DOOR_CLOSED);
   assert(g.player.x==kStageEntries[st].x&&g.player.y==kStageEntries[st].y);
  }
  printf("Stage %d: approach and exit PASS%s\n",st,st==10?" (title dispatch remains in main)":"; next stage loaded");
 }
 return 0;
}
