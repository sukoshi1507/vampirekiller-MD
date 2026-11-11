/* Port side of tools/verify_rom_markers.py (former PROVISIONAL markers). */
#include "health.h"
#include "items.h"
#include "candles.h"
#include "room_freeze.h"

int markers_bar_pixels(int energy) { return vk_health_bar_pixels(energy); }
int markers_door_fades(int stage) { return vk_items_door_fades(stage); }
/* A candle put out and its object taken, then the stage entered again. */
int markers_candles_reset(int stage, int room)
{
    static VKCandles c;
    vk_candles_begin(&c);
    VK_CANDLE_OUT(&c, stage, room) = 0xFF;
    VK_CANDLE_TAKEN(&c, stage, room) = 0xFF;
    vk_candles_reset_stage(&c, (VKByte)stage);
    return VK_CANDLE_OUT(&c, stage, room) | VK_CANDLE_TAKEN(&c, stage, room);
}
int markers_freeze(int stage, int room)
{
    const VKRoomFreezeTiming t = vk_room_freeze_timing((VKFreezeByte)stage, (VKFreezeByte)room);
    return t.frames << 8 | t.dark;
}
