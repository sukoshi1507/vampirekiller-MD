#include "psg_player.h"

/* SGDK's PSG_PORT is the bus address of the PSG write port. */
#define PSG_WRITE(value) (*(volatile u8 *)PSG_PORT = (value))

static const u8 *cursor;
static bool playing;

static void silence(void)
{
    /* Latch volume 15 (silent) on the three tone channels and the noise channel. */
    PSG_WRITE(0x9F);
    PSG_WRITE(0xBF);
    PSG_WRITE(0xDF);
    PSG_WRITE(0xFF);
}

void vk_psg_play(const u8 *stream)
{
    cursor = stream;
    playing = TRUE;
}

void vk_psg_stop(void)
{
    playing = FALSE;
    silence();
}

void vk_psg_update(void)
{
    u8 count, i;
    if (!playing) return;
    count = *cursor++;
    if (count == 0xFF)
    {
        vk_psg_stop();
        return;
    }
    for (i = 0; i < count; i++) PSG_WRITE(cursor[i]);
    cursor += count;
}
