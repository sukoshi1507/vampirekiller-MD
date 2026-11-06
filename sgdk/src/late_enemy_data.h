/* Z80-derived enemy metadata (stages 12-18). Does not create enemies. */
#ifndef VK_LATE_ENEMY_DATA_H
#define VK_LATE_ENEMY_DATA_H
typedef struct { unsigned char value; unsigned char valid; } VkLateEnemyFlag;
typedef struct { unsigned char raw_type, packed_yx, reserved, identifier, x, y, valid; } VkLateFixedEnemy;
VkLateEnemyFlag vk_late_enemy_room_flag(unsigned char stage, unsigned char room);
unsigned char vk_late_enemy_room_count(unsigned char stage);
VkLateFixedEnemy vk_late_fixed_enemy(unsigned char stage, unsigned char room, unsigned char slot);
/* CPU update address from ROM 9942 for one of the six observed fixed types. 0 = absent. */
unsigned short vk_late_enemy_update_address(unsigned char raw_type);
/* Fixed enemy late-stage coverage status.  Does not imply trace-level fidelity.
 * 0 = empty slot, 1 = existing engine constructor, 2 = unported handler,
 * 3 = invalid room/slot.  Uses ROM type identity, never visual similarity. */
unsigned char vk_late_enemy_port_status(unsigned char stage, unsigned char room, unsigned char slot);
/* Number of fixed ROM entries still lacking a constructor, per stage. */
unsigned char vk_late_enemy_missing_count(unsigned char stage);
#endif
