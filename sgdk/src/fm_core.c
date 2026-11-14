#include "fm_core.h"
#include "generated/msx_fm_md.h"

/* YM2612 registers of part 0 (see fm_core.h). */
#define FM_KEY 0x28
#define FM_TL_FIRST 0x40
#define FM_TL_LAST 0x4F
#define FM_VOICE_C 2

/* The PSG core state before the frame, as the frame sees it. */
typedef struct {
    VKPsgByte pause_edge, timer_edge, effect_new, fade_timer, fade_steps;
    VKPsgByte music_playing, effect_playing, c_muted, paused, timed;
} Pre;

static void raw(VKFm *f, VKPsgByte reg, VKPsgByte value)
{
    f->out[2 * f->n] = reg;
    f->out[2 * f->n + 1] = value;
    f->n++;
}

static VKPsgByte faded_tl(const VKPsg *p, VKPsgByte tl)
{
    unsigned v = (unsigned)tl + p->fade_steps * VK_FM_FADE_TL;
    return (VKPsgByte)(v > 127 ? 127 : v);
}

/* Voice C is muted: an effect plays, or it waits for its next note. */
static int muted(int voice_c_muted, VKPsgByte voice)
{
    return voice == FM_VOICE_C && voice_c_muted;
}

static void key_off_all(VKFm *f, int voice_c_muted)
{
    VKPsgByte v;
    for (v = 0; v < 3; v++)
        if (!muted(voice_c_muted, v)) raw(f, FM_KEY, v);
}

/* Held notes keyed on again (FE, FC): frequency and levels are still in the chip. */
static void key_on_held(VKFm *f, int voice_c_muted)
{
    VKPsgByte v;
    for (v = 0; v < 3; v++)
        if (f->keyed[v] && !muted(voice_c_muted, v)) raw(f, FM_KEY, (VKPsgByte)(0xF0 | v));
}

/* The carrier levels again, after a fade step. */
static void levels(VKFm *f, const VKPsg *p, int voice_c_muted)
{
    VKPsgByte r;
    for (r = 0; r < 16; r++)
        if (f->tl[r] != VK_FM_UNSET && !muted(voice_c_muted, r & 3))
            raw(f, (VKPsgByte)(FM_TL_FIRST + r), faded_tl(p, f->tl[r]));
}

/* One register write of the stream: tracked even when the voice is muted, written only when
 * it is not; carrier levels go out with the fade. */
static void stream_write(VKFm *f, const VKPsg *p, int voice_c_muted, VKPsgByte reg, VKPsgByte value)
{
    const VKPsgByte voice = reg == FM_KEY ? (VKPsgByte)(value & 3) : (VKPsgByte)(reg & 3);
    if (reg == FM_KEY) f->keyed[voice] = (value & 0xF0) ? 1 : 0;
    else if (reg >= FM_TL_FIRST && reg <= FM_TL_LAST)
    {
        f->tl[reg & 15] = value;
        value = faded_tl(p, value);
    }
    if (!muted(voice_c_muted, voice)) raw(f, reg, value);
}

void vk_fm_core_reset(VKFm *f)
{
    VKPsgByte *bytes = (VKPsgByte *)f;
    unsigned i;
    for (i = 0; i < sizeof(*f); i++) bytes[i] = 0;
}

void vk_fm_core_start(VKFm *f, const VKSoundStream *fm)
{
    unsigned i;
    if (!fm || !fm->data) return;
    f->stream = fm;
    f->cursor = fm->data;
    f->active = 1;
    f->edge = 1;
    f->run = 1;
    for (i = 0; i < 3; i++) f->keyed[i] = 0;
    for (i = 0; i < 16; i++) f->tl[i] = VK_FM_UNSET;
}

void vk_fm_core_cancel(VKFm *f)
{
    f->active = 0;
    f->edge = 0;
    f->run = 0;
}

void vk_fm_core_init(VKFm *f)
{
    int i;
    if (f->inited) return;
    f->inited = 1;
    for (i = 0; i < VK_FM_INIT_PAIRS; i++) raw(f, vk_fm_init[2 * i], vk_fm_init[2 * i + 1]);
}

int vk_fm_core_silence(VKPsgByte *out)
{
    out[0] = FM_KEY; out[1] = 0;
    out[2] = FM_KEY; out[3] = 1;
    out[4] = FM_KEY; out[5] = 2;
    return 6;
}

/* One frame of the FM stream, in step with the PSG music frame (same frames, same loop frame). */
static void music_frame(VKFm *f, const VKPsg *p, int voice_c_muted)
{
    const VKPsgByte *c = f->cursor;
    int n, i;
    if (*c == VK_PSG_FRAME_END) c = f->stream->data + f->stream->loop;
    n = *c++;
    for (i = 0; i < n; i++, c += 2) stream_write(f, p, voice_c_muted, c[0], c[1]);
    f->cursor = c;
}

int vk_fm_core_step(VKFm *f, VKPsg *p, VKPsgByte *psg_out)
{
    Pre pre;
    int n, muted_now, fade_end = 0;
    f->n = 0;
    if (f->edge)
    {
        /* A music starts: instrument registers (first time), every voice keyed off. */
        VKPsgByte v;
        f->edge = 0;
        vk_fm_core_init(f);
        for (v = 0; v < 3; v++) raw(f, FM_KEY, v);
    }
    pre.pause_edge = p->pause_edge;
    pre.timer_edge = p->timer_edge;
    pre.effect_new = p->effect_new;
    pre.fade_timer = p->fade_timer;
    pre.fade_steps = p->fade_steps;
    pre.music_playing = p->music.playing;
    pre.effect_playing = p->effect.playing;
    pre.c_muted = p->c_muted;
    pre.paused = p->paused;
    pre.timed = p->timed;
    n = vk_psg_core_fm_frame(p, psg_out);
    f->run = f->active;
    if (!f->active) return n;
    muted_now = pre.effect_playing || pre.c_muted;            /* psg_core.c music_mask() */
    /* 8964 order, as vk_psg_core_frame: pause edges, then only the pause voice. */
    if (pre.pause_edge == 1) key_off_all(f, 0);
    else if (pre.pause_edge == 2 && pre.music_playing && !pre.timed) key_on_held(f, muted_now);
    if (pre.paused) return n;
    if (pre.timer_edge == 1) key_off_all(f, 0);
    else if (pre.timer_edge == 2 && pre.music_playing) key_on_held(f, muted_now);
    if (!pre.timed && pre.music_playing)
    {
        if (pre.fade_timer == 1)
        {
            if (pre.fade_steps + 1 >= VK_PSG_FADE_STEPS)
            {
                key_off_all(f, muted_now);
                f->active = 0;
                fade_end = 1;
            }
            else levels(f, p, muted_now);
        }
        if (!fade_end)
        {
            if (!p->music.playing)
            {
                /* Every channel reached FF (no loop): every voice keyed off. */
                key_off_all(f, muted_now);
                f->active = 0;
            }
            else
            {
                /* The first note of voice C after an effect (NOTE_C) cleared c_muted before the frame's
                 * writes; the effect itself plays on until its own frame. */
                const int unmuted = pre.c_muted && !p->c_muted;
                music_frame(f, p, pre.effect_playing || (pre.c_muted && !unmuted));
            }
        }
    }
    /* The effect takes voice C: what the music left there stops now (the effect's first frame). */
    if (pre.effect_new && f->active) raw(f, FM_KEY, FM_VOICE_C);
    f->run = f->active;
    return n;
}
