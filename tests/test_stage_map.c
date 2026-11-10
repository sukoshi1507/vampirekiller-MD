#include <assert.h>
#include <string.h>
#include "stage_map.h"
#include "generated/stage_map_md.h"

const VKByte vk_courtyard_maps[3][768] = {{0}};
const VKByte vk_courtyard_neighbors[3][4] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

static VKByte tiles[VK_MAP_BYTES];

static VKByte pixel(int x, int y)
{
    const VKByte b = tiles[(((y - VK_MAP_Y) >> 3) * VK_MAP_COLS + ((x - VK_MAP_X) >> 3)) * 32 +
                           ((y - VK_MAP_Y) & 7) * 4 + (((x - VK_MAP_X) & 7) >> 1)];
    return (x - VK_MAP_X) & 1 ? (VKByte)(b & 15) : (VKByte)(b >> 4);
}

int main(void)
{
    VKCandles c;
    const VKByte *place = vk_map_place[vk_map_rooms[1][0]];
    memset(&c, 0, sizeof c);
    /* The ROM tables: 8 rooms in the hall, exit arrow at (180, 78). */
    assert(vk_map_room_count[1] == 8 && vk_map_exit[1][0] == 78 && vk_map_exit[1][1] == 180);
    vk_stage_map_render(tiles, &c, 1, 0, 136, 192);
    assert(pixel(VK_MAP_X, VK_MAP_Y) == 3 && pixel(VK_MAP_X + VK_MAP_W - 1, VK_MAP_Y + VK_MAP_H - 1) == 3);
    assert(pixel(VK_MAP_X, VK_MAP_Y + VK_MAP_H) == 0);       /* under the 126 lines */
    /* 984B/498A: packed DX rounds down to an even pixel. Simon, red with opaque black corners (y = (192 - 64) / 8). */
    assert(pixel(place[1] + 16, place[0] + 16) == 0 && pixel(place[1] + 17, place[0] + 16) == 8);
    assert(pixel(place[1] + 18, place[0] + 17) == 8);
    /* 9874: the arrow points right. */
    assert(pixel(180 + 3, 78) == 8 && pixel(180, 78) == 0 && pixel(180 + 6, 78 + 3) == 8);
    return 0;
}
