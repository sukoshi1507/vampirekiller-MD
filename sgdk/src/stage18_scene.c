/* Stage 18 room data access/persistence; never performs a gameplay action. */
#include "stage18_scene.h"
void vk_stage18_scene_reset(VKStage18Scene *s) { unsigned r; if (!s) return; for(r=0;r<VK_STAGE18_ROOMS;r++) {s->candles[r]=0;s->fixed[r]=0;} }
int vk_stage18_scene_reset_room(VKStage18Scene *s,unsigned r) {if(!s||r>=VK_STAGE18_ROOMS)return 0;s->candles[r]=0;s->fixed[r]=0;return 1;}
unsigned vk_stage18_scene_candle_count(unsigned r) { return r<VK_STAGE18_ROOMS ? vk_stage18_candle_first[r+1]-vk_stage18_candle_first[r]:0; }
unsigned vk_stage18_scene_fixed_count(unsigned r) { return r<VK_STAGE18_ROOMS ? vk_stage18_fixed_first[r+1]-vk_stage18_fixed_first[r]:0; }
unsigned vk_stage18_scene_white_key_count(unsigned r) { return r<VK_STAGE18_ROOMS ? vk_stage18_white_key_first[r+1]-vk_stage18_white_key_first[r]:0; }
unsigned vk_stage18_scene_raw_count(unsigned r) { return r<VK_STAGE18_ROOMS ? vk_stage18_raw_first[r+1]-vk_stage18_raw_first[r]:0; }
const VKCandleDef *vk_stage18_scene_candle(unsigned r,unsigned i) {return i<vk_stage18_scene_candle_count(r)?&vk_stage18_candles[vk_stage18_candle_first[r]+i]:0;}
const VKCandleDef *vk_stage18_scene_fixed(unsigned r,unsigned i) {return i<vk_stage18_scene_fixed_count(r)?&vk_stage18_fixed[vk_stage18_fixed_first[r]+i]:0;}
const VKCandleDef *vk_stage18_scene_white_key(unsigned r,unsigned i) {return i<vk_stage18_scene_white_key_count(r)?&vk_stage18_white_keys[vk_stage18_white_key_first[r]+i]:0;}
const VKStage18RawSlot *vk_stage18_scene_raw(unsigned r,unsigned i) {return i<vk_stage18_scene_raw_count(r)?&vk_stage18_raw[vk_stage18_raw_first[r]+i]:0;}
const VKByte *vk_stage18_scene_map(unsigned r) {return r<VK_STAGE18_ROOMS?vk_stage18_tiles[r]:0;}
const VKRoomLink *vk_stage18_scene_link(unsigned r) {return r<VK_STAGE18_ROOMS?&vk_stage18_raw_links[r]:0;}
int vk_stage18_scene_present(const VKStage18Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage18_scene_fixed_count(r):vk_stage18_scene_candle_count(r);if(!s||i>=n)return 0;return !((fix?s->fixed[r]:s->candles[r])&(1u<<i)); }
int vk_stage18_scene_mark(VKStage18Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage18_scene_fixed_count(r):vk_stage18_scene_candle_count(r);if(!s||i>=n)return 0;if(fix)s->fixed[r]|=(VKByte)(1u<<i);else s->candles[r]|=(VKByte)(1u<<i);return 1; }
