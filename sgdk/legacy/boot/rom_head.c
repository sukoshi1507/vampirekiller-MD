#include "genesis.h"

__attribute__((externally_visible))
const ROMHeader rom_header = {
#if (ENABLE_BANK_SWITCH != 0)
    "SEGA SSF        ",
#elif (MODULE_MEGAWIFI != 0)
    "SEGA MEGAWIFI   ",
#else
    "SEGA MEGA DRIVE ",
#endif
    "(C)2026 HOMEBREW",
    "VK MSX2 ROOM EXPLORER                           ",
    "VK MSX2 ROOM EXPLORER                           ",
    "GM 00000000-00",
    0x000,
    "JD              ",
    0x00000000,
#if (ENABLE_BANK_SWITCH != 0)
    0x003FFFFF,
#else
    0x0007FFFF,
#endif
    0xE0FF0000,
    0xE0FFFFFF,
    "  ",
    0x0000,
    0x00000000,
    0x00000000,
    "            ",
    "STATIC BACKGROUNDS - NO GAMEPLAY        ",
    "JUE             "
};
