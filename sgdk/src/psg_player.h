#ifndef VK_PSG_PLAYER_H
#define VK_PSG_PLAYER_H

#include <genesis.h>
#include "psg_core.h"
#include "generated/msx_sound_md.h"

/* MSX2 sound requests on the Mega Drive PSG. The ids are those the game passes to
 * the MSX request routine 509F (research/audio-driver.md): 0x01..0x1C effects,
 * 0x80..0x8E music. Channel sharing and priority: psg_core.h. Call
 * vk_psg_update once per frame. */
extern VKPsg vk_psg;

bool vk_psg_music(u8 id);       /* 509F with A = 80..8E */
bool vk_psg_effect(u8 id);      /* 509F with A = 01..1C; FALSE when refused */
void vk_psg_stop(void);         /* 509F with A = 00 */
void vk_psg_fade(void);         /* 509F with A = FF */
/* Sound test: any stream on the music voice (FB timed item, FD pause jingle). */
void vk_psg_play_stream(const VKSoundStream *stream);
void vk_psg_update(void);

/* MSX request ids used by the game code (callers: research/audio-driver.md). */
#define VK_SFX_WHIP 0x05         /* 7225 */
#define VK_SFX_LANDING 0x07      /* 6D38, 6F7E */
#define VK_SFX_WATER 0x09        /* 76E5 */
#define VK_SFX_HIT 0x0C          /* 7FD8 and the other whip contacts */
#define VK_SFX_BLOCK 0x0E        /* 87D8 */
#define VK_SFX_HEART 0x0F        /* 8DAF, 8DB7 */
#define VK_SFX_CHEST 0x11        /* 8F7B */
#define VK_SFX_ITEM 0x12         /* 8D8A: weapon and most items */
#define VK_SFX_DAMAGE 0x13       /* 6F9A */
#define VK_SFX_KEY 0x14          /* 8E5D, 8E68 */
#define VK_SFX_DOOR 0x15         /* 6DC5, 7774 */
#define VK_SFX_DOOR_1A 0x1A      /* 776F: door of 772A when C5AC != 5 */
#define VK_MUSIC_STAGE_0_3 0x80  /* 7972[D000] for stages 0..3 */
#define VK_MUSIC_DEATH 0x89      /* 70AA */
#define VK_MUSIC_START 0x8A      /* 4222 */
#define VK_MUSIC_GAME_OVER 0x8B  /* 42AB */

#endif
