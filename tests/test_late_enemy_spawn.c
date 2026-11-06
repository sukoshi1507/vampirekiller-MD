/* Regression on opt-in fixed spawn for late stages.  These assert reuse of
 * existing routines, NOT their frame-by-frame fidelity in stages 12..18. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "enemies.h"
#include "generated/late_fixed_enemies.h"
#include "generated/late_enemy_flags.h"

static void check(unsigned char stage, unsigned char room, unsigned int expected,
                  unsigned char first_type, int first_x, unsigned char first_y)
{
    EnemyState s;
    EnemyRoomVars v;
    EnemySpawnTimer t;
    unsigned int n=0, i;
    memset(&v,0,sizeof v);
    memset(&t,0,sizeof t);
    v.stage=stage; v.room=room; v.simonX=110; v.simonY=180;
    Enemies_init(&s);
    Enemies_setRoom(&s);
    Enemies_god=1;
    assert(Enemies_frame(&s,&t,&v)==0);
    for(i=0;i<ENEMY_MAX;i++) if(s.enemy[i].active) n++;
    assert(n==expected);
    if(expected) {
       assert(s.enemy[0].active);
       assert(s.enemy[0].type==first_type);
       /* Dragon moves its collision body on initialisation: compare type only.
        * Other two types preserve raw XY immediately after spawn. */
       if(first_type!=ENEMY_TYPE_DRAGON) {
          assert(s.enemy[0].x==first_x);
          assert(s.enemy[0].y==first_y);
       }
    }
    /* A second frame must not duplicate the fixed list. */
    assert(Enemies_frame(&s,&t,&v)==0);
    n=0;
    for(i=0;i<ENEMY_MAX;i++) if(s.enemy[i].active) n++;
    assert(n==expected);
    Enemies_god=0;
}
static void all_rooms(void)
{
    unsigned int st,room,covered=0,unknown=0,rooms=0;
    for(st=12;st<=18;st++) for(room=0;room<vk_late_enemy_room_counts[st-12];room++) {
        EnemyState s;
        EnemySpawnTimer t;
        EnemyRoomVars v;
        unsigned int expected=0,found=0;
        unsigned int slot;
        memset(&v,0,sizeof v); memset(&t,0,sizeof t);
        v.stage=(EnemyU8)st;v.room=(EnemyU8)room;v.simonX=110;v.simonY=180;
        Enemies_init(&s);Enemies_setRoom(&s);Enemies_god=1;
        for(slot=0;slot<4;slot++) {
            const unsigned int type=vk_late_fixed_raw[st-12][room][slot][0]&127;
            if(type==11||type==13||type==14) {expected++;covered++;}
            else if(type) unknown++;
        }
        assert(Enemies_frame(&s,&t,&v)==0);
        for(slot=0;slot<ENEMY_MAX;slot++) if(s.enemy[slot].active) found++;
        assert(found==expected);
        rooms++;
    }
    Enemies_god=0;
    assert(rooms==74 && covered==46 && unknown==47);
    printf("late fixed spawn: %u rooms, %u known types spawned, %u unknown kept inactive\n",rooms,covered,unknown);
}
int main(void) {
    check(12,0,1,ENEMY_TYPE_DRAGON,192,144); /* type 0Eh */
    check(13,0,1,ENEMY_TYPE_HOPPER,192,176); /* 0Bh; 09h kept unsupported */
    check(13,1,2,ENEMY_TYPE_HOPPER,64,80);  /* 0Bh + 0Dh */
    check(14,0,0,0,0,0);                   /* 10h not silently remapped */
    check(16,1,0,0,0,0);                   /* 12h not silently remapped */
    check(17,1,1,ENEMY_TYPE_HOPPER,96,176);
    check(18,3,2,ENEMY_TYPE_FLEA,208,112);
    all_rooms();
    puts("late stages 12..18: opt-in known-type fixed spawn and unsupported-type guard PASS");
    return 0;
}
