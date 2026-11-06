/* Independent data access, NO connection to existing enemy spawn/gameplay. */
#include "late_enemy_data.h"
#include "generated/late_enemy_flags.h"
#include "generated/late_fixed_enemies.h"
#include "generated/late_enemy_routines.h"
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

unsigned short vk_late_enemy_update_address(unsigned char raw_type) {
    unsigned char k;
    for (k=0; k<6; k++) if ((raw_type & 0x7F)==vk_late_update_type[k])
        return vk_late_update_address[k];
    return 0;
}

unsigned char vk_late_enemy_port_status(unsigned char stage, unsigned char room, unsigned char slot) {
    VkLateFixedEnemy e = vk_late_fixed_enemy(stage, room, slot);
    unsigned char type;
    if (!e.valid) return 3;
    type = (unsigned char)(e.raw_type & 0x7f);
    if (!type) return 0;
    if (type == 0x0b || type == 0x0d || type == 0x0e) return 1;
    return 2;
}
unsigned char vk_late_enemy_missing_count(unsigned char stage) {
    unsigned char room, slot, n = 0;
    if (stage < 12 || stage > 18) return 0;
    for (room = 0; room < vk_late_enemy_room_count(stage); ++room)
        for (slot = 0; slot < 4; ++slot)
            if (vk_late_enemy_port_status(stage, room, slot) == 2) ++n;
    return n;
}
