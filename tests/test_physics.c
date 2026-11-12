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
    /* Rising past a ledge (6D13: floor test from step 9, 6D3B: Y &= F8): hall room 4,
     * ledge x 0-31 at y 160, Simon at (34, 192): the probe x - 5 is on the ledge at
     * the apex, he lands at 160 on step 9 (openMSX trace), never 1 px into it. */
    memset(map,0,sizeof(map)); row(22);
    for(i=0;i<4;i++) map[18*32+i]=1;
    start(&p,192); p.x=34;
    vk_player_tick_map(&p,map,1,0,1);
    for(i=0;i<19 && p.mode==1;i++) vk_player_tick_map(&p,map,1,0,0);
    assert(p.mode==1 && p.y==161 && p.phase==8);
    vk_player_tick_map(&p,map,1,0,0);
    assert(p.mode==0 && p.y==160);
    for(i=0;i<40;i++) vk_player_tick_map(&p,map,1,0,0);
    assert(p.mode==0 && p.y==160);
    /* Same jump at x 60, head under the ledge row only: back on the floor at 192. */
    start(&p,192); p.x=60;
    vk_player_tick_map(&p,map,1,0,1);
    for(i=0;i<60;i++) vk_player_tick_map(&p,map,1,0,0);
    assert(p.mode==0 && p.y==192);
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
    vk_player_tick_map(&p,map,1,1,0);   /* the walk moves on every second frame (30 Hz pass) */
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
        /* 30 Hz pass of the stairs (openMSX, tools/run_jump_cases.py: stage 15 room 1 foot,
         * UP at n = 1, C420 = 3 at once, X and Y change at n = 3, 5, 7...): the entry frame
         * starts the pass whatever the walk clock was, the first step comes 2 frames later. */
        start(&p,160);
        p.clock = 1;
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.on_stairs && p.y==160);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.y==160);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.y==159 && p.x==81);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.y==159);
        vk_player_tick_controls(&p,map,1,0,0,0,1);
        assert(p.y==158 && p.x==82);
    }
    /* Horizontal motion of a jump (6D61 / 6D67, openMSX traces of tools/run_jump_cases.py, stage 15
     * room 3 floor y 176): the game pass moves X by 2 px on the frame of the vertical step and
     * before it. After the key frame (n = 1: 2 px of the walk pass) X does not move until the first step
     * (n = 5), then 2 px every second frame, the Y steps being those of the table 6D95. */
    {
        static const VKByte y_of_step[19] = {170,164,159,154,154,150,147,145,144,144,144,145,147,150,154,159,164,170,176};
        int n;
        memset(map,0,sizeof(map)); row(20);
        start(&p,176); p.x=200;
        vk_player_tick_map(&p,map,1,-1,1);
        assert(p.mode==1 && p.x==198 && p.y==176);   /* the pass of the key walks 2 px first (MSX n = 1: 198) */
        for(n=2;n<=41;n++) {
            vk_player_tick_map(&p,map,1,-1,0);
            if(n<5) assert(p.x==198 && p.y==176);
            else {
                const int step=(n-5)/2;   /* 0 at n = 5, 6 */
                assert(p.x==198-2*(step+1));
                if(n<41) assert(p.mode==1 && p.y==y_of_step[step]);
            }
        }
        assert(p.mode==0 && p.y==176);
    }
    /* Landing on a ledge at the apex of a jump that moves towards it (openMSX, stage 15 room 3:
     * the block left of the stairs, top y 144, Simon from the floor y 176 at x 136 with LEFT).
     * The wall probe 7C05 looks at the Y before the step and moves X by 2 at once, so X is 132 and
     * the floor under him on step 9 (frame 21); a 1 px move a frame, after the Y step, lands at 23. */
    {
        int n, c, r;
        memset(map,0,sizeof(map)); row(20);
        for(r=16;r<20;r++) for(c=0;c<16;c++) map[r*32+c]=1;
        start(&p,176); p.x=136;
        vk_player_tick_map(&p,map,1,-1,1);
        for(n=2;n<=20;n++) vk_player_tick_map(&p,map,1,-1,0);
        assert(p.mode==1 && p.y==145 && p.x==134);   /* the wall stopped X at the first step */
        vk_player_tick_map(&p,map,1,-1,0);
        assert(p.mode==0 && p.y==144 && p.x==132);
    }
    /* 7C5E, CE00 = 6 (Dracula room, ROM read in openMSX: 7C5F CP 6 / 7C73 DEC A, CP 6): only tiles 1..6 are solid, the
     * tiles 7 and 8 of the towers are decor; with CE00 != 6 the stage 18 limit is 8. */
    assert(vk_tile_solid(6,18,0) && vk_tile_solid(7,18,0) && vk_tile_solid(8,18,0) && !vk_tile_solid(9,18,0));
    assert(vk_tile_solid(6,VK_STAGE_CE00_6,0) && !vk_tile_solid(7,VK_STAGE_CE00_6,0) && !vk_tile_solid(8,VK_STAGE_CE00_6,0));
    return 0;
}
