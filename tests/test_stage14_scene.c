#include <assert.h>
#include <stdio.h>
#include "stage14_scene.h"
int main(void) {
 VKStage14Scene scene;
 unsigned room,i,nc=0,nf=0,nr=0;
 vk_stage14_scene_reset(&scene);
 for(room=0;room<VK_STAGE14_ROOMS;room++) {
   unsigned c=vk_stage14_scene_candle_count(room),f=vk_stage14_scene_fixed_count(room),r=vk_stage14_scene_raw_count(room);
   assert(vk_stage14_scene_link(room)==&vk_stage14_raw_links[room]);
   assert(vk_stage14_scene_map(room)==vk_stage14_tiles[room]);
   assert(c<=8&&f<=8);
   nc+=c;nf+=f;nr+=r;
   for(i=0;i<c;i++) {assert(vk_stage14_scene_candle(room,i));assert(vk_stage14_scene_present(&scene,room,i,0));assert(vk_stage14_scene_mark(&scene,room,i,0));assert(!vk_stage14_scene_present(&scene,room,i,0));}
   for(i=0;i<f;i++) {assert(vk_stage14_scene_fixed(room,i));assert(vk_stage14_scene_present(&scene,room,i,1));assert(vk_stage14_scene_mark(&scene,room,i,1));assert(!vk_stage14_scene_present(&scene,room,i,1));}
   for(i=0;i<r;i++) assert(vk_stage14_scene_raw(room,i));
   assert(!vk_stage14_scene_candle(room,c));assert(!vk_stage14_scene_fixed(room,f));assert(!vk_stage14_scene_raw(room,r));
   assert(!vk_stage14_scene_mark(&scene,room,c,0));assert(!vk_stage14_scene_mark(&scene,room,f,1));
 }
 assert(nc==22 && nf==10 && nr==33);
 assert(!vk_stage14_scene_map(VK_STAGE14_ROOMS));
 assert(!vk_stage14_scene_link(VK_STAGE14_ROOMS));
 assert(!vk_stage14_scene_reset_room(&scene,VK_STAGE14_ROOMS));
 assert(!vk_stage14_scene_mark(&scene,VK_STAGE14_ROOMS,0,0));
 assert(vk_stage14_scene_reset_room(&scene,VK_STAGE14_ROOMS-1));
 vk_stage14_scene_reset(&scene);
 assert(vk_stage14_scene_present(&scene,0,0,0));
 puts("stage14 scene/object test: PASS");return 0;
}
