#include <assert.h>
#include <stdio.h>
#include "stage12_scene.h"
#include "stage12_object_view.h"

int main(void)
{
    VKStage12Scene scene;
    unsigned room, i, candles = 0, fixed = 0, records = 0;
    vk_stage12_scene_reset(&scene);
    for (room = 0; room < VK_STAGE12_SCENE_ROOMS; ++room) {
        const VKRoomLink *link = vk_stage12_scene_link(room);
        const VKByte *map = vk_stage12_scene_collision(room);
        unsigned nc = vk_stage12_scene_candle_count(room);
        unsigned nf = vk_stage12_scene_fixed_count(room);
        assert(link && map && map == vk_stage12_tiles[room]);
        assert(link == &vk_stage12_raw_links[room]);
        assert(nc <= VK_STAGE12_SCENE_MAX_OBJECTS && nf <= VK_STAGE12_SCENE_MAX_OBJECTS);
        candles += nc; fixed += nf;
        records += vk_stage12_object_count(room);
        for (i = 0; i < nc; ++i) {
            const VKCandleDef *def = vk_stage12_scene_candle(room,i);
            assert(def == &vk_stage12_candle_defs[vk_stage12_candle_first[room]+i]);
            assert(vk_stage12_scene_candle_present(&scene,room,i));
            assert(vk_stage12_scene_mark_candle_removed(&scene,room,i));
            assert(vk_stage12_scene_mark_candle_removed(&scene,room,i));
            assert(!vk_stage12_scene_candle_present(&scene,room,i));
        }
        assert(!vk_stage12_scene_candle(room,nc));
        assert(!vk_stage12_scene_mark_candle_removed(&scene,room,nc));
        for (i = 0; i < nf; ++i) {
            const VKCandleDef *def = vk_stage12_scene_fixed(room,i);
            assert(def == &vk_stage12_fixed_object_defs[vk_stage12_fixed_object_first[room]+i]);
            assert(vk_stage12_scene_fixed_present(&scene,room,i));
            assert(vk_stage12_scene_mark_fixed_taken(&scene,room,i));
            assert(!vk_stage12_scene_fixed_present(&scene,room,i));
        }
        assert(!vk_stage12_scene_fixed(room,nf));
        assert(!vk_stage12_scene_mark_fixed_taken(&scene,room,nf));
    }
    assert(candles == 30 && fixed == 5 && records == 36);
    /* Must keep all 12 independent states, not alias rooms 9..11 onto older rooms. */
    assert(vk_stage12_scene_candle_count(11) == 1);
    assert(!vk_stage12_scene_candle_present(&scene,11,0));
    assert(vk_stage12_scene_reset_room(&scene,11));
    assert(vk_stage12_scene_candle_present(&scene,11,0));
    assert(!vk_stage12_scene_candle_present(&scene,10,0));
    assert(!vk_stage12_scene_candle_present(&scene,0,0));
    assert(!vk_stage12_scene_reset_room(&scene,12));
    assert(!vk_stage12_scene_link(12) && !vk_stage12_scene_collision(12));
    assert(!vk_stage12_scene_candle(12,0) && !vk_stage12_scene_fixed(12,0));
    assert(!vk_stage12_scene_mark_candle_removed(&scene,12,0));
    assert(!vk_stage12_scene_mark_fixed_taken(&scene,12,0));
    /* No slot at C5B0 is promoted into a fixed item. */
    assert(vk_stage12_scene_fixed_count(4) == 1);
    assert(vk_stage12_object_count(4) == 3);
    vk_stage12_scene_reset(&scene);
    assert(vk_stage12_scene_candle_present(&scene,0,0));
    assert(vk_stage12_scene_candle_present(&scene,11,0));
    puts("test_stage12_scene: OK (12 rooms, 30 candles, 5 fixed, 36 raw slots)");
    return 0;
}
