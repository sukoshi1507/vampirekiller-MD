#include "psg_player.h"

/* SGDK's PSG_PORT is the bus address of the PSG write port. */
#define PSG_WRITE(value) (*(volatile u8 *)PSG_PORT = (value))

/* The streams are written for 60 Hz. On PAL (50 Hz) the player advances 6 frames
 * every 5 VBLANKs so that music and effects keep their real-time speed. */
VKPsg vk_psg;
static u16 pacing;

static void write_bytes(const VKPsgByte *bytes, int count)
{
    int i;
    for (i = 0; i < count; i++) PSG_WRITE(bytes[i]);
}

bool vk_psg_music(u8 id)
{
    if (id < 0x80 || id >= 0x80 + VK_MSX_MUSIC_SLOTS) return FALSE;
    return vk_psg_core_music(&vk_psg, &vk_msx_music[id - 0x80], id) != 0;
}

bool vk_psg_effect(u8 id)
{
    if (id == 0 || id >= VK_MSX_EFFECT_SLOTS) return FALSE;
    return vk_psg_core_effect(&vk_psg, &vk_msx_effects[id], id) != 0;
}

void vk_psg_play_stream(const VKSoundStream *stream)
{
    vk_psg_core_music(&vk_psg, stream, 0);
}

void vk_psg_stop(void)
{
    VKPsgByte bytes[4];
    vk_psg_core_stop(&vk_psg);
    write_bytes(bytes, vk_psg_core_silence(bytes));
}

void vk_psg_fade(void)
{
    vk_psg_core_fade(&vk_psg);
}

void vk_psg_pause(void)
{
    vk_psg_core_pause(&vk_psg, &vk_msx_pause);
}

void vk_psg_resume(void)
{
    vk_psg_core_resume(&vk_psg);
}

void vk_psg_update(void)
{
    const u16 rate = IS_PAL_SYSTEM ? 50 : 60;
    VKPsgByte bytes[VK_PSG_OUT_MAX];
    pacing += 60;
    while (pacing >= rate)
    {
        pacing -= rate;
        write_bytes(bytes, vk_psg_core_frame(&vk_psg, bytes));
    }
}
