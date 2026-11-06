/* Stage 14 room data access/persistence; never performs a gameplay action. */
#include "stage14_scene.h"
void vk_stage14_scene_reset(VKStage14Scene *s) { unsigned r; if (!s) return; for(r=0;r<VK_STAGE14_ROOMS;r++) {s->candles[r]=0;s->fixed[r]=0;} }
int vk_stage14_scene_reset_room(VKStage14Scene *s,unsigned r) {if(!s||r>=VK_STAGE14_ROOMS)return 0;s->candles[r]=0;s->fixed[r]=0;return 1;}
unsigned vk_stage14_scene_candle_count(unsigned r) { return r<VK_STAGE14_ROOMS ? vk_stage14_candle_first[r+1]-vk_stage14_candle_first[r]:0; }
unsigned vk_stage14_scene_fixed_count(unsigned r) { return r<VK_STAGE14_ROOMS ? vk_stage14_fixed_first[r+1]-vk_stage14_fixed_first[r]:0; }
unsigned vk_stage14_scene_raw_count(unsigned r) { return r<VK_STAGE14_ROOMS ? vk_stage14_raw_first[r+1]-vk_stage14_raw_first[r]:0; }
const VKCandleDef *vk_stage14_scene_candle(unsigned r,unsigned i) {return i<vk_stage14_scene_candle_count(r)?&vk_stage14_candles[vk_stage14_candle_first[r]+i]:0;}
const VKCandleDef *vk_stage14_scene_fixed(unsigned r,unsigned i) {return i<vk_stage14_scene_fixed_count(r)?&vk_stage14_fixed[vk_stage14_fixed_first[r]+i]:0;}
const VKStage14RawSlot *vk_stage14_scene_raw(unsigned r,unsigned i) {return i<vk_stage14_scene_raw_count(r)?&vk_stage14_raw[vk_stage14_raw_first[r]+i]:0;}
const VKByte *vk_stage14_scene_map(unsigned r) {return r<VK_STAGE14_ROOMS?vk_stage14_tiles[r]:0;}
const VKRoomLink *vk_stage14_scene_link(unsigned r) {return r<VK_STAGE14_ROOMS?&vk_stage14_raw_links[r]:0;}
int vk_stage14_scene_present(const VKStage14Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage14_scene_fixed_count(r):vk_stage14_scene_candle_count(r);if(!s||i>=n)return 0;return !((fix?s->fixed[r]:s->candles[r])&(1u<<i)); }
int vk_stage14_scene_mark(VKStage14Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage14_scene_fixed_count(r):vk_stage14_scene_candle_count(r);if(!s||i>=n)return 0;if(fix)s->fixed[r]|=(VKByte)(1u<<i);else s->candles[r]|=(VKByte)(1u<<i);return 1; }
