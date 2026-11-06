/* Stage 10 room/gameplay regression, using ROM-extracted 4F83 room links,
 * 5A49/5B1B object tables, 906C platform records and K3/K4 stage exits.
 * Values are transcribed from the existing research/stage10-level.md and
 * generated ROM tables; this does not establish new openMSX trace evidence. */
#include <assert.h>
#include <stdio.h>
#include "room_manager.h"
#include "platforms.h"
#include "generated/stage10_maps.h"
#include "generated/candles_md.h"
#include "generated/stage_exits.h"

#define N VK_ROOM_NONE

typedef struct { VKByte up, down, left, right; } ExpectedLink;
static const ExpectedLink links[9] = {
    {N,N,5,1}, {6,N,0,2}, {N,N,1,3},
    {N,N,2,4}, {N,N,3,5}, {8,N,4,0},
    {N,1,7,7}, {N,N,6,6}, {N,5,N,N}
};
static const VKByte candle_counts[9] = {2,3,2,3,2,3,4,6,1};
static const VKByte object_counts[9] = {1,1,0,0,1,3,0,0,0};
typedef struct { VKByte room, x, y, kind, content; } Object;
static const Object objects[] = {
    {0,224,128,0x19,0x1A}, {1,144,96,0x17,0},
    {4,64,128,0x19,0x11}, {5,224,128,0x17,0},
    {5,96,64,0x19,0x1C}, {5,112,128,0x19,0x13}
};
typedef struct { VKByte room, x, y, length; } Platform;
static const Platform platforms[] = {
    {0,0x60,0x8F,0x60}, {2,0x40,0xA7,0x80},
    {3,0x20,0x8F,0xA0}, {4,0x80,0xA7,0x40}
};

int main(void)
{
    unsigned r, j, total_candles = 0, total_objects = 0;
    VKPlatforms p;
    assert(vk_stage_room_count(10) == 9);
    assert(vk_stage_links(10) == vk_stage10_links);
    for (r = 0; r < 9; r++) {
        const VKRoomLink *l = &vk_stage10_links[r];
        assert(l->up == links[r].up && l->down == links[r].down);
        assert(l->left == links[r].left && l->right == links[r].right);
        assert(vk_candle_first[10][r+1] - vk_candle_first[10][r] == candle_counts[r]);
        assert(vk_fixed_object_first[10][r+1] - vk_fixed_object_first[10][r] == object_counts[r]);
        total_candles += candle_counts[r];
        total_objects += object_counts[r];
        /* 4F83 extracted maps must be 768 tiles per room; the original
         * bytes are already ROM-verified in the map generator. */
        assert(sizeof vk_stage10_collision[r] == 768);
        for (j = 0; j < 2; j++) {
            int direction = j ? 1 : -1;
            VKByte target = j ? links[r].right : links[r].left;
            VKByte next = (VKByte)r;
            VKPlayer player = {0};
            player.x = j ? 248 : 8;
            player.y = 176; player.mode = 1; player.phase = 7;
            assert(vk_room_try_edge(10, &next, &player, direction) == (target != N));
            assert(next == (target == N ? r : target));
            if (target != N) {
                assert(player.x == (j ? 10 : 246));
                assert(player.y == 176 && player.mode == 1 && player.phase == 7);
            }
            next = (VKByte)r;
            player.x = j ? 247 : 9;
            assert(!vk_room_try_edge(10, &next, &player, direction) && next == r);
            player.x = j ? 248 : 8;
            player.on_stairs = 1;
            assert(!vk_room_try_edge(10, &next, &player, direction) && next == r);
        }
        vk_platforms_room(&p, 10, (VKByte)r);
        for (j = 0; j < sizeof platforms / sizeof platforms[0]; j++)
            if (platforms[j].room == r) break;
        if (j == sizeof platforms / sizeof platforms[0]) {
            assert(p.slot[0].id == 0 && p.slot[1].id == 0);
        } else {
            const Platform *v = &platforms[j];
            unsigned k;
            assert(p.slot[0].id == 1 && p.slot[1].id == 0);
            assert(p.slot[0].x == v->x && p.slot[0].y == v->y);
            assert(p.slot[0].length == v->length && p.slot[0].dx == 1);
            for (k = 0; k < (unsigned)v->length - 1; k++) vk_platforms_step(&p);
            assert(p.slot[0].x == 191 && p.slot[0].dx == 1);
            vk_platforms_step(&p);
            assert(p.slot[0].x == 191 && p.slot[0].dx == -1);
        }
    }
    assert(total_candles == 26 && total_objects == 6);
    for (j = 0; j < sizeof objects / sizeof objects[0]; j++) {
        const Object *o = &objects[j];
        const VKCandleDef *d = &vk_fixed_object_defs[64+j];
        assert(d->x == o->x && d->y == o->y);
        assert(d->kind == o->kind && d->content == o->content);
        assert(64+j >= vk_fixed_object_first[10][o->room]);
        assert(64+j < vk_fixed_object_first[10][o->room+1]);
    }
    assert(vk_wall_items[10][1][0] == 0x12 && vk_wall_items[10][8][0] == 0x04);
    assert(kStageItems[9].key_room == 0 && kStageItems[9].key_x == 48 && kStageItems[9].key_y == 48);
    assert(!kStageItems[9].key_hidden && kStageItems[9].door_room == 8);
    assert(kStageItems[9].door_x == 236 && kStageItems[9].door_y == 128);
    assert(kStageEntries[9].x == 32 && kStageEntries[9].y == 144);
    assert(kStageEntries[10].x == 16 && kStageEntries[10].y == 176);
    puts("Stage 10 rooms: 9 links, 18 edge checks, 26 candles, 6 fixed objects, 4 platforms and exit PASS");
    return 0;
}
