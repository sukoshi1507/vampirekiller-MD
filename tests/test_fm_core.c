#include <assert.h>
#include <string.h>
#include "psg_core.h"
#include "fm_core.h"
#include "generated/msx_fm_md.h"

/* FM music mode of fm_core.c on top of psg_core.c, on hand-made streams (the generated streams
 * are replayed by tests/test_convert_fm.py): effect priority on voice C, pause, FB, fade, end. */

/* PSG side (drums only here: nothing), flags as the generator writes them. */
static const VKPsgByte psg_data[] = {
    0,                  /* f0 */
    0,                  /* f1 */
    VK_PSG_NOTE_C | 0,  /* f2: a note starts on voice C */
    0,                  /* f3 */
    0,                  /* f4 */
    VK_PSG_FRAME_END
};
static const VKSoundStream psg_music = {psg_data, 1};
static const VKSoundStream psg_end = {psg_data, VK_PSG_NO_LOOP};

static const VKPsgByte fm_data[] = {
    /* f0: voices 0 and 2 keyed on, carrier TL 10 */
    8, 0xA4, 0x25, 0xA0, 0x9A, 0x48, 10, 0x28, 0xF0, 0x4A, 10, 0xA6, 0x24, 0x28, 0xF2, 0xA2, 0x3C,
    /* f1 (loop): voice 0 TL 12 and voice 2 TL 14 */
    2, 0x48, 12, 0x4A, 14,
    /* f2: voice 2 new note */
    5, 0x28, 0x02, 0xA6, 0x26, 0xA2, 0x40, 0x4A, 20, 0x28, 0xF2,
    /* f3: voice 0 off */
    1, 0x28, 0x00,
    /* f4 */
    0,
    VK_PSG_FRAME_END
};
static const VKSoundStream fm_music = {fm_data, 17};     /* loop at f1 (byte 17), as the PSG stream */
static const VKSoundStream fm_end = {fm_data, VK_PSG_NO_LOOP};
static const VKPsgByte effect_data[] = { 1, 0xCA, 2, 0xD0, 0x01, VK_PSG_FRAME_END };
static const VKSoundStream effect = {effect_data, VK_PSG_NO_LOOP};

static VKPsg ps;
static VKFm p;
static VKPsgByte psg_out[VK_PSG_OUT_MAX];

/* The player's step: with FM music the FM core runs the PSG core and the FM stream together. */
static void step(void)
{
    if (p.run) vk_fm_core_step(&p, &ps, psg_out);
    else { p.n = 0; vk_psg_core_frame(&ps, psg_out); }
}

static int has(VKPsgByte reg, int value)
{
    int i;
    for (i = 0; i < p.n; i++)
        if (p.out[2 * i] == reg && (value < 0 || p.out[2 * i + 1] == value)) return 1;
    return 0;
}

static void start_with(const VKSoundStream *psg, const VKSoundStream *fm, int id)
{
    vk_fm_core_reset(&p);
    vk_psg_core_reset(&ps);
    assert(vk_psg_core_music(&ps, psg, (VKPsgByte)id));
    vk_fm_core_start(&p, fm);
}

static void start(void) { start_with(&psg_music, &fm_music, 0x80); }

