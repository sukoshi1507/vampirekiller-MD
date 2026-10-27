/* Native simulation of sgdk/src/player.c, one call per VBLANK (main.c loop).
 * Flat floor: rows 20..23 solid (feet at Y 176, as measured on MSX), no walls. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "player.h"

static VKByte map[768];

/* argv: scenario frames jump_frame dir_plan
 * still : direction 0, press jump at jump_frame.
 * right : direction +1 from frame 0, press jump at jump_frame.
 * left  : start at X 64 (after 48 right frames), direction -1 from frame 48, press at jump_frame. */
int main(int argc, char **argv)
{
    const char *scen = argc > 1 ? argv[1] : "still";
    const int frames = argc > 2 ? atoi(argv[2]) : 60;
    const int jump_frame = argc > 3 ? atoi(argv[3]) : 0;
    VKPlayer p;
    int f;
    memset(map, 0, sizeof map);
    for (int r = 20; r < 24; r++)
        for (int c = 0; c < 32; c++) map[r * 32 + c] = 1;
    memset(&p, 0, sizeof p);
    p.x = 16; p.y = 176;
    printf("frame,y,x,mode,phase,pose,clock\n");
    for (f = 0; f < frames; f++)
    {
        int dir = 0;
        int press = (f == jump_frame);
        if (!strcmp(scen, "right")) dir = 1;
        if (!strcmp(scen, "left")) { if (f < 48) dir = 1; else dir = -1; }
        if (!strcmp(scen, "left") && f == 0) p.x = 16;
        vk_player_tick_map(&p, map, 1, dir, press);
        printf("%d,%d,%d,%d,%d,%d,%d\n", f, p.y, p.x, p.mode, p.phase, p.pose, p.clock);
    }
    return 0;
}
