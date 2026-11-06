/* Stage 16 room data access/persistence; never performs a gameplay action. */
#include "stage16_scene.h"
void vk_stage16_scene_reset(VKStage16Scene *s) { unsigned r; if (!s) return; for(r=0;r<VK_STAGE16_ROOMS;r++) {s->candles[r]=0;s->fixed[r]=0;} }
int vk_stage16_scene_reset_room(VKStage16Scene *s,unsigned r) {if(!s||r>=VK_STAGE16_ROOMS)return 0;s->candles[r]=0;s->fixed[r]=0;return 1;}
unsigned vk_stage16_scene_candle_count(unsigned r) { return r<VK_STAGE16_ROOMS ? vk_stage16_candle_first[r+1]-vk_stage16_candle_first[r]:0; }
unsigned vk_stage16_scene_fixed_count(unsigned r) { return r<VK_STAGE16_ROOMS ? vk_stage16_fixed_first[r+1]-vk_stage16_fixed_first[r]:0; }
unsigned vk_stage16_scene_white_key_count(unsigned r) { return r<VK_STAGE16_ROOMS ? vk_stage16_white_key_first[r+1]-vk_stage16_white_key_first[r]:0; }
unsigned vk_stage16_scene_raw_count(unsigned r) { return r<VK_STAGE16_ROOMS ? vk_stage16_raw_first[r+1]-vk_stage16_raw_first[r]:0; }
const VKCandleDef *vk_stage16_scene_candle(unsigned r,unsigned i) {return i<vk_stage16_scene_candle_count(r)?&vk_stage16_candles[vk_stage16_candle_first[r]+i]:0;}
const VKCandleDef *vk_stage16_scene_fixed(unsigned r,unsigned i) {return i<vk_stage16_scene_fixed_count(r)?&vk_stage16_fixed[vk_stage16_fixed_first[r]+i]:0;}
const VKCandleDef *vk_stage16_scene_white_key(unsigned r,unsigned i) {return i<vk_stage16_scene_white_key_count(r)?&vk_stage16_white_keys[vk_stage16_white_key_first[r]+i]:0;}
const VKStage16RawSlot *vk_stage16_scene_raw(unsigned r,unsigned i) {return i<vk_stage16_scene_raw_count(r)?&vk_stage16_raw[vk_stage16_raw_first[r]+i]:0;}
const VKByte *vk_stage16_scene_map(unsigned r) {return r<VK_STAGE16_ROOMS?vk_stage16_tiles[r]:0;}
const VKRoomLink *vk_stage16_scene_link(unsigned r) {return r<VK_STAGE16_ROOMS?&vk_stage16_raw_links[r]:0;}
int vk_stage16_scene_present(const VKStage16Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage16_scene_fixed_count(r):vk_stage16_scene_candle_count(r);if(!s||i>=n)return 0;return !((fix?s->fixed[r]:s->candles[r])&(1u<<i)); }
int vk_stage16_scene_mark(VKStage16Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage16_scene_fixed_count(r):vk_stage16_scene_candle_count(r);if(!s||i>=n)return 0;if(fix)s->fixed[r]|=(VKByte)(1u<<i);else s->candles[r]|=(VKByte)(1u<<i);return 1; }
