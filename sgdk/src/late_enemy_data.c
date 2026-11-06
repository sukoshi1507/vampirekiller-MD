/* Independent data access, NO connection to existing enemy spawn/gameplay. */
#include "late_enemy_data.h"
#include "generated/late_enemy_flags.h"
#include "generated/late_fixed_enemies.h"
unsigned char vk_late_enemy_room_count(unsigned char stage) {
    return stage >= 12 && stage <= 18 ? vk_late_enemy_room_counts[stage-12] : 0;
}
VkLateEnemyFlag vk_late_enemy_room_flag(unsigned char stage, unsigned char room) {
    VkLateEnemyFlag result = {0, 0};
    if (stage >= 12 && stage <= 18 && room < vk_late_enemy_room_counts[stage-12]) {
        result.value = vk_late_enemy_flags[stage-12][room];
        result.valid = 1;
    }
    return result;
}
VkLateFixedEnemy vk_late_fixed_enemy(unsigned char stage, unsigned char room, unsigned char slot) {
    VkLateFixedEnemy e = {0,0,0,0,0,0,0};
    const unsigned char *b;
    if (stage < 12 || stage > 18 || room >= vk_late_enemy_room_counts[stage-12] || slot >= 4)
        return e;
    b = vk_late_fixed_raw[stage-12][room][slot];
    e.raw_type=b[0];e.packed_yx=b[1];e.reserved=b[2];e.identifier=b[3];
    e.x=(unsigned char)((b[1]&15)*16);e.y=(unsigned char)(b[1]&240);
    e.valid=1;
    return e;
}
