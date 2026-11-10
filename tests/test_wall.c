#include <stddef.h>
/* Native tests of wall.c: wall objects and merchant (research/merchant.md). */
#include <assert.h>
#include <stdio.h>
#include "wall.h"
_Static_assert(sizeof(VKWall)==693 && offsetof(VKWall,shop)==348,"LOT08 wall offsets must not follow candle stage count");
#include "enemies.h"

/* Courtyard tables of the intro generator, not linked in native tests. */
const VKByte vk_courtyard_maps[3][768] = {{0}};
const VKByte vk_courtyard_neighbors[3][4] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

static VKCandles cand;
static VKWall wall;

/* Runs passes until the animation of record i is over; returns the events of the
 * last step. */
static VKWallEvents settle(int i)
{
    VKWallEvents ev;
    int n = 0;
    do { vk_wall_step(&wall, &cand, &ev); n++; } while (wall.obj[i].state == VK_WALL_ANIM && n < 200);
    assert(n < 200);
    return ev;
}

static VKWallEvents hit(int i)
{
    VKWallEvents ev;
    vk_wall_hit(&wall, i);
    vk_wall_step(&wall, &cand, &ev);        /* 91F2 at once */
    assert(wall.obj[i].state == VK_WALL_ANIM && wall.obj[i].timer == VK_WALL_ANIM_PASSES);
    return settle(i);
}

static void hall_merchant(void)
{
    VKWallEvents ev;
    int i, events, n;
    const VKCandleDef block = { 192, 160, 3, 0x1F, 0xE1 };
    vk_candles_begin(&cand);
    vk_wall_begin(&wall);
    cand.stage = 1; cand.room = 7; cand.count = 1;
    vk_wall_enter(&wall, 1, 7);
    assert(!wall.obj[0].state && !wall.obj[1].state);
    /* run A, frame 4627: record 01 A0 C0 .. 08 01, shown at once (82h, slot 0, frame 3). */
    i = vk_wall_block(&wall, 0, &block);
    assert(i == 0 && wall.obj[0].state == VK_WALL_SHOWN && wall.obj[0].x == 0xC0 && wall.obj[0].y == 0xA0);
    assert(wall.obj[0].row == 1 && vk_wall_frame(&wall, 0) == 3);
    /* first hit: C70C = 0, C70B = 1 (the red merchant), then 9327. */
    ev = hit(0);
    assert(wall.c70c == VK_WALL_MERCHANT && wall.obj[0].frame == 1 && ev.shop && wall.shop == VK_SHOP_OPEN);
    assert(wall.count[1][7][0] == 1);
    cand.hearts = 99;
    vk_shop_step(&wall, &cand, 0, &ev);
    assert(ev.sounds == 1 && ev.sound[0] == 0x19 && wall.sale == 0x1B && wall.price == 0x50);
    assert(wall.count[1][7][0] == 0x81 && wall.timeout == 0x14);
    {
        VKByte x, y;
        vk_shop_window(&wall, &x, &y);
        assert(x == 0xA0 && y == 0x88);
    }
    vk_shop_step(&wall, &cand, 1, &ev);       /* yes */
    assert(cand.hearts == 49 && cand.weapon == 2 && ev.sounds == 1 && ev.sound[0] == 0x12);
    assert(wall.count[1][7][0] == 0x01 && wall.bought);
    for (n = 0; wall.shop != VK_SHOP_NONE; n++) events = vk_shop_step(&wall, &cand, 1, &ev);
    (void)events;
    assert(n == VK_SHOP_CLOSE_PASSES);
    /* hits 2..8 of row 1: 3 3 1 5 5 5 6 (run A: hearts 54, 59, D012 = 1, 5000 points). */
    ev = hit(0); assert(wall.c70c == 3 && cand.hearts == 54 && ev.sound[0] == 0x0F);
    ev = hit(0); assert(wall.c70c == 3 && cand.hearts == 59);
    ev = hit(0); assert(wall.c70c == 1 && wall.d012 == 1);
    ev = hit(0); assert(wall.c70c == 5);
    ev = hit(0); assert(wall.c70c == 5);
    ev = hit(0); assert(wall.c70c == 5);
    ev = hit(0);
    assert(wall.c70c == 6 && !wall.obj[0].state && ev.points && ev.sound[0] == 0x10 && ev.block_gone == 1);
    assert(cand.out[1][7] & 1);
}

