/* Native test of the state 9 freeze (sgdk/src/room_freeze.c). */
#include <stdio.h>
#include "room_freeze.h"

static int failures = 0;
#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            failures++;                                   \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);   \
            printf(__VA_ARGS__);                          \
            printf("\n");                                 \
        }                                                 \
    } while (0)

/* Table measured in openMSX (research/room-links.md section 9). */
static void test_table(void)
{
    static const unsigned char hall[8] = {19, 20, 22, 22, 21, 19, 19, 23};
    static const unsigned char hall_dark[8] = {2, 2, 4, 4, 3, 2, 2, 4};
    static const unsigned char stage2[6] = {23, 24, 20, 21, 21, 21};
    static const unsigned char stage2_dark[6] = {3, 3, 2, 3, 3, 3};
    int r;
    for (r = 0; r < 3; r++)
        CHECK(vk_room_freeze_timing(0, (VKFreezeByte)r).frames == 18 &&
              vk_room_freeze_timing(0, (VKFreezeByte)r).dark == 1, "cour salle %d : 18 images, noir a 1", r);
    for (r = 0; r < 8; r++)
        CHECK(vk_room_freeze_timing(1, (VKFreezeByte)r).frames == hall[r] &&
              vk_room_freeze_timing(1, (VKFreezeByte)r).dark == hall_dark[r], "hall salle %d", r);
    for (r = 0; r < 6; r++)
        CHECK(vk_room_freeze_timing(2, (VKFreezeByte)r).frames == stage2[r] &&
              vk_room_freeze_timing(2, (VKFreezeByte)r).dark == stage2_dark[r], "stage 2 salle %d", r);
    CHECK(vk_room_freeze_timing(3, 0).frames == 20 && vk_room_freeze_timing(1, 8).frames == 20,
          "salle inconnue : 20 images (provisoire)");
}

/* Sequence of a courtyard change: old room 1 frame, display off, black, room shown last. */
static void test_sequence(void)
{
    VKRoomFreeze f = {0, 0, 0};
    int k, dark = -1, show = -1, frozen = 0;
    CHECK(!vk_room_freeze_active(&f) && vk_room_freeze_tick(&f) == VK_FREEZE_OFF, "inactif au depart");
    vk_room_freeze_begin(&f, 0, 1);
    for (k = 0; k < 40; k++)
    {
        const VKFreezeStep s = vk_room_freeze_tick(&f);
        if (s == VK_FREEZE_OFF) break;
        frozen++;
        if (s == VK_FREEZE_DARK) { CHECK(dark < 0, "un seul passage au noir"); dark = k; }
        if (s == VK_FREEZE_SHOW) { CHECK(show < 0, "un seul affichage"); show = k; }
    }
    CHECK(frozen == 18, "18 images figees (obtenu %d)", frozen);
    CHECK(dark == 1 && show == 17, "noir a l'image 1, salle a l'image 17 (obtenu %d, %d)", dark, show);
    CHECK(!vk_room_freeze_active(&f), "fin du gel");
    vk_room_freeze_begin(&f, 2, 1);
    vk_room_freeze_clear(&f);
    CHECK(!vk_room_freeze_active(&f) && vk_room_freeze_tick(&f) == VK_FREEZE_OFF, "gel annule");
}

int main(void)
{
    test_table();
    test_sequence();
    if (failures) { printf("room freeze tests: %d failure(s)\n", failures); return 1; }
    printf("room freeze tests: OK\n");
    return 0;
}
