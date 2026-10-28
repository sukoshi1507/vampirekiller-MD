#include "sound_test.h"
#include "psg_player.h"

/* Debug-only sound test: every MSX2 sound extracted from the ROM driver
 * (generated/msx_sound_md.c), by its 509F request id. An effect started while a
 * music plays takes channel C as on the MSX (psg_core.h). */

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

/* Entries: effects 01..1C, music 80..8E, then FB (timed item) and FD (pause). */
#define EFFECT_ENTRIES (VK_MSX_EFFECT_SLOTS - 1)
#define MUSIC_ENTRIES (VK_MSX_MUSIC_SLOTS - 1)
#define SOUND_ENTRY_COUNT (EFFECT_ENTRIES + MUSIC_ENTRIES + 2)
#define VISIBLE_ROWS 16
#define FIRST_ROW 5

static const u16 sound_palette[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0EEE
};
static u16 selected;
static u16 first;

static u8 entry_id(u16 entry)
{
    if (entry < EFFECT_ENTRIES) return (u8)(entry + 1);
    entry -= EFFECT_ENTRIES;
    if (entry < MUSIC_ENTRIES) return (u8)(0x80 + entry);
    return entry == MUSIC_ENTRIES ? 0xFB : 0xFD;
}

static const char *entry_name(u8 id)
{
    if (id == 0xFB) return "MINUTERIE FB";
    if (id == 0xFD) return "PAUSE FD";
    return id >= 0x80 ? vk_msx_music_names[id - 0x80] : vk_msx_effect_names[id];
}

static void draw_menu(void)
{
    u16 row;
    char line[40];
    for (row = 0; row < VISIBLE_ROWS; row++)
    {
        const u16 entry = first + row;
        const u8 id = entry_id(entry);
        sprintf(line, "%c %s %02X %-16s", entry == selected ? '>' : ' ',
                id >= 0x80 ? "MUS" : "EFF", id, entry_name(id));
        VDP_drawText(line, 2, FIRST_ROW + row);
    }
    sprintf(line, "MUSIQUE %02X  EFFET %02X  ", vk_psg.music_id, vk_psg.effect_id);
    VDP_drawText(line, 2, 23);
    VDP_drawText("HAUT/BAS A JOUER C ARRET B QUITTER", 1, 26);
}

void vk_sound_test_enter(void)
{
    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    PAL_setPalette(PAL1, sound_palette, CPU);
    VDP_setTextPalette(PAL1);
    VDP_drawText("SOUND TEST (DEBUG)", 8, 2);
    selected = 0;
    first = 0;
    draw_menu();
    VDP_setEnable(TRUE);
}

bool vk_sound_test_tick(u16 pressed)
{
    static u8 shown_music, shown_effect;
    if (pressed & BUTTON_B)
    {
        vk_psg_stop();
        return TRUE;
    }
    if (pressed & BUTTON_UP) selected = selected == 0 ? SOUND_ENTRY_COUNT - 1 : selected - 1;
    else if (pressed & BUTTON_DOWN) selected = (selected + 1) % SOUND_ENTRY_COUNT;
    if (selected < first) first = selected;
    else if (selected >= first + VISIBLE_ROWS) first = selected - VISIBLE_ROWS + 1;
    if (pressed & BUTTON_A)
    {
        const u8 id = entry_id(selected);
        if (id == 0xFB) vk_psg_play_stream(&vk_msx_timed_item);
        else if (id == 0xFD) vk_psg_play_stream(&vk_msx_pause);
        else if (id >= 0x80) vk_psg_music(id);
        else vk_psg_effect(id);
    }
    if (pressed & BUTTON_C) vk_psg_stop();
    if ((pressed & (BUTTON_UP | BUTTON_DOWN | BUTTON_A | BUTTON_C)) ||
        shown_music != vk_psg.music_id || shown_effect != vk_psg.effect_id)
    {
        shown_music = vk_psg.music_id;
        shown_effect = vk_psg.effect_id;
        draw_menu();
    }
    return FALSE;
}