static void refuse_and_return(void)
{
    VKWallEvents ev;
    const VKCandleDef block = { 192, 160, 3, 0x1F, 0xE1 };
    vk_candles_begin(&cand);
    vk_wall_begin(&wall);
    vk_wall_enter(&wall, 1, 7);
    vk_wall_block(&wall, 0, &block);
    hit(0);
    cand.hearts = 10;
    vk_shop_step(&wall, &cand, 0, &ev);
    vk_shop_step(&wall, &cand, 1, &ev);       /* not enough hearts: effect 02 */
    assert(ev.sound[0] == 0x02 && cand.hearts == 10 && wall.count[1][7][0] == 0x81);
    while (wall.shop != VK_SHOP_NONE) vk_shop_step(&wall, &cand, 0, &ev);
    /* the room again: the block holds it again; 924C: count - 1 (80h), so the next
     * hit counts 1 again: the merchant. */
    vk_wall_enter(&wall, 1, 7);
    vk_wall_block(&wall, 0, &block);
    assert(wall.count[1][7][0] == 0x80);
    hit(0);
    assert(wall.c70c == VK_WALL_MERCHANT);
    /* timeout: C706 = 14h steps of 32 passes. */
    vk_shop_step(&wall, &cand, 0, &ev);
    {
        int n = 0;
        while (wall.shop == VK_SHOP_WAIT) { vk_shop_step(&wall, &cand, 0, &ev); n++; }
        assert(ev.sound[0] == 0x02 && n > 0x13 * 32 && n <= 0x14 * 32);
    }
}

static void stage2(void)
{
    VKWallEvents ev;
    vk_candles_begin(&cand);
    vk_wall_begin(&wall);
    vk_wall_enter(&wall, 2, 0);
    /* run K3: a white old man at (160, 32) on arrival, slot 0 (object 16). */
    assert(wall.obj[0].state == VK_WALL_SHOWN && wall.obj[0].x == 160 && wall.obj[0].y == 32);
    assert(wall.obj[1].state == 0);
    hit(0);
    assert(wall.c70c == VK_WALL_MERCHANT);
    vk_shop_step(&wall, &cand, 0, &ev);
    assert(wall.sale == 0x16 && wall.price == 0x40);
    cand.c702 = 0x80;
    vk_shop_step(&wall, &cand, 2, &ev);       /* no */
    assert(ev.sound[0] == 0x02);
    /* object 05: removed for 5000 points, it does not come back. */
    while (wall.shop != VK_SHOP_NONE) vk_shop_step(&wall, &cand, 0, &ev);
    vk_wall_cross(&wall);
    vk_wall_step(&wall, &cand, &ev);
    ev = settle(0);
    assert(ev.points && !wall.obj[0].state && wall.gone[2][0] == 1);
    vk_wall_enter(&wall, 2, 0);
    assert(!wall.obj[0].state);
    vk_wall_reset_stage(&wall, 2);
    vk_wall_enter(&wall, 2, 0);
    assert(wall.obj[0].state == VK_WALL_SHOWN && wall.count[2][0][0] == 0);
    /* row 2 (block CE, slot 1): hits 1 and 2 are random 9 (3 or 4). */
    {
        const VKCandleDef block = { 192, 160, 3, 0x1F, 0xCE };
        assert(vk_wall_block(&wall, 3, &block) == 1);
        cand.hearts = 20;
        /* 92E8: bit 0 of R set gives H (3), clear gives L (4); forced through Enemies_s7R, then the free generator draws both. */
        Enemies_s7R = 0x55; hit(1);
        assert(wall.c70c == 3);
        wall.count[2][0][1] = 0;
        wall.phase = 0; Enemies_s7R = 0x2A; hit(1);
        assert(wall.c70c == 4);
        Enemies_s7R = -1;
        wall.count[2][0][1] = 0;
        {
            int seen3 = 0, seen4 = 0, k;
            for (k = 0; k < 64; k++) { wall.count[2][0][1] = 0; wall.phase = 0; hit(1); if (wall.c70c == 3) seen3++; else if (wall.c70c == 4) seen4++; }
            assert(seen3 + seen4 == 64 && seen3 > 16 && seen4 > 16);   /* both outcomes drawn, about half each (research/hasard-R.md) */
        }
        wall.count[2][0][1] = 1; cand.hearts = 20; wall.phase = 0;
        cand.c702 = 0x80;
        hit(1); hit(1);
        assert(wall.c70c == 0);
        vk_shop_step(&wall, &cand, 0, &ev);
        assert(wall.sale == 0x04 && wall.price == 0x10);
    }
}

int main(void)
{
    hall_merchant();
    refuse_and_return();
    stage2();
    puts("wall: PASS");
    return 0;
}
