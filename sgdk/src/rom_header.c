#include "genesis.h"

/* Mega Drive header at 0x100. The fixed-size fields hold no NUL terminator.
 * "SEGA MEGA DRIVE " is what the TMSS boot ROM looks for (SGDK's sega.s
 * writes "SEGA" to A14000 on consoles that have it). tools/md_rom_header.py
 * sets rom_end and the standard checksum after SGDK pads the ROM. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunterminated-string-initialization"

__attribute__((externally_visible))
const ROMHeader rom_header = {
    "SEGA MEGA DRIVE ",
    "(C)SKSH 2026.OCT",
    "VAMPIRE KILLER                                  ",
    "VAMPIRE KILLER                                  ",
    "GM 00000000-00",
    0x0000,                         /* checksum: tools/md_rom_header.py */
    "J               ",             /* 3-button joypad */
    0x00000000,
    0x00000000,                     /* rom_end: tools/md_rom_header.py */
    0x00FF0000,
    0x00FFFFFF,
    "  ",                           /* no backup RAM */
    0x2020,
    0x20202020,
    0x20202020,
    "            ",
    "MSX2 PORT, SGDK                         ",
    "JUE             "
};

#pragma GCC diagnostic pop
