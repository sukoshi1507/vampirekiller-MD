#include "psg_player.h"

/* SGDK's PSG_PORT is the bus address of the PSG write port. */
#define PSG_WRITE(value) (*(volatile u8 *)PSG_PORT = (value))

/* The stream is written for 60 Hz. On PAL (50 Hz) the player advances 6 frames
 * every 5 VBLANKs so that the music keeps its real-time speed. */
static const u8 *cursor;
static bool playing;
static u16 pacing;

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
    pacing = 0;
}

void vk_psg_stop(void)
{
    playing = FALSE;
    silence();
}

static void psg_step(void)
{
    u8 count, i;
    count = *cursor++;
    if (count == 0xFF)
    {
        vk_psg_stop();
        return;
    }
    for (i = 0; i < count; i++) PSG_WRITE(cursor[i]);
    cursor += count;
}

void vk_psg_update(void)
{
    const u16 rate = IS_PAL_SYSTEM ? 50 : 60;
    if (!playing) return;
    pacing += 60;
    while (pacing >= rate && playing)
    {
        pacing -= rate;
        psg_step();
    }
}
