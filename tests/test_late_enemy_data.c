#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "late_enemy_data.h"
static const unsigned char counts[7] = {12,12,8,10,10,12,10};
static const unsigned char expectedFixed[7] = {7,32,15,10,6,10,13};
static const unsigned char expected[7][12] = {
 {0,0,0,0,0,0,0,0,0,0,0,0},
 {0,0,128,0,0,0,128,128,128,128,128,128},
 {128,128,128,128,128,128,128,128,0,0,0,0},
 {0,128,128,128,0,0,0,128,0,0,0,0},
 {0,0,0,0,0,0,0,0,0,0,0,0},
 {0,0,0,128,0,128,0,64,0,0,64,0},
 {0,128,0,0,0,0,0,128,128,0,0,0}
};
int main(void) {
 assert(vk_late_enemy_update_address(0x09)==0xAF5F);
 assert(vk_late_enemy_update_address(0x0B)==0xB37E);
 assert(vk_late_enemy_update_address(0x0D)==0xB248);
 assert(vk_late_enemy_update_address(0x0E)==0xA9B7);
 assert(vk_late_enemy_update_address(0x10)==0xADF8);
 assert(vk_late_enemy_update_address(0x12)==0xBE70);
 assert(vk_late_enemy_update_address(0)==0);

 unsigned int stage,room,slot,total=0,fixed=0;
 uint32_t hash=2166136261u;
 for(stage=12;stage<=18;stage++) {
  unsigned int stageFixed=0;
  assert(vk_late_enemy_room_count((unsigned char)stage)==counts[stage-12]);
  for(room=0;room<counts[stage-12];room++) {
   VkLateEnemyFlag b=vk_late_enemy_room_flag((unsigned char)stage,(unsigned char)room);
   assert(b.valid==1 && b.value==expected[stage-12][room]);
   total++;
  }
  for(room=0;room<12;room++) {
   for(slot=0;slot<4;slot++) {
    VkLateFixedEnemy b=vk_late_fixed_enemy((unsigned char)stage,(unsigned char)room,(unsigned char)slot);
    unsigned char raw[4];unsigned int k;
    if(room>=counts[stage-12]) assert(b.valid==0);
    else {
     assert(b.valid==1);
     assert(b.x==(unsigned char)((b.packed_yx&15)*16));
     assert(b.y==(unsigned char)(b.packed_yx&240));
     if(b.raw_type) { fixed++;stageFixed++; }
    }
    raw[0]=b.raw_type; raw[1]=b.packed_yx; raw[2]=b.reserved;raw[3]=b.identifier;
    for(k=0;k<4;k++) hash=(hash^raw[k])*16777619u;
   }
  }
  assert(stageFixed==expectedFixed[stage-12]);
  assert(!vk_late_enemy_room_flag((unsigned char)stage,counts[stage-12]).valid);
  assert(!vk_late_fixed_enemy((unsigned char)stage,0,4).valid);
  assert(!vk_late_fixed_enemy((unsigned char)stage,255,0).valid);
 }
 assert(!vk_late_enemy_room_flag(11,0).valid);
 assert(!vk_late_fixed_enemy(19,0,0).valid);
 assert(fixed==93);
 assert(hash==0x500916c5u);
 printf("late-stage Z80 enemy data: %u room flags, %u fixed records, 7 stages PASS\n",total,fixed);
 return 0;
}
