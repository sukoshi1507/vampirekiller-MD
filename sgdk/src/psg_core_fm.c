/* A second copy of psg_core.c for the FM mode (fm_core.c calls vk_psg_core_fm_frame).
 *
 * Why a copy: when psg_core.c has one caller of vk_psg_core_frame (the PSG V-Int), the compiler
 * inlines it there; a second caller makes it a shared function and changes the cost of the PSG
 * interrupt by a few dozen cycles, enough to break the frame-exact replay of tests/md_playthrough
 * (research/audio-fm.md). With the copy, the PSG mode is compiled exactly as before FM existed.
 * Only vk_psg_core_fm_frame is used; the linker (--gc-sections) drops the rest. The pragma stops
 * the compiler from merging the two identical copies back into one function (-fipa-icf). */
#pragma GCC optimize ("no-ipa-icf")
#define vk_psg_core_reset vk_psg_core_fm_reset
#define vk_psg_core_silence vk_psg_core_fm_silence
#define vk_psg_core_music vk_psg_core_fm_music
#define vk_psg_core_effect vk_psg_core_fm_effect
#define vk_psg_core_stop vk_psg_core_fm_stop
#define vk_psg_core_fade vk_psg_core_fm_fade
#define vk_psg_core_pause vk_psg_core_fm_pause
#define vk_psg_core_resume vk_psg_core_fm_resume
#define vk_psg_core_timer_end vk_psg_core_fm_timer_end
#define vk_psg_core_timer vk_psg_core_fm_timer
#define vk_psg_core_frame vk_psg_core_fm_frame
#include "psg_core.c"
