#include <stdio.h>
#include <stdlib.h>
#include "psg_core.h"
#include "fm_core.h"
#include "generated/msx_fm_md.h"

/* Plays a music in FM mode through psg_core.c and prints, per 60 Hz frame, the YM2612
 * (register, value) pairs and the PSG bytes: "F <frame> <n> reg val ... | psg b b ...".
 * Usage: fm_core_dump ID FRAMES (ID = 0x80..0x8E, hex). Used by tests/test_convert_fm.py. */
int main(int argc, char **argv)
{
    static VKPsg p;
    static VKFm fm;
    VKPsgByte out[VK_PSG_OUT_MAX];
    int id = argc > 1 ? (int)strtol(argv[1], 0, 16) : 0x80;
    int frames = argc > 2 ? atoi(argv[2]) : 100;
    int f, i, n;
    vk_psg_core_reset(&p);
    vk_fm_core_reset(&fm);
    if (!vk_psg_core_music(&p, &vk_fm_noise[id - 0x80], (VKPsgByte)id)) return 1;
    vk_fm_core_start(&fm, &vk_fm_music[id - 0x80]);
    for (f = 0; f < frames; f++)
    {
        n = fm.run ? vk_fm_core_step(&fm, &p, out) : vk_psg_core_frame(&p, out);
        printf("F %d %d", f, fm.n);
        for (i = 0; i < fm.n; i++) printf(" %02X %02X", fm.out[2 * i], fm.out[2 * i + 1]);
        printf(" |");
        for (i = 0; i < n; i++) printf(" %02X", out[i]);
        printf("\n");
    }
    return 0;
}
