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
VKFm vk_fm;
/* FM music mode (research/audio-fm.md), chosen on the title screen: 0 = the three music
 * voices on the PSG (default), 1 = on YM2612 channels 1 to 3, the PSG keeping the drums. */
volatile u8 vk_sound_fm;
static u16 pacing;
/* VBLANK rate, read once: the interrupt must not read the VDP status port while
 * the main loop is in the middle of a VDP command. */
static u16 rate = 60;

static void write_bytes(const VKPsgByte *bytes, int count)
{
    int i;
    for (i = 0; i < count; i++) PSG_WRITE(bytes[i]);
}

/* YM2612 part 0 from the 68000. SGDK's default driver (the PCM driver loaded in main) keeps
 * running on the Z80: it writes the DAC (register 2A) through the YM2612 address latch it
 * set once and never rewrites between samples. So the Z80 is halted for the writes
 * (Z80_getAndRequestBus: it leaves the bus alone if the main loop holds it), and the latch
 * is put back on 2A before the bus goes back, or the next DAC sample would land in the
 * register we wrote last. Channel 6 and register 2B (DAC enable) are never written here. */
#define YM_DAC_REGISTER 0x2A
static void fm_write(const VKPsgByte *pairs, int count)
{
    bool taken;
    int i;
    if (count <= 0) return;
    taken = Z80_getAndRequestBus(TRUE);
    for (i = 0; i < count; i++) YM2612_writeReg(0, pairs[2 * i], pairs[2 * i + 1]);
    YM2612_write(0, YM_DAC_REGISTER);
    if (!taken) Z80_releaseBus();
}

/* The voices of the FM music keyed off now, and the FM stream dropped (a PSG music, a stop). */
static void fm_silence(void)
{
    VKPsgByte keys[6];
    fm_write(keys, vk_fm_core_silence(keys) / 2);
}

static void fm_cancel(void)
{
    vk_fm_core_cancel(&vk_fm);
    fm_silence();
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

/* V-Int callback of the FM mode (vk_sound_fm = 1): the same player with the FM stream in step.
 * A separate function installed by vk_psg_set_fm, so that the PSG mode runs psg_vint exactly as
 * it was before FM existed (no test, no extra register): the replay of tests/md_playthrough
 * breaks for a few dozen cycles more in the interrupt. */

static void fm_vint(void)
{
    VKPsgByte bytes[VK_PSG_OUT_MAX];
    pacing += 60;
    while (pacing >= rate)
    {
        pacing -= rate;
        if (vk_fm.run)
        {
            write_bytes(bytes, vk_fm_core_step(&vk_fm, &vk_psg, bytes));
            fm_write(vk_fm.out, vk_fm.n);
        }
        else write_bytes(bytes, vk_psg_core_fm_frame(&vk_psg, bytes));
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
volatile u16 vk_psg_cost_pairs[32];   /* FM mode: longest step (HV steps) per count of YM2612 pairs, 31 = 31 or more */
volatile u16 vk_psg_cost_pairs_n[32]; /* steps seen per count of pairs */

/* H32 counter: 00..93, then E9..FF in the horizontal blanking. */
static u16 h32_step(u16 hv)
{
    const u16 h = hv & 0xFF;
    return h <= 0x93 ? h : (u16)(h - 0xE9 + 0x94);
}

void vk_psg_measure(void)
{
    static VKPsg copy;
    static VKFm fm_copy;
    VKPsgByte bytes[VK_PSG_OUT_MAX];
    volatile u8 sink;
    u16 hv0, hv1, cost;
    int n, i;
    if (pacing + 60 < rate) return;          /* PAL: no step in this VBLANK */
    /* Out of the vertical blanking first (V counter E0..FF), where V is not linear. */
    while ((*(volatile u16 *)VDP_HVCOUNTER_PORT >> 8) >= 0xE0) ;
    SYS_disableInts();
    copy = vk_psg;
    fm_copy = vk_fm;
    if (!fm_copy.run) fm_copy.n = 0;
    hv0 = *(volatile u16 *)VDP_HVCOUNTER_PORT;
    n = fm_copy.run ? vk_fm_core_step(&fm_copy, &copy, bytes) : vk_psg_core_frame(&copy, bytes);
    for (i = 0; i < n; i++) sink = bytes[i];
    /* The YM2612 writes of the step, to the unused register 2F (no effect on the chip),
     * with the same bus request, busy waits and latch restore as the real step. */
    if (fm_copy.n)
    {
        const bool taken = Z80_getAndRequestBus(TRUE);
        for (i = 0; i < fm_copy.n; i++) YM2612_writeReg(0, 0x2F, fm_copy.out[2 * i + 1]);
        YM2612_write(0, YM_DAC_REGISTER);
        if (!taken) Z80_releaseBus();
    }
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
    i = fm_copy.n > 31 ? 31 : fm_copy.n;
    vk_psg_cost_pairs_n[i]++;
    if (cost > vk_psg_cost_pairs[i]) vk_psg_cost_pairs[i] = cost;
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
    if (vk_sound_fm && vk_fm_music[id - 0x80].data)
    {
        /* The drums on the PSG, the three voices on the YM2612 (fm_core.h). */
        taken = vk_psg_core_music(&vk_psg, &vk_fm_noise[id - 0x80], id);
        if (taken)
        {
            if (vk_fm.active) fm_silence();
            vk_fm_core_start(&vk_fm, &vk_fm_music[id - 0x80]);
        }
    }
    else
    {
        taken = vk_psg_core_music(&vk_psg, &vk_msx_music[id - 0x80], id);
        if (taken && vk_fm.run) fm_cancel();
    }
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
    if (vk_fm.run) fm_cancel();
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
    vk_fm_core_cancel(&vk_fm);
    if (vk_fm.inited) fm_silence();
    SYS_enableInts();
}

void vk_psg_set_fm(bool on)
{
    SYS_disableInts();
    vk_sound_fm = on ? 1 : 0;
    SYS_setVIntCallback(on ? fm_vint : psg_vint);
    if (on && !vk_fm.inited)
    {
        /* The instrument registers now, on the title screen, not at the first note. */
        vk_fm.n = 0;
        vk_fm_core_init(&vk_fm);
        fm_write(vk_fm.out, vk_fm.n);
        vk_fm.n = 0;
    }
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
