#include <assert.h>
#include <string.h>
#include "player.h"
#include "generated/player_assets.h"

static VKByte map[768];
static void row(int r) { int x; for (x=0;x<32;x++) map[r*32+x]=1; }
static void start(VKPlayer *p, int y) { vk_player_reset(p); p->x=80; p->y=(VKByte)y; }
int main(void)
{
    VKPlayer p;
    int i, highest;
    /* Solid floor at feet y=160, with original (y-16) tile origin. */
    memset(map, 0, sizeof(map)); row(18);
    start(&p, 160);
    assert(vk_grounded(map,p.x,p.y,1,0));
    vk_player_tick_map(&p,map,1,0,1);
    assert(p.mode==1);
    highest=p.y;
    for(i=0;i<85;i++) { vk_player_tick_map(&p,map,1,0,0); if(p.y<highest) highest=p.y; }
    assert(highest<160 && p.mode==0 && p.y>=152 && p.y<=160);
    /* No ceiling clipping through a one-tile-thick shelf. */
    memset(map,0,sizeof(map)); row(18); row(12);
    start(&p,160);
    vk_player_tick_map(&p,map,1,0,1);
    for(i=0;i<25;i++) {
        vk_player_tick_map(&p,map,1,0,0);
        assert(p.y>= (12*8+16+30));
    }
    /* Falling quickly must not pass through a thin floor. */
    memset(map,0,sizeof(map)); row(18);
    start(&p,100); p.mode=4; p.phase=3;
    for(i=0;i<45;i++) vk_player_tick_map(&p,map,1,0,0);
    assert(p.mode==0 && p.y<=160 && p.y>=152);
    /* Walk slowly into a one-tile-thick vertical barrier. */
    memset(map,0,sizeof(map));
    for(i=0;i<24;i++) map[i*32+12]=1;
    start(&p,120); p.mode=1; p.phase=1;
    for(i=0;i<12;i++) vk_player_tick_map(&p,map,1,1,0);
    assert(p.x<=88);
    /* Crouch: stationary, immune to jump while DOWN is held. */
    memset(map,0,sizeof(map)); row(18);
    start(&p,160);
    vk_player_tick_controls(&p,map,1,1,1,1,0,0,0);
    assert(p.crouched && p.mode==0 && p.x==80);
    vk_player_tick_controls(&p,map,1,0,0,0,0,0,0);
    assert(!p.crouched);
    /* An explicit staircase strip enables climbing without falling. */
    {
        const VKStair stair = {76,100,110,170};
        start(&p,160);
        vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
        assert(p.on_stairs && p.y==159 && p.facing==0);
        for(i=0;i<8;i++) vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
        assert(p.y==151 && p.on_stairs);
        vk_player_tick_controls(&p,map,1,0,0,1,0,&stair,1);
        assert(p.on_stairs && p.y==152 && p.facing==1);
        assert(p.pose>=3 && p.pose<=5);
        /* Pose cadence follows actual movement, not global idle frames. */
        start(&p,160);
        vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
        assert(p.stair_anim==1 && p.pose==0);
        for (i=0;i<6;i++) vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
        assert(p.stair_anim==7 && p.pose==0);
        vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
        assert(p.stair_anim==8 && p.pose==1);
        { VKByte progress=p.stair_anim;
          for (i=0;i<15;i++) vk_player_tick_controls(&p,map,1,0,0,0,0,&stair,1);
          assert(p.stair_anim==progress && p.pose==0);
        }
        vk_player_tick_controls(&p,map,1,0,0,1,0,&stair,1);
        assert(p.facing==1 && p.pose>=3 && p.pose<=5);
        assert(p.stair_anim==9);
        /* At a terminal step the actor stays attached to the stair. */
        for (i=0;i<180;i++) vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
        assert(p.on_stairs && p.x<=100 && p.y>=110);
        { VKByte hold_x=p.x, hold_y=p.y;
          for(i=0;i<10;i++) vk_player_tick_controls(&p,map,1,0,0,0,1,&stair,1);
          assert(p.on_stairs && p.x==hold_x && p.y==hold_y); }
        vk_player_tick_controls(&p,map,1,0,0,0,0,&stair,1);
        assert(p.on_stairs && p.pose==0);
        vk_player_tick_controls(&p,map,1,0,1,0,0,&stair,1);
        assert(!p.on_stairs && p.mode==1);
        start(&p,160);
        vk_player_tick_controls(&p,map,1,0,0,0,1,0,0);
        assert(!p.on_stairs);
    }
    return 0;
}
