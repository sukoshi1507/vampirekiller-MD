#include <assert.h>
#include <stdio.h>
#include "items.h"
#include "room_manager.h"
#include "candles.h"
#include "generated/stage_exits.h"

int main(void)
{
    int stage;
    for (stage=1; stage<=18; ++stage) {
        VKGame g={0}; VKItems it; VKCandles c;
        const VKStageItems *s=&kStageItems[stage-1];
        int t,found=0,oldx;
        VKByte x,y;
        vk_game_enter_stage(&g,stage);
        assert(g.room==0 && g.player.x==kStageEntries[stage-1].x &&
               g.player.y==kStageEntries[stage-1].y && g.player.facing==kStageEntries[stage-1].facing);
        vk_items_begin(&it);vk_candles_begin(&c);
        g.room=s->key_room;vk_candles_enter(&c,&g);
        assert(vk_items_key_place(&g,&x,&y) && x==s->key_x && y==s->key_y);
        assert(vk_items_key_shown(&it,&g)==!s->key_hidden);
        if (s->key_hidden) {
            for (t=0;t<c.count;t++) {
                const VKCandleDef *d=vk_candles_def(&c,t);
                if (d->kind==VK_OBJ_WHITE_KEY) {
                    assert(d->x==x && d->y==y);
                    assert(vk_candles_hit(&c,t)&VK_CANDLE_EVENT_WHITE_KEY);
                    it.white_key_revealed=1;found++;
                }
            }
            assert(found==1);
        }
        g.player.x=x+8;g.player.y=y+2;
        assert(vk_items_tick(&it,&g)==VK_ITEM_KEY_TAKEN);
        vk_candles_white_key_taken(&c);
        assert(!vk_items_key_shown(&it,&g) && (it.keys&VK_KEY_WHITE));
        vk_items_enter_room(&it);assert(!vk_items_key_shown(&it,&g));
        g.room=s->door_room;g.player.x=s->door_x<128?20:228;
        g.player.y=s->door_y+16;g.player.facing=s->door_x<128;
        assert(vk_items_door_place(&g,&x,&y) && x==s->door_x && y==s->door_y);
        it.keys=0;assert(vk_items_tick(&it,&g)==VK_ITEM_NONE);
        it.keys=VK_KEY_WHITE;g.player.facing^=1;
        assert(vk_items_tick(&it,&g)==VK_ITEM_NONE);
        g.player.facing^=1;g.player.y=s->door_y-7;
        assert(vk_items_tick(&it,&g)==VK_ITEM_NONE);
        g.player.y=s->door_y+50;assert(vk_items_tick(&it,&g)==VK_ITEM_NONE);
        g.player.y=s->door_y+16;g.player.on_stairs=1;
        assert(vk_items_tick(&it,&g)==VK_ITEM_NONE);g.player.on_stairs=0;
        assert(vk_items_tick(&it,&g)==VK_ITEM_DOOR_OPENED);
        assert(!(it.keys&VK_KEY_WHITE) && vk_items_controls_locked(&it));
        /* The walk starts from the X of the opening: right 228 -> 248 at t 167, exit at t 168; left 20 -> 7 at
         * t 85 + 4 * 13 = 137, exit at t 138 (openMSX: 26 -> 7 at t 161, research/door-room7.md). */
        for(t=1;t<=(s->door_x<128?138:168);t++) {
            VKItemEvent ev;oldx=g.player.x;ev=vk_items_tick(&it,&g);
            assert(s->door_x<128 ? g.player.x<=oldx : g.player.x>=oldx);
            assert(t<=88 || (g.player.pose>=3)==(s->door_x<128)); /* walk poses (the opening pose is kept up to t 88) of the side Simon faces */
            assert((ev==VK_ITEM_STAGE_EXIT)==(t==(s->door_x<128?138:168)));
        }
        assert(!vk_items_controls_locked(&it));
        vk_items_enter_stage(&it);assert(it.door==VK_DOOR_CLOSED && !it.white_key_taken);
        vk_items_new_life(&it);assert(!it.white_key_revealed);
        printf("stage %d: entry, key, door guards, animation, exit and reset PASS\n",stage);
    }
    /* Multiple late white keys have independent stream flags (stage16/17). */
    for(stage=16;stage<=17;stage++){
        VKGame g={0};VKItems it;int room,n=0;vk_game_enter_stage(&g,stage);vk_items_begin(&it);
        for(room=0;room<vk_stage_room_count(stage);room++){
            VKByte x,y;g.room=room;vk_items_enter_room(&it);
            if(!vk_items_key_place(&g,&x,&y))continue;
            it.white_key_revealed=1;g.player.x=x+8;g.player.y=y+2;
            assert(vk_items_tick(&it,&g)==VK_ITEM_KEY_TAKEN);n++;
        }
        assert(n==(stage==16?3:2));
        for(room=0;room<vk_stage_room_count(stage);room++){g.room=room;it.white_key_revealed=1;assert(!vk_items_key_shown(&it,&g));}
        vk_items_new_life(&it);g.room=stage==16?5:10;assert(vk_items_key_shown(&it,&g));
    }
    {
        int stage, room, count = 0;
        VKGame g = {0};
        for (stage = 1; stage <= 18; stage++) {
            for (room = 0; room < vk_stage_room_count(stage); room++) {
                assert(vk_game_debug_start(&g, stage, room));
                assert(g.area == VK_AREA_INTERIOR && g.stage == stage && g.room == room);
                assert(vk_game_room_map(&g));
                count++;
            }
            assert(!vk_game_debug_start(&g, stage, vk_stage_room_count(stage)));
            assert(g.stage == stage && g.room == vk_stage_room_count(stage)-1);
        }
        assert(!vk_game_debug_start(&g, 0, 0));
        assert(!vk_game_debug_start(&g, 19, 0));
        assert(!vk_game_debug_start(&g, 255, 0));
        assert(g.stage == 18 && g.room == 9);
        printf("Debug selection: %d playable rooms and invalid selections PASS\n", count);
    }
    return 0;
}
