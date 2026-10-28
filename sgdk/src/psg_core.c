#include "psg_core.h"

/* SN76489 bytes: 1 cc t dddd latches channel cc (t = 1: volume), a byte with
 * bit 7 clear carries the high bits of the last tone latch. */
#define SN_LATCH 0x80
#define SN_VOLUME 0x10
#define SN_SILENT 0x0F
#define SN_CHANNEL(b) (((b) >> 5) & 3)

/* Effect channels: SN tone 2 (AY channel C) and the noise (bit per SN channel). */
#define EFFECT_CHANNELS 0x0C

static const VKPsgByte silence_bytes[4] = {0x9F, 0xBF, 0xDF, 0xFF};

void vk_psg_core_reset(VKPsg *p)
{
    VKPsgByte *bytes = (VKPsgByte *)p;
    unsigned i;
    for (i = 0; i < sizeof(*p); i++) bytes[i] = 0;
}

int vk_psg_core_silence(VKPsgByte *out)
{
    int i;
    for (i = 0; i < 4; i++) out[i] = silence_bytes[i];
    return 4;
}

static void voice_start(VKPsgVoice *v, const VKSoundStream *s)
{
    v->stream = s;
    v->cursor = s->data;
    v->playing = 1;
}

int vk_psg_core_music(VKPsg *p, const VKSoundStream *s, VKPsgByte id)
{
    if (!s || !s->data) return 0;
    /* 5118-5136: the effect channel is parked, C096 = 0, C0A8 = 0, new music. */
    p->effect.playing = 0;
    p->effect_id = 0;
    p->effect_new = 0;
    p->lock = 0;
    p->c_muted = 0;
    p->fade_steps = 0;
    p->fade_timer = 0;
    p->noise = 0;
    p->music_id = id;
    voice_start(&p->music, s);
    return 1;
}

int vk_psg_core_effect(VKPsg *p, const VKSoundStream *s, VKPsgByte id)
{
    if (!s || !s->data) return 0;
    if (p->lock) return 0;                                  /* 516B: C0A8 */
    if (p->effect.playing && p->effect_id > id) return 0;   /* 5172-5179 */
    p->effect_id = id;
    p->last_effect = id;
    p->effect_count++;
    p->effect_new = 1;
    p->c_muted = 1;                                         /* 8A85 */
    voice_start(&p->effect, s);
    return 1;
}

void vk_psg_core_stop(VKPsg *p)
{
    /* 5085: C096, C098, C0A8 = 0, every channel parked. */
    p->music.playing = 0;
    p->effect.playing = 0;
    p->music_id = 0;
    p->effect_id = 0;
    p->effect_new = 0;
    p->lock = 0;
    p->c_muted = 0;
    p->fade_steps = 0;
    p->fade_timer = 0;
}

void vk_psg_core_fade(VKPsg *p)
{
    /* 51BC: C0A5 = 3A; 897A then lowers C0A6 by one every 3A frames. */
    if (p->music.playing && !p->fade_timer) p->fade_timer = VK_PSG_FADE_FRAMES;
}

/* Music volume byte with the fade: one AY step (3 dB) is 1.5 SN steps (2 dB). */
static VKPsgByte faded(const VKPsg *p, VKPsgByte b)
{
    unsigned att = (unsigned)(b & SN_SILENT) + (p->fade_steps * 3u + 1u) / 2u;
    if (att > SN_SILENT) att = SN_SILENT;
    return (VKPsgByte)((b & 0xF0) | att);
}

static int music_mask(const VKPsg *p)
{
    return (p->effect.playing || p->c_muted) ? EFFECT_CHANNELS : 0;
}

/* Music channel state back on the PSG (channel C after an effect, or a fade step). */
static int restore(const VKPsg *p, VKPsgByte *out, int channels)
{
    int n = 0, ch;
    for (ch = 0; ch < 4; ch++)
    {
        if (!(channels & (1 << ch))) continue;
        if (ch < 3 && p->tone[ch][0]) { out[n++] = p->tone[ch][0]; out[n++] = p->tone[ch][1]; }
        if (ch == 3 && p->noise) out[n++] = p->noise;
        if (p->volume[ch]) out[n++] = faded(p, p->volume[ch]);
    }
    return n;
}

