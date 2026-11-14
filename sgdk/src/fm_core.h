#ifndef VK_FM_CORE_H
#define VK_FM_CORE_H

#include "psg_core.h"

/* FM music mode (research/audio-fm.md): the three music voices on YM2612 channels 1 to 3,
 * the PSG keeping the drums. No SGDK dependency (tested natively, tests/test_fm_core.c).
 *
 * The PSG core (psg_core.c) is not touched: it runs the music with its stream of drums
 * (generated/msx_fm_md.c, vk_fm_noise: same frames, note and lock flags and loop frame as the
 * full PSG stream) and keeps every rule of the MSX driver. vk_fm_core_step runs one PSG
 * frame, then moves the FM stream in step with it, deciding from the state of the PSG core
 * before and after the frame (VKFmPre) what the driver does to the voices:
 * - the FM stream advances one frame whenever the PSG music frame does (same loop frame, end
 *   of a music without loop keys every voice off);
 * - voice C (YM2612 channel 3) is keyed off when an effect starts and its stream writes are
 *   dropped (state kept) while an effect plays and until the first note of that voice after
 *   it (the PSG core's c_muted, cleared by the NOTE_C flag): the note then writes its own
 *   frequency, level and key on;
 * - FD (pause) and FB (time stop) key every voice off; FE and FC key the held notes on again;
 * - the FF fade lowers the carrier levels by VK_FM_FADE_TL TL steps (0.75 dB) per fade step of
 *   3 dB, in the stream writes too; at the end of the fade every voice is keyed off.
 * The (register, value) pairs go to YM2612 part 0, written by psg_player.c. */

/* Largest output of one step in pairs: instrument registers (first music), three key offs,
 * one stream frame (63 pairs at most), edge work. */
#define VK_FM_OUT_PAIRS 176
#define VK_FM_FADE_TL 4          /* carrier TL steps per AY fade step (3 dB of 0.75 dB) */
#define VK_FM_UNSET 0xFF

typedef struct {
    const VKSoundStream *stream;
    const VKPsgByte *cursor;
    VKPsgByte run;               /* active or edge: the player calls vk_fm_core_step (one test in the V-Int) */
    VKPsgByte active;            /* the current music has an FM stream */
    VKPsgByte inited;            /* the instrument registers are written */
    VKPsgByte edge;              /* a music starts: every voice keyed off first */
    VKPsgByte keyed[3];          /* what the stream wants: voice keyed on */
    VKPsgByte tl[16];            /* last carrier TL of the stream per register & 15, VK_FM_UNSET: none */
    VKPsgByte n;                 /* pairs in out for this step */
    VKPsgByte out[2 * VK_FM_OUT_PAIRS];
} VKFm;

/* vk_psg_core_frame of psg_core.c, compiled a second time (psg_core_fm.c) for this module. */
int vk_psg_core_fm_frame(VKPsg *p, VKPsgByte *out);

void vk_fm_core_reset(VKFm *f);
/* The music s was just requested on the PSG core (vk_psg_core_music with its drums stream):
 * its FM stream goes with it. */
void vk_fm_core_start(VKFm *f, const VKSoundStream *fm);
/* A PSG music replaces the FM one, or a stop: the voices must be keyed off by the caller
 * (vk_fm_core_silence); the FM stream is dropped. */
void vk_fm_core_cancel(VKFm *f);
/* Instrument registers into out (as pairs, n set) if not written yet. */
void vk_fm_core_init(VKFm *f);
/* Key off of the three voices: 3 pairs. Returns the byte count (6). */
int vk_fm_core_silence(VKPsgByte *out);
/* One 60 Hz step: vk_psg_core_frame (returns its byte count, PSG bytes in psg_out), then the
 * FM frame in step; the YM2612 pairs are in f->out, f->n of them. Call it instead of
 * vk_psg_core_frame while f->run (set by vk_fm_core_start, cleared with the FM music). */
int vk_fm_core_step(VKFm *f, VKPsg *p, VKPsgByte *psg_out);

#endif
