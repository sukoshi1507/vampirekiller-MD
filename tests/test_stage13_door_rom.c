/* Stage 13 RAW door table validation only, no gameplay assertions. */
#include <assert.h>
#include <stdio.h>
#include "generated/stage13_door_rom.h"
int main(void) {
    assert(VK_STAGE13_DOOR_RAW_FLAGS == 0x88);
    assert(VK_STAGE13_DOOR_ROOM == 8);
    assert(VK_STAGE13_DOOR_X == 12);
    assert(VK_STAGE13_DOOR_Y == 128);
    puts("stage13 raw door table: PASS (opening/key unverified)");
    return 0;
}