int main(void)
{
    int i;

    /* Start: instrument registers, three key offs, then the first frame of the stream. */
    start();
    step();
    assert(p.n == VK_FM_INIT_PAIRS + 3 + 8);
    assert(p.out[0] == vk_fm_init[0] && p.out[1] == vk_fm_init[1]);
    assert(has(0x28, 0xF0) && has(0x28, 0xF2) && has(0x48, 10));
    step();                                  /* f1 */
    assert(p.n == 2 && has(0x48, 12) && has(0x4A, 14));

    /* An effect: voice C is keyed off, then silent; voices A and B go on. */
    start(); step(); step();
    assert(vk_psg_core_effect(&ps, &effect, 0x13));
    step();                                  /* f2: effect first frame, NOTE_C but the effect plays */
    assert(has(0x28, 0x02));                 /* the key off of the effect start ... */
    assert(!has(0x4A, 20) && !has(0x28, 0xF2) && !has(0xA6, 0x26));   /* ... and no voice C write */
    step();                              /* f3: effect second frame */
    assert(has(0x28, 0x00));             /* voice A still plays on */
    step();                              /* f4: the effect ends (END read) */
    assert(!ps.effect.playing && ps.c_muted);
    step();                              /* f1 of the loop: effect over, no NOTE_C yet: voice C still muted */
    assert(!has(0x4A, 14) && has(0x48, 12));
    step();                              /* f2: NOTE_C: voice C is back with its own note */
    assert(!ps.c_muted && has(0x4A, 20) && has(0x28, 0xF2) && has(0xA6, 0x26));

    /* Pause: every voice off, frozen; resume: the keyed voices on again (A and C of f0). */
    start(); step();
    vk_psg_core_pause(&ps, 0);
    step();
    assert(p.n == 3 && has(0x28, 0) && has(0x28, 1) && has(0x28, 2));
    step();
    assert(p.n == 0);
    vk_psg_core_resume(&ps);
    step();
    assert(p.n == 4 && has(0x28, 0xF0) && has(0x28, 0xF2) && !has(0x28, 0xF1));
    assert(has(0x48, 12));                   /* the music goes on where it was (f1 of the same step) */

    /* FB: the music voices off and frozen; FC: back. */
    start(); step();
    vk_psg_core_timer(&ps, &vk_msx_timed_item);
    step();
    assert(p.n == 3);
    step();
    assert(p.n == 0);
    vk_psg_core_timer_end(&ps);
    step();
    assert(p.n == 4 && has(0x28, 0xF0) && has(0x28, 0xF2) && has(0x48, 12));

    /* Fade: 4 TL steps per fade step, on the carriers only, in the stream writes too. */
    start(); step();
    vk_psg_core_fade(&ps);
    for (i = 1; i < VK_PSG_FADE_FRAMES; i++) step();
    step();                                  /* the first fade step lands here */
    assert(ps.fade_steps == 1);
    assert(has(0x48, p.tl[8] + VK_FM_FADE_TL) && has(0x4A, p.tl[10] + VK_FM_FADE_TL));
    {
        int found = 0;
        for (i = 0; i < 3 * VK_PSG_FADE_FRAMES; i++)
        {
            step();
            if (p.n && has(0x48, 12 + 2 * VK_FM_FADE_TL)) found = 1;   /* stream write at fade 2 */
        }
        assert(found);
    }

    /* End without loop: every voice keyed off, then nothing. */
    start_with(&psg_end, &fm_end, 0x89);
    for (i = 0; i < 6; i++) step();          /* f0..f4 and the END read */
    assert(p.n == 3 && has(0x28, 0) && has(0x28, 1) && has(0x28, 2));
    assert(!ps.music.playing && !p.active);
    step();
    assert(p.n == 0);

    /* A PSG music after an FM one keys the voices off; PSG mode alone writes no FM at all. */
    /* (the player writes the key offs itself when a PSG music replaces an FM one: fm_cancel) */
    start(); step(); step();
    vk_fm_core_cancel(&p);
    assert(vk_psg_core_music(&ps, &psg_music, 0x81));
    for (i = 0; i < 6; i++) { step(); assert(p.n == 0); }
    vk_fm_core_reset(&p);
    vk_psg_core_reset(&ps);
    assert(vk_psg_core_music(&ps, &vk_msx_music[0], 0x80));
    for (i = 0; i < 3000; i++) { step(); assert(p.n == 0); }
    assert(vk_psg_core_effect(&ps, &effect, 0x13));
    for (i = 0; i < 4; i++) { step(); assert(p.n == 0); }

    /* Every generated music, two loops or to the end: no overflow, same frames as the PSG stream. */
    for (i = 0; i < VK_MSX_MUSIC_SLOTS - 1; i++)
    {
        int frames = 0, fm_frames = 0;
        const VKPsgByte *c;
        for (c = vk_fm_noise[i].data; *c != VK_PSG_FRAME_END; c += 1 + (*c & VK_PSG_COUNT_MASK)) frames++;
        for (c = vk_fm_music[i].data; *c != VK_PSG_FRAME_END; c += 1 + 2 * *c) fm_frames++;
        assert(frames == fm_frames);
        start_with(&vk_fm_noise[i], &vk_fm_music[i], 0x80 + i);
        {
            int f, max = 0;
            for (f = 0; f < 2 * frames + 5; f++)
            {
                step();
                if (p.n > max && f > 0) max = p.n;
                assert(p.n < VK_FM_OUT_PAIRS);
                if (!ps.music.playing) break;
            }
            assert(max <= 63);
            if (vk_fm_music[i].loop == VK_PSG_NO_LOOP) assert(!ps.music.playing && !p.active);
            else assert(ps.music.playing && p.active);
        }
    }
    return 0;
}
