#include "sound_test.h"
#include "generated/music_md.h"
#include "psg_player.h"

/* Debug-only sound test. Music is the candidate PSG stream from the MSX2 capture
 * (research/audio), played by psg_player.c. The trigger mapping for effects is
 * not established yet. Effects will be added here once identified. */

static const u16 konami_keys[10] = {
    BUTTON_UP, BUTTON_UP, BUTTON_DOWN, BUTTON_DOWN, BUTTON_LEFT,
    BUTTON_RIGHT, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_B, BUTTON_A
};
static u16 konami_index;

bool vk_konami_step(u16 pressed)
{
    if (!pressed) return FALSE;
    if (pressed & konami_keys[konami_index])
    {
        if (++konami_index < 10) return FALSE;
        konami_index = 0;
        return TRUE;
    }
    /* A wrong key restarts the code, but an UP press can be its first key. */
    konami_index = (pressed & BUTTON_UP) ? 1 : 0;
    return FALSE;
}

typedef struct {
    const char *label;
    const u8 *music;
} SoundEntry;

static const SoundEntry sound_entries[] = {
    {"CAPTURE PSG", vk_music_test},
    {"ARRET", NULL},
};
#define SOUND_ENTRY_COUNT ((u16)(sizeof(sound_entries) / sizeof(sound_entries[0])))

static const u16 sound_palette[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0EEE
};
static u16 selected;
static u16 playing = 0xFFFF;

static void draw_menu(void)
{
    u16 i;
    char line[40];
    for (i = 0; i < SOUND_ENTRY_COUNT; i++)
    {
        sprintf(line, "%c %s                    ", i == selected ? '>' : ' ', sound_entries[i].label);
        VDP_drawText(line, 6, 8 + i * 2);
    }
    sprintf(line, "LECTURE: %s   ", playing == 0xFFFF ? "AUCUNE" : sound_entries[playing].label);
    VDP_drawText(line, 2, 20);
    VDP_drawText("HAUT/BAS  A JOUER  B QUITTER", 2, 27);
}

void vk_sound_test_enter(void)
{
    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    PAL_setPalette(PAL1, sound_palette, CPU);
    VDP_setTextPalette(PAL1);
    VDP_drawText("SOUND TEST (DEBUG)", 8, 3);
    selected = 0;
    playing = 0xFFFF;
    draw_menu();
    VDP_setEnable(TRUE);
}

bool vk_sound_test_tick(u16 pressed)
{
    vk_psg_update();
    if (pressed & BUTTON_B)
    {
        vk_psg_stop();
        return TRUE;
    }
    if (pressed & BUTTON_UP) selected = selected == 0 ? SOUND_ENTRY_COUNT - 1 : selected - 1;
    else if (pressed & BUTTON_DOWN) selected = (selected + 1) % SOUND_ENTRY_COUNT;
    if (pressed & BUTTON_A)
    {
        if (sound_entries[selected].music)
        {
            vk_psg_play(sound_entries[selected].music);
            playing = selected;
        }
        else
        {
            vk_psg_stop();
            playing = 0xFFFF;
        }
    }
    if (pressed & (BUTTON_UP | BUTTON_DOWN | BUTTON_A)) draw_menu();
    return FALSE;
}
