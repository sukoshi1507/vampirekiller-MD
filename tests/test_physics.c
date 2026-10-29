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
    /* The right edge probe must not wrap to column 0: a solid left column does not
     * stop Simon from reaching x 248, where the room manager takes over. */
    memset(map,0,sizeof(map)); row(18);
    for(i=0;i<24;i++) map[i*32]=1;
    start(&p,160); p.x=247;
    vk_player_tick_map(&p,map,1,1,0);
    assert(p.x==248);
    /* Crouch: stationary, immune to jump while DOWN is held. */
    memset(map,0,sizeof(map)); row(18);
    start(&p,160);
    vk_player_tick_controls(&p,map,1,1,1,1,0);
    assert(p.crouched && p.mode==0 && p.x==80);
    vk_player_tick_controls(&p,map,1,0,0,0,0);
    assert(!p.crouched);
    /* A staircase is a tile-12 flight (climbing right), here four steps from the
     * foot (80,160). HAUT mounts it and the top is reached 40 px away. */
    {
        int frames;
        memset(map,0,sizeof(map)); row(18);
        map[17*32+10]=12; map[16*32+11]=12; map[15*32+12]=12; map[14*32+13]=12;
        start(&p,160);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.on_stairs && p.x==80 && p.y==160 && p.stair_dir==1 && p.stair_len==40);
        /* Released at once, the first cell is still climbed (6BEE: C435), then
         * Simon stands on the step. */
        for(i=0;i<40;i++) vk_player_tick_controls(&p,map,1,0,0,0,0);
        assert(p.on_stairs && p.x==88 && p.y==152);
        for(frames=0;frames<200 && p.on_stairs;frames++) {
            vk_player_tick_controls(&p,map,1,0,0,0,1);
            assert(p.x-80 == 160-p.y);
        }
        assert(!p.on_stairs && p.x==120 && p.y==120 && p.mode==0);
        /* Mount again, climb to mid-flight, then BAS descends back to the foot. */
        start(&p,160);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        for(frames=0;frames<200 && p.stair_anim!=16;frames++) vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.stair_anim==16);
        for(frames=0;frames<200 && p.on_stairs;frames++) vk_player_tick_controls(&p,map,1,0,0,1,0);
        assert(!p.on_stairs && p.x==80 && p.y==160);
        /* Mount again, then JUMP dismounts into a jump. */
        start(&p,160);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.on_stairs && p.facing==0 && p.pose==0);
        vk_player_tick_controls(&p,map,1,0,1,0,0);
        assert(!p.on_stairs && p.mode==1);
    }
    return 0;
}
