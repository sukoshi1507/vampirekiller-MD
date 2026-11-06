#include <assert.h>
#include <stdio.h>
#include "stage17_scene.h"
int main(void) {
 VKStage17Scene scene;
 unsigned room,i,nc=0,nf=0,nk=0,nr=0;
 vk_stage17_scene_reset(&scene);
 for(room=0;room<VK_STAGE17_ROOMS;room++) {
   unsigned c=vk_stage17_scene_candle_count(room),f=vk_stage17_scene_fixed_count(room),k=vk_stage17_scene_white_key_count(room),r=vk_stage17_scene_raw_count(room);
   assert(vk_stage17_scene_link(room)==&vk_stage17_raw_links[room]);
   assert(vk_stage17_scene_map(room)==vk_stage17_tiles[room]);
   assert(c<=8&&f<=8);
   nc+=c;nf+=f;nk+=k;nr+=r;
   for(i=0;i<c;i++) {assert(vk_stage17_scene_candle(room,i));assert(vk_stage17_scene_present(&scene,room,i,0));assert(vk_stage17_scene_mark(&scene,room,i,0));assert(!vk_stage17_scene_present(&scene,room,i,0));}
   for(i=0;i<f;i++) {assert(vk_stage17_scene_fixed(room,i));assert(vk_stage17_scene_present(&scene,room,i,1));assert(vk_stage17_scene_mark(&scene,room,i,1));assert(!vk_stage17_scene_present(&scene,room,i,1));}
   for(i=0;i<k;i++) {const VKCandleDef *key=vk_stage17_scene_white_key(room,i);assert(key && key->kind==0x18);}
   for(i=0;i<r;i++) assert(vk_stage17_scene_raw(room,i));
   assert(!vk_stage17_scene_candle(room,c));assert(!vk_stage17_scene_fixed(room,f));assert(!vk_stage17_scene_raw(room,r));
   assert(!vk_stage17_scene_mark(&scene,room,c,0));assert(!vk_stage17_scene_mark(&scene,room,f,1));
 }
 assert(nc==37 && nf==18 && nk==1 && nr==58);
 assert(!vk_stage17_scene_white_key(VK_STAGE17_ROOMS,0));
 assert(!vk_stage17_scene_map(VK_STAGE17_ROOMS));
 assert(!vk_stage17_scene_link(VK_STAGE17_ROOMS));
 assert(!vk_stage17_scene_reset_room(&scene,VK_STAGE17_ROOMS));
 assert(!vk_stage17_scene_mark(&scene,VK_STAGE17_ROOMS,0,0));
 assert(vk_stage17_scene_reset_room(&scene,VK_STAGE17_ROOMS-1));
 vk_stage17_scene_reset(&scene);
 for(room=0;room<VK_STAGE17_ROOMS;room++) if(vk_stage17_scene_candle_count(room)) assert(vk_stage17_scene_present(&scene,room,0,0));
 puts("stage17 scene/object test: PASS");return 0;
}