static int music_frame(VKPsg *p, VKPsgByte *out)
{
    VKPsgVoice *v = &p->music;
    VKPsgByte header = *v->cursor;
    int n = 0, i, count, mask;
    if (header == VK_PSG_FRAME_END)
    {
        if (v->stream->loop == VK_PSG_NO_LOOP)
        {
            /* Every channel reached FF: 89CE writes volume 0 (8A0B). */
            v->playing = 0;
            p->music_id = 0;
            mask = music_mask(p);
            for (i = 0; i < 4; i++)
                if (!(mask & (1 << i))) out[n++] = silence_bytes[i];
            return n;
        }
        v->cursor = v->stream->data + v->stream->loop;
        header = *v->cursor;
    }
    v->cursor++;
    p->lock = (header & VK_PSG_LOCK) ? 1 : 0;
    /* 8B00: the first note on channel C after the effect ends the mute. */
    if ((header & VK_PSG_NOTE_C) && p->c_muted && !p->effect.playing)
    {
        p->c_muted = 0;
        n += restore(p, out + n, EFFECT_CHANNELS);
    }
    mask = music_mask(p);
    count = header & VK_PSG_COUNT_MASK;
    for (i = 0; i < count; i++)
    {
        const VKPsgByte b = v->cursor[i];
        int ch;
        if (b & SN_LATCH)
        {
            ch = SN_CHANNEL(b);
            if (b & SN_VOLUME) p->volume[ch] = b;
            else if (ch == 3) p->noise = b;
            else { p->tone[ch][0] = b; p->pending = (VKPsgByte)ch; }
        }
        else
        {
            ch = p->pending;
            p->tone[ch][1] = b;
        }
        if (mask & (1 << ch)) continue;
        out[n++] = ((b & (SN_LATCH | SN_VOLUME)) == (SN_LATCH | SN_VOLUME)) ? faded(p, b) : b;
    }
    v->cursor += count;
    return n;
}

static int effect_frame(VKPsg *p, VKPsgByte *out)
{
    VKPsgVoice *v = &p->effect;
    VKPsgByte header = *v->cursor;
    int n = 0, count, i;
    if (p->effect_new)
    {
        /* The effect takes channel C: what music left there stops now. */
        p->effect_new = 0;
        out[n++] = silence_bytes[2];
        out[n++] = silence_bytes[3];
    }
    if (header == VK_PSG_FRAME_END)
    {
        /* 89CE-89EE: C096 = 0, volume 0 on channel C; the music channel C stays
         * muted until its next note (c_muted). */
        v->playing = 0;
        p->effect_id = 0;
        out[n++] = silence_bytes[2];
        out[n++] = silence_bytes[3];
        if (!p->music.playing) p->c_muted = 0;
        return n;
    }
    count = header & VK_PSG_COUNT_MASK;
    for (i = 0; i < count; i++) out[n++] = v->cursor[1 + i];
    v->cursor += 1 + count;
    return n;
}

int vk_psg_core_frame(VKPsg *p, VKPsgByte *out)
{
    int n = 0;
    /* 8964 order: the three music channels, then the effect channel. */
    if (p->music.playing)
    {
        if (p->fade_timer && --p->fade_timer == 0)
        {
            if (++p->fade_steps >= VK_PSG_FADE_STEPS)
            {
                /* 8990: C0A6 reached F0, the music channels are parked. */
                const int mask = music_mask(p);
                int i;
                p->music.playing = 0;
                p->music_id = 0;
                p->fade_steps = 0;
                for (i = 0; i < 4; i++)
                    if (!(mask & (1 << i))) out[n++] = silence_bytes[i];
            }
            else
            {
                p->fade_timer = VK_PSG_FADE_FRAMES;
                n += restore(p, out + n, 0x0F & ~music_mask(p));
            }
        }
        if (p->music.playing) n += music_frame(p, out + n);
    }
    if (p->effect.playing) n += effect_frame(p, out + n);
    return n;
}
