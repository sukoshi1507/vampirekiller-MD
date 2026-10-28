#include <assert.h>
#include <string.h>
#include "psg_core.h"
#include "generated/msx_sound_md.h"

/* Music and effect sharing of psg_core.c, on hand-made streams and on the
 * generated MSX sounds. */

static int contains(const VKPsgByte *out, int n, VKPsgByte b)
{
    int i;
    for (i = 0; i < n; i++) if (out[i] == b) return 1;
    return 0;
}

/* Music: frame 0 sets tone 0 and 2 and their volumes, frame 1 a new tone 2
 * without note flag, frame 2 a channel C note (flag), then loop to frame 1. */
static const VKPsgByte music_data[] = {
    8, 0x81, 0x02, 0x90, 0xC3, 0x04, 0xD2, 0xE7, 0xF5,
    2, 0xC6, 0x07,
    VK_PSG_NOTE_C | 2, 0xC8, 0x09,
    VK_PSG_FRAME_END
};
static const VKSoundStream music = {music_data, 9};

/* Effect: two frames on tone 2 and the noise. */
static const VKPsgByte effect_data[] = {
    3, 0xCA, 0x0B, 0xD0,
    1, 0xF1,
    VK_PSG_FRAME_END
};
static const VKSoundStream effect = {effect_data, VK_PSG_NO_LOOP};

/* Music that locks the effects on its last frame, then ends. */
static const VKPsgByte jingle_data[] = {
    1, 0x91,
    VK_PSG_LOCK | 1, 0x92,
    VK_PSG_FRAME_END
};
static const VKSoundStream jingle = {jingle_data, VK_PSG_NO_LOOP};

static void check_generated_streams(void)
{
    int i;
    for (i = 0; i < VK_MSX_EFFECT_SLOTS + VK_MSX_MUSIC_SLOTS; i++)
    {
        const VKSoundStream *s = i < VK_MSX_EFFECT_SLOTS ? &vk_msx_effects[i]
                                                         : &vk_msx_music[i - VK_MSX_EFFECT_SLOTS];
        const VKPsgByte *c;
        int frames = 0, loop_ok;
        if (!s->data) continue;
        c = s->data;
        loop_ok = s->loop == VK_PSG_NO_LOOP;
        /* Frame headers chain to 0xFF; the loop offset is a frame header. */
        while (*c != VK_PSG_FRAME_END)
        {
            if ((unsigned)(c - s->data) == s->loop) loop_ok = 1;
            assert((*c & VK_PSG_COUNT_MASK) < VK_PSG_COUNT_MASK);
            c += 1 + (*c & VK_PSG_COUNT_MASK);
            frames++;
        }
        assert(frames > 0 && loop_ok);
    }
    /* Effects write only SN tone 2 and the noise (AY channel C). */
    for (i = 1; i < VK_MSX_EFFECT_SLOTS; i++)
    {
        const VKPsgByte *c = vk_msx_effects[i].data;
        assert(c);
        while (*c != VK_PSG_FRAME_END)
        {
            int k, n = *c & VK_PSG_COUNT_MASK;
            for (k = 1; k <= n; k++)
                if (c[k] & 0x80) assert(((c[k] >> 5) & 3) >= 2);
            c += 1 + n;
        }
    }
    /* Stage music 80 loops, death 89 does not and locks the effects (C0A8). */
    assert(vk_msx_music[0].loop != VK_PSG_NO_LOOP);
    assert(vk_msx_music[9].loop == VK_PSG_NO_LOOP);
    assert(vk_msx_music[9].data[0] & VK_PSG_LOCK);
}

