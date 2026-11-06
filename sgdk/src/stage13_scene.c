/* Stage 13 room data access/persistence; never performs a gameplay action. */
#include "stage13_scene.h"
void vk_stage13_scene_reset(VKStage13Scene *s) { unsigned r; if (!s) return; for(r=0;r<VK_STAGE13_ROOMS;r++) {s->candles[r]=0;s->fixed[r]=0;} }
int vk_stage13_scene_reset_room(VKStage13Scene *s,unsigned r) {if(!s||r>=VK_STAGE13_ROOMS)return 0;s->candles[r]=0;s->fixed[r]=0;return 1;}
unsigned vk_stage13_scene_candle_count(unsigned r) { return r<VK_STAGE13_ROOMS ? vk_stage13_candle_first[r+1]-vk_stage13_candle_first[r]:0; }
unsigned vk_stage13_scene_fixed_count(unsigned r) { return r<VK_STAGE13_ROOMS ? vk_stage13_fixed_first[r+1]-vk_stage13_fixed_first[r]:0; }
unsigned vk_stage13_scene_raw_count(unsigned r) { return r<VK_STAGE13_ROOMS ? vk_stage13_raw_first[r+1]-vk_stage13_raw_first[r]:0; }
const VKCandleDef *vk_stage13_scene_candle(unsigned r,unsigned i) {return i<vk_stage13_scene_candle_count(r)?&vk_stage13_candles[vk_stage13_candle_first[r]+i]:0;}
const VKCandleDef *vk_stage13_scene_fixed(unsigned r,unsigned i) {return i<vk_stage13_scene_fixed_count(r)?&vk_stage13_fixed[vk_stage13_fixed_first[r]+i]:0;}
const VKStage13RawSlot *vk_stage13_scene_raw(unsigned r,unsigned i) {return i<vk_stage13_scene_raw_count(r)?&vk_stage13_raw[vk_stage13_raw_first[r]+i]:0;}
const VKByte *vk_stage13_scene_map(unsigned r) {return r<VK_STAGE13_ROOMS?vk_stage13_tiles[r]:0;}
const VKRoomLink *vk_stage13_scene_link(unsigned r) {return r<VK_STAGE13_ROOMS?&vk_stage13_raw_links[r]:0;}
int vk_stage13_scene_present(const VKStage13Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage13_scene_fixed_count(r):vk_stage13_scene_candle_count(r);if(!s||i>=n)return 0;return !((fix?s->fixed[r]:s->candles[r])&(1u<<i)); }
int vk_stage13_scene_mark(VKStage13Scene *s,unsigned r,unsigned i,int fix) { unsigned n=fix?vk_stage13_scene_fixed_count(r):vk_stage13_scene_candle_count(r);if(!s||i>=n)return 0;if(fix)s->fixed[r]|=(VKByte)(1u<<i);else s->candles[r]|=(VKByte)(1u<<i);return 1; }
