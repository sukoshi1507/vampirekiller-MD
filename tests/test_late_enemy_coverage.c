/* The coverage report deliberately cannot pretend unsupported types work. */
#include <assert.h>
#include <stdio.h>
#include "late_enemy_data.h"

int main(void) {
    const unsigned char expected_missing[7] = {0,9,15,10,6,4,3};
    const unsigned char expected_known[7] = {7,23,0,0,0,6,10};
    unsigned st, room, slot, total_missing=0, total_known=0, total_rooms=0;
    for (st=12; st<=18; ++st) {
        unsigned missing=0, known=0;
        for (room=0; room<vk_late_enemy_room_count((unsigned char)st); ++room) {
            ++total_rooms;
            for (slot=0; slot<4; ++slot) {
                unsigned char status=vk_late_enemy_port_status((unsigned char)st,(unsigned char)room,(unsigned char)slot);
                assert(status<=2);
                missing+=(status==2);
                known+=(status==1);
            }
        }
        assert(missing==expected_missing[st-12]);
        assert(known==expected_known[st-12]);
        assert(vk_late_enemy_missing_count((unsigned char)st)==missing);
        total_missing+=missing;
        total_known+=known;
    }
    assert(total_rooms==74 && total_missing==47 && total_known==46);
    assert(vk_late_enemy_port_status(11,0,0)==3);
    assert(vk_late_enemy_port_status(12,0,4)==3);
    assert(vk_late_enemy_missing_count(11)==0);
    assert(vk_late_enemy_missing_count(19)==0);
    printf("enemy coverage: %u rooms, %u existing constructors, %u unported entries PASS\\n",total_rooms,total_known,total_missing);
    return 0;
}
