#include "psg_player.h"

/* SGDK's PSG_PORT is the bus address of the PSG write port. */
#define PSG_WRITE(value) (*(volatile u8 *)PSG_PORT = (value))

/* The player runs in the vertical interrupt, as the MSX driver runs in H.TIMI, so
 * the sound goes on while the main loop is busy (a room drawn over several
 * frames). The requests come from the main loop: each one changes vk_psg with the
 * interrupts off, so the interrupt never sees a half-made request. The PSG port
 * is a 68000 bus port (C00011), apart from the VDP ports and the Z80 bus: the
 * writes need no Z80 bus request and do not disturb the PCM driver, which only
 * uses the YM2612 DAC.
 *
 * The streams are written for 60 Hz. On PAL (50 Hz) the player advances 6 frames
 * every 5 VBLANKs so that music and effects keep their real-time speed. */
VKPsg vk_psg;
static u16 pacing;
/* VBLANK rate, read once: the interrupt must not read the VDP status port while
 * the main loop is in the middle of a VDP command. */
static u16 rate = 60;

static void write_bytes(const VKPsgByte *bytes, int count)
{
    int i;
    for (i = 0; i < count; i++) PSG_WRITE(bytes[i]);
}

/* V-Int callback: one VBLANK of the player. */
static void psg_vint(void)
{
    VKPsgByte bytes[VK_PSG_OUT_MAX];
    pacing += 60;
    while (pacing >= rate)
    {
        pacing -= rate;
        write_bytes(bytes, vk_psg_core_frame(&vk_psg, bytes));
    }
}

#ifdef VK_MEASURE_PSG
/* Measurement build only (EXTRA_FLAGS=-DVK_MEASURE_PSG, tools/measure_psg_cost.py):
 * the work of the next psg_vint (vk_psg_core_frame and the port writes) run on a copy
 * of vk_psg from the main loop, during the active display where the V counter is
 * linear, timed by the HV counter (C00008) in H32 steps (VDP_setScreenWidth256), 171 per
 * line of 3420 master clocks (488.6 68000 cycles, 2.86 a step). The interrupt entry of
 * SGDK is not counted. */
volatile u16 vk_psg_cost_last, vk_psg_cost_max, vk_psg_cost_bytes_max;
volatile u32 vk_psg_cost_sum;
volatile u16 vk_psg_cost_count;
volatile u16 vk_psg_cost_over;   /* steps longer than 5 lines (855) */

/* H32 counter: 00..93, then E9..FF in the horizontal blanking. */
static u16 h32_step(u16 hv)
{
    const u16 h = hv & 0xFF;
    return h <= 0x93 ? h : (u16)(h - 0xE9 + 0x94);
}

void vk_psg_measure(void)
{
    static VKPsg copy;
    VKPsgByte bytes[VK_PSG_OUT_MAX];
    volatile u8 sink;
    u16 hv0, hv1, cost;
    int n, i;
    if (pacing + 60 < rate) return;          /* PAL: no step in this VBLANK */
    /* Out of the vertical blanking first (V counter E0..FF), where V is not linear. */
    while ((*(volatile u16 *)VDP_HVCOUNTER_PORT >> 8) >= 0xE0) ;
    SYS_disableInts();
    copy = vk_psg;
    hv0 = *(volatile u16 *)VDP_HVCOUNTER_PORT;
    n = vk_psg_core_frame(&copy, bytes);
    for (i = 0; i < n; i++) sink = bytes[i];
    hv1 = *(volatile u16 *)VDP_HVCOUNTER_PORT;
    SYS_enableInts();
    (void)sink;
    if ((hv0 >> 8) >= 0xD0 || (hv1 >> 8) >= 0xE0 || (hv1 >> 8) < (hv0 >> 8)) return;
    cost = (u16)(((hv1 >> 8) - (hv0 >> 8)) * 171 + h32_step(hv1) - h32_step(hv0));
    vk_psg_cost_last = cost;
    if (cost > vk_psg_cost_max) { vk_psg_cost_max = cost; vk_psg_cost_bytes_max = (u16)n; }
    vk_psg_cost_sum += cost;
    vk_psg_cost_count++;
    if (cost > 5 * 171) vk_psg_cost_over++;
}
#endif

void vk_psg_init(void)
{
    rate = IS_PAL_SYSTEM ? 50 : 60;
    SYS_setVIntCallback(psg_vint);
}

bool vk_psg_music(u8 id)
{
    int taken;
    if (id < 0x80 || id >= 0x80 + VK_MSX_MUSIC_SLOTS) return FALSE;
    SYS_disableInts();
    taken = vk_psg_core_music(&vk_psg, &vk_msx_music[id - 0x80], id);
    SYS_enableInts();
    return taken != 0;
}

bool vk_psg_effect(u8 id)
{
    int taken;
    if (id == 0 || id >= VK_MSX_EFFECT_SLOTS) return FALSE;
    SYS_disableInts();
    taken = vk_psg_core_effect(&vk_psg, &vk_msx_effects[id], id);
    SYS_enableInts();
    return taken != 0;
}

void vk_psg_play_stream(const VKSoundStream *stream)
{
    SYS_disableInts();
    vk_psg_core_music(&vk_psg, stream, 0);
    SYS_enableInts();
}

/* The silence is written inside the same critical section: an interrupt between
 * the stop and the writes could otherwise split a latch/data pair. */
void vk_psg_stop(void)
{
    VKPsgByte bytes[4];
    SYS_disableInts();
    vk_psg_core_stop(&vk_psg);
    write_bytes(bytes, vk_psg_core_silence(bytes));
    SYS_enableInts();
}

void vk_psg_fade(void)
{
    SYS_disableInts();
    vk_psg_core_fade(&vk_psg);
    SYS_enableInts();
}

void vk_psg_pause(void)
{
    SYS_disableInts();
    vk_psg_core_pause(&vk_psg, &vk_msx_pause);
    SYS_enableInts();
}

void vk_psg_resume(void)
{
    SYS_disableInts();
    vk_psg_core_resume(&vk_psg);
    SYS_enableInts();
}

void vk_psg_timer(void)
{
    SYS_disableInts();
    vk_psg_core_timer(&vk_psg, &vk_msx_timed_item);
    SYS_enableInts();
}

void vk_psg_timer_end(void)
{
    SYS_disableInts();
    vk_psg_core_timer_end(&vk_psg);
    SYS_enableInts();
}
