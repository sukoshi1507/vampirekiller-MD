#ifndef VK_PSG_PLAYER_H
#define VK_PSG_PLAYER_H

#include <genesis.h>

/* Minimal SN76489 player driven from the 68k. The stream format is produced by
 * tools/build_music_md.py: per frame, a count byte then that many PSG bytes;
 * the count 0xFF ends the stream. Call vk_psg_update once per 60 Hz frame. */
void vk_psg_play(const u8 *stream);
void vk_psg_stop(void);
void vk_psg_update(void);

#endif