int main(void)
{
    VKPsg p;
    VKPsgByte out[VK_PSG_OUT_MAX];
    int n, i;

    check_generated_streams();

    /* Music alone: every byte goes out, the loop goes back to frame 1. */
    vk_psg_core_reset(&p);
    assert(vk_psg_core_music(&p, &music, 0x80));
    n = vk_psg_core_frame(&p, out);
    assert(n == 8 && !memcmp(out, music_data + 1, 8));
    n = vk_psg_core_frame(&p, out);
    assert(n == 2 && out[0] == 0xC6);
    n = vk_psg_core_frame(&p, out);
    assert(n == 2 && out[0] == 0xC8);
    n = vk_psg_core_frame(&p, out);
    assert(n == 2 && out[0] == 0xC6);             /* looped to frame 1 */

    /* Pause (FD): every channel silent, the jingle alone, the music frozen;
     * FE puts the music channels back and the music goes on where it was. */
    vk_psg_core_reset(&p);
    vk_psg_core_music(&p, &music, 0x80);
    vk_psg_core_frame(&p, out);
    vk_psg_core_pause(&p, &effect);
    n = vk_psg_core_frame(&p, out);
    assert(n == 4 + 3 && out[0] == 0x9F && out[3] == 0xFF && out[4] == 0xCA);
    n = vk_psg_core_frame(&p, out);
    assert(n == 1 && out[0] == 0xF1);
    n = vk_psg_core_frame(&p, out);
    assert(n == 4 && !p.pause.playing);           /* jingle over: silence */
    n = vk_psg_core_frame(&p, out);
    assert(n == 0 && p.paused);
    vk_psg_core_resume(&p);
    n = vk_psg_core_frame(&p, out);
    assert(contains(out, n, 0x81) && contains(out, n, 0x90) && contains(out, n, 0xD2));
    assert(contains(out, n, 0xC6));               /* then music frame 1, not frame 2 */

    /* An effect takes tone 2 and the noise; music tone 0 goes on. */
    vk_psg_core_reset(&p);
    vk_psg_core_music(&p, &music, 0x80);
    vk_psg_core_frame(&p, out);
    assert(vk_psg_core_effect(&p, &effect, 0x05));
    assert(p.effect_id == 0x05 && p.c_muted);
    n = vk_psg_core_frame(&p, out);               /* music frame 1 (tone 2) is held */
    assert(!contains(out, n, 0xC6));
    assert(contains(out, n, 0xDF) && contains(out, n, 0xFF) && contains(out, n, 0xCA));
    /* A lower effect is refused, an equal or higher one is taken (5172). */
    assert(!vk_psg_core_effect(&p, &effect, 0x03));
    assert(vk_psg_core_effect(&p, &effect, 0x05));
    assert(vk_psg_core_effect(&p, &effect, 0x0C));
    assert(p.effect_id == 0x0C && p.effect_count == 3 && p.last_effect == 0x0C);
    vk_psg_core_frame(&p, out);                   /* effect frame 0 again, music frame 2 held */
    vk_psg_core_frame(&p, out);                   /* effect frame 1 */
    n = vk_psg_core_frame(&p, out);               /* effect end: channel C silent */
    assert(!p.effect.playing && p.effect_id == 0 && p.c_muted);
    assert(contains(out, n, 0xDF) && contains(out, n, 0xFF));
    /* Music frame 1 (no note flag) still muted, frame 2 (note on C) restores. */
    for (i = 0; i < 3 && p.c_muted; i++)
    {
        n = vk_psg_core_frame(&p, out);
        if (p.c_muted) assert(!contains(out, n, 0xC6) && !contains(out, n, 0xC8));
    }
    assert(!p.c_muted);
    assert(contains(out, n, 0xD2) && contains(out, n, 0xE7) && contains(out, n, 0xC8));

    /* A music request ends the effect (5118). */
    vk_psg_core_effect(&p, &effect, 0x05);
    vk_psg_core_music(&p, &music, 0x80);
    assert(!p.effect.playing && p.effect_id == 0 && !p.c_muted);

    /* Lock (C0A8): effects refused after the locking frame, even once the music
     * ended; a stop or a new music clears it. */
    vk_psg_core_reset(&p);
    vk_psg_core_music(&p, &jingle, 0x89);
    vk_psg_core_frame(&p, out);
    assert(vk_psg_core_effect(&p, &effect, 0x05));
    vk_psg_core_stop(&p);
    vk_psg_core_music(&p, &jingle, 0x89);
    vk_psg_core_frame(&p, out);
    vk_psg_core_frame(&p, out);
    assert(p.lock && !vk_psg_core_effect(&p, &effect, 0x1C));
    n = vk_psg_core_frame(&p, out);               /* end: the channels go silent */
    assert(!p.music.playing && p.music_id == 0 && n == 4);
    assert(!vk_psg_core_effect(&p, &effect, 0x05));
    vk_psg_core_stop(&p);
    assert(vk_psg_core_effect(&p, &effect, 0x05));

    /* Fade (FF): volumes drop by 1.5 SN steps every 3A frames, the music stops
     * after 16 steps. */
    vk_psg_core_reset(&p);
    vk_psg_core_music(&p, &music, 0x80);
    vk_psg_core_frame(&p, out);
    vk_psg_core_fade(&p);
    for (i = 1; i < VK_PSG_FADE_FRAMES; i++) vk_psg_core_frame(&p, out);
    n = vk_psg_core_frame(&p, out);
    assert(p.fade_steps == 1 && contains(out, n, 0x92));   /* 0x90 + 2 */
    for (i = 0; i < VK_PSG_FADE_FRAMES * VK_PSG_FADE_STEPS && p.music.playing; i++)
        vk_psg_core_frame(&p, out);
    assert(!p.music.playing);

    /* Real data: whip 05 over stage music 80 for 200 frames never writes music
     * bytes on tone 2 while the effect plays. */
    vk_psg_core_reset(&p);
    vk_psg_core_music(&p, &vk_msx_music[0], 0x80);
    for (i = 0; i < 100; i++) vk_psg_core_frame(&p, out);
    assert(vk_psg_core_effect(&p, &vk_msx_effects[5], 0x05));
    for (i = 0; i < 200; i++)
    {
        n = vk_psg_core_frame(&p, out);
        assert(n <= VK_PSG_OUT_MAX);
    }
    assert(p.music.playing && !p.effect.playing && p.effect_count == 1);
    return 0;
}
