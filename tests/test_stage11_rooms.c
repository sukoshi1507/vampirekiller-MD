/* Stage 11: regression based on ROM-extracted 4F83/B99A, 5A49/5B1B,
 * 85A6, fixed enemies and stage-exit tables; see research/stage11-level.md.
 * This is NOT a new openMSX trace or a console-emulator validation. */
#include <assert.h>
#include <stdio.h>
#include "room_manager.h"
#include "platforms.h"
#include "generated/stage11_maps.h"
#include "generated/candles_md.h"
#include "generated/stage_exits.h"
#include "generated/enemy_rooms.h"

static const VKByte counts[6] = {3,7,3,7,3,2};
static const EnemyU8 flags[6] = {0x81,0,0x40,0x40,0x40,0x80};

int main(void)
{
    int room, direction, count15 = 0, fixed = 0, candles = 0;
    assert(vk_stage_room_count(11) == 6);
    assert(vk_stage_links(11) == vk_stage11_links);
    assert(vk_enemy_room_count[11] == 6);
    for (room = 0; room < 6; room++) {
        const VKRoomLink *l = &vk_stage11_links[room];
        VKPlatforms platforms;
        int i;
        assert(l->up == VK_ROOM_NONE && l->down == VK_ROOM_NONE);
        assert(l->left == (room == 0 ? VK_ROOM_NONE : room - 1));
        assert(l->right == (room == 5 ? VK_ROOM_NONE : room + 1));
        assert(vk_enemy_room_flags[11][room] == flags[room]);
        assert(sizeof vk_stage11_collision[room] == 768);
        assert(vk_candle_first[11][room+1] - vk_candle_first[11][room] == counts[room]);
        candles += counts[room];
        for (i = vk_candle_first[11][room]; i < vk_candle_first[11][room+1]; i++)
            count15 += vk_candle_defs[i].kind == 0x15;
        vk_platforms_room(&platforms, 11, (VKByte)room);
        assert(platforms.slot[0].id == 0 && platforms.slot[1].id == 0);
        for (direction = -1; direction <= 1; direction += 2) {
            VKPlayer player = {0};
            VKByte next = (VKByte)room;
            const int connected = direction > 0 ? room < 5 : room > 0;
            player.x = direction > 0 ? 248 : 8;
            player.y = 176; player.mode = 1; player.phase = 7;
            assert(vk_room_try_edge(11, &next, &player, direction) == connected);
            assert(next == (connected ? room + direction : room));
            if (connected) {
                assert(player.x == (direction > 0 ? 10 : 246));
                assert(player.y == 176 && player.mode == 1 && player.phase == 7);
            }
            next = (VKByte)room;
            player.x = direction > 0 ? 247 : 9;
            assert(!vk_room_try_edge(11, &next, &player, direction) && next == room);
            player.x = direction > 0 ? 248 : 8;
            player.on_stairs = 1;
            assert(!vk_room_try_edge(11, &next, &player, direction) && next == room);
        }
        for (i = vk_fixed_object_first[11][room]; i < vk_fixed_object_first[11][room+1]; i++) fixed++;
    }
    assert(candles == 25 && count15 == 4 && fixed == 1);
    assert(vk_candle_first[11][0] == 250 && vk_candle_first[11][6] == 275);
    assert(vk_fixed_object_first[11][1] == 70 && vk_fixed_object_first[11][2] == 71);
    assert(vk_fixed_object_defs[70].kind == 0x17 && vk_fixed_object_defs[70].x == 128 && vk_fixed_object_defs[70].y == 64);
    assert(vk_candle_defs[vk_candle_first[11][1]].x == 96 && vk_candle_defs[vk_candle_first[11][1]].y == 80 &&
           vk_candle_defs[vk_candle_first[11][1]].kind == 0x18 && vk_candle_defs[vk_candle_first[11][1]].mode == 3);
    assert(vk_wall_items[11][1][0] == 0x0E && vk_wall_items[11][3][0] == 0x16);
    assert(vk_fixed_enemies[11][1][0].raw_type == 0x0D && vk_fixed_enemies[11][1][0].x == 144 && vk_fixed_enemies[11][1][0].y == 80);
    assert(vk_fixed_enemies[11][2][0].raw_type == 0x0D && vk_fixed_enemies[11][2][0].x == 112 && vk_fixed_enemies[11][2][0].y == 112);
    assert(vk_fixed_enemies[11][5][0].raw_type == 0x0E && vk_fixed_enemies[11][5][0].x == 208 && vk_fixed_enemies[11][5][0].y == 112);
    assert(kStageItems[10].key_room == 1 && kStageItems[10].key_x == 96 && kStageItems[10].key_y == 80 && kStageItems[10].key_hidden);
    assert(kStageItems[10].door_room == 5 && kStageItems[10].door_x == 236 && kStageItems[10].door_y == 128);
    assert(kStageEntries[10].x == 16 && kStageEntries[10].y == 176 && kStageEntries[10].facing == 0);
    puts("Stage 11: 6 ROM rooms, 12 edges, 25 candles, 4 object-15, key/door, 3 fixed enemies, no platforms PASS");
    return 0;
}
