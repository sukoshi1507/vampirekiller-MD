/* Fireball of the stage 3 boss: pass by pass against the ROM loop 9E31 (9E58, 99B9, 6445, 99E5) run in a Z80 emulator
 * (tools/gen_boss3_ball_vectors.py -> tests/boss3_ball_vectors.h): from the state 9F61 creates, the ball must stay alive
 * for the same number of passes and be at the same Y and X on its last pass (exit test 99E5: Y >= E4h, X >= F1h or X < 7). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../sgdk/src/boss.c"
#include "boss3_ball_vectors.h"

int main(void)
{
    int n, edges[4] = {0, 0, 0, 0};
    for (n = 0; n < BOSS3_BALL_ROWS; n++)
    {
        const int *r = boss3_ball_rows[n];
        VKBoss b;
        int alive = 0, ly, lx;
        memset(&b, 0, sizeof b);
        b.kind = 0;
        b.shot[0].active = 1;
        b.shot[0].y = (unsigned short)(r[0] | r[1] << 8);
        b.shot[0].x = (unsigned short)(r[2] | r[3] << 8);
        b.shot[0].vy = (short)r[4];
        b.shot[0].vx = (short)r[5];
        ly = HI(b.shot[0].y); lx = HI(b.shot[0].x);
        for (;;)
        {
            shots_pass(&b);
            if (!b.shot[0].active) break;
            alive++; ly = HI(b.shot[0].y); lx = HI(b.shot[0].x);
            assert(alive < 1000);
        }
        if (alive != r[6] || ly != r[7] || lx != r[8])
            printf("row %d: alive %d (ROM %d), last Y %d (ROM %d), last X %d (ROM %d)\n", n, alive, r[6], ly, r[7], lx, r[8]);
        assert(alive == r[6] && ly == r[7] && lx == r[8]);
        if (r[7] == 0) edges[0]++; else if (r[8] < 16) edges[1]++; else if (r[8] > 200) edges[2]++; else edges[3]++;
    }
    /* The vectors reach the top (wrap), the left, the right and the bottom. */
    assert(edges[0] && edges[1] && edges[2] && edges[3]);
    printf("stage 3 boss ball: %d ROM trajectories (top %d, left %d, right %d, bottom %d) PASS\n", BOSS3_BALL_ROWS, edges[0], edges[1], edges[2], edges[3]);
    return 0;
}
