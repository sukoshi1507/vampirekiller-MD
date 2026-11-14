#include <assert.h>
#include "whip.h"
int main(void)
{
    VKPlayer p = {0}; VKWhip w; int l,t,r,b; int i;
    p.x=80; p.y=160;
    vk_whip_reset(&w);
    assert(!vk_whip_active(&w));
    vk_whip_tick(&w,&p,1);
    assert(w.timer==14 && !vk_whip_active(&w));
    assert(vk_whip_arm_frame(&w)==1);
    assert(vk_whip_visual_step(&w)==0);
    /* One test frame per stroke, timer 11 (the MSX pass with C422 = 3). */
    for(i=0;i<2;i++){vk_whip_tick(&w,&p,0); assert(!vk_whip_active(&w));}
    vk_whip_tick(&w,&p,0);
    assert(w.timer==11 && vk_whip_active(&w));
    assert(vk_whip_arm_frame(&w)==1 && vk_whip_visual_step(&w)==1);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(l>=p.x && r>l && t<b);
    for(i=0;i<4;i++){vk_whip_tick(&w,&p,0); assert(!vk_whip_active(&w));}
    assert(vk_whip_arm_frame(&w)==1);
    assert(vk_whip_visual_step(&w)==2);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(r-l==24); /* Visible standing lash box: x+8..x+32. */
    while(w.timer){vk_whip_tick(&w,&p,0); assert(!vk_whip_active(&w));}
    assert(vk_whip_arm_frame(&w)==0);
    assert(vk_whip_visual_step(&w)==0);
    p.facing=1;vk_whip_tick(&w,&p,1);
    for(i=0;i<3;i++)vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(r<=p.x && l<r);
    { int ax,ay; vk_whip_arm_position(&w,&ax,&ay); assert(ax < p.x && ay < p.y); }
    /* Crouched attacks use a lower hit plane, locked at attack start. */
    vk_whip_reset(&w);
    p.facing=0; p.crouched=1;
    vk_whip_tick(&w,&p,1);
    assert(w.crouched==1);
    for(i=0;i<4;i++) vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(t==p.y-17 && b==p.y-10 && l==p.x+8 && r==p.x+32);
    p.crouched=0; p.facing=1; /* posture and direction stay those of the start (7201) */
    vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(t==p.y-17 && b==p.y-10 && l==p.x+8 && r==p.x+32);
    p.facing=0;
    /* The origin follows Simon on every frame (84A7 reads C425/C427 in the test
     * pass): a stroke in a jump tests from Simon's place on the test frame. */
    vk_whip_reset(&w);
    p.x=80; p.y=160;
    vk_whip_tick(&w,&p,1);
    {
        static const int jump_y[4] = {154, 154, 150, 150};
        for(i=0;i<3;i++){p.x=(VKByte)(p.x+1); p.y=(VKByte)jump_y[i]; vk_whip_tick(&w,&p,0);}
        assert(vk_whip_active(&w) && w.x==83 && w.y==150);
        vk_whip_bounds(&w,&l,&t,&r,&b);
        assert(l==91 && r==115 && t==128 && b==135);
        p.x=84; p.y=147; vk_whip_tick(&w,&p,0);
        assert(w.x==84 && w.y==147 && !vk_whip_active(&w));
    }
    /* Exhaustive attack timeline: all visual frame indices remain in their
     * generated-resource ranges, and active hitboxes have visible lash. */
    {
        static const int expected_arm[15] = {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1};
        static const int expected_standing[15] = {0,0,0,2,2,2,2,2,2,2,2,1,1,0,0};
        static const int expected_crouched[15] = {0,0,0,3,3,3,3,3,3,3,3,2,2,1,1};
        for (i=0;i<15;i++) {
            w.timer=(VKByte)i;
            assert(vk_whip_arm_frame(&w)==expected_arm[i]);
            assert(vk_whip_visual_step(&w)==expected_standing[i]);
            assert(vk_whip_crouch_visual_step(&w)==expected_crouched[i]);
            assert(vk_whip_visual_step(&w)>=0 && vk_whip_visual_step(&w)<3);
            assert(vk_whip_crouch_visual_step(&w)>=0 && vk_whip_crouch_visual_step(&w)<4);
            assert(!vk_whip_active(&w) || (i==11 && vk_whip_visual_step(&w)==1));
        }
    }
    /* On a staircase the whip starts as on the floor (the MSX allows it). */
    vk_whip_reset(&w);
    p.on_stairs=1; p.facing=0; p.crouched=0; p.x=100; p.y=150;
    vk_whip_tick(&w,&p,1);
    assert(w.timer==14 && w.x==100 && w.y==150);
    for(i=0;i<3;i++) vk_whip_tick(&w,&p,0);
    assert(vk_whip_active(&w));
    while(w.timer) vk_whip_tick(&w,&p,0);
    p.on_stairs=0;
    /* Whip levels: default 0, levels 1 to 4 accepted (2..4 are thrown), 5 refused.
     * Hit box per level: 0 reaches x +32 (y -22..-15), 1 reaches x +39, and 2
     * reaches x +12 with y -19..-15. */
    vk_whip_reset(&w);
    assert(vk_whip_level(&w)==0);
    /* A hit stops the stroke and keeps the whip. */
    assert(vk_whip_set_level(&w,1)==0);
    w.timer=9;
    vk_whip_stop(&w);
    assert(w.timer==0 && vk_whip_level(&w)==1);
    vk_whip_reset(&w);
    assert(vk_whip_level(&w)==0);
    assert(vk_whip_set_level(&w,5)==-1 && vk_whip_level(&w)==0);
    assert(vk_whip_set_level(&w,-1)==-1 && vk_whip_level(&w)==0);
    p.facing=0; p.crouched=0; p.x=80; p.y=160;
    vk_whip_tick(&w,&p,1);
    for(i=0;i<4;i++) vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(l==88 && r==112 && t==138 && b==145);
    {
        static const int right_reach[3] = {112, 119, 92};
        static const int top_row[3] = {138, 138, 141};
        for(i=1;i<=2;i++){
            assert(vk_whip_set_level(&w,i)==0 && vk_whip_level(&w)==i);
            assert(vk_whip_visual_step(&w)==2 && vk_whip_arm_frame(&w)==1);
            vk_whip_bounds(&w,&l,&t,&r,&b);
            assert(l==88 && r==right_reach[i] && t==top_row[i] && b==145);
        }
    }
    /* Left-facing mirror of level 2: x -12..-8. */
    vk_whip_reset(&w);
    assert(vk_whip_set_level(&w,2)==0);
    p.facing=1;
    vk_whip_tick(&w,&p,1);
    for(i=0;i<4;i++) vk_whip_tick(&w,&p,0);
    vk_whip_bounds(&w,&l,&t,&r,&b);
    assert(l==68 && r==72 && t==141 && b==145);
    /* Crouched attack by level (research/whip-variants.md, "Accroupi": C42E 6, upper 6..8 / 9..11 /
     * 12..14): the hit box is y -17..-10, x +8..+32 for level 0 and x +8..+39 for the chain whip
     * (crouch frame 6/11 reaches x +39), mirrored to the left; the crouch frames 1..3 steps are the
     * same 12 ticks (2/2/8) for levels 0 and 1 and 6 ticks (2/2/2) for the thrown levels. */
    {
        static const int reach[2] = {32, 39};
        static const int steps12[12] = {1,1,2,2,3,3,3,3,3,3,3,3};
        static const int steps6[6] = {1,1,2,2,3,3};
        int lvl, facing, n;
        for(lvl=0;lvl<=2;lvl++) for(facing=0;facing<2;facing++){
            vk_whip_reset(&w);
            assert(vk_whip_set_level(&w,lvl)==0);
            p.facing=facing; p.crouched=1; p.x=80; p.y=160;
            vk_whip_tick(&w,&p,1);
            assert(w.crouched==1 && w.facing==facing);
            if(lvl<2){
                vk_whip_bounds(&w,&l,&t,&r,&b);
                assert(t==143 && b==150);
                assert(facing ? (l==80-reach[lvl] && r==72) : (l==88 && r==80+reach[lvl]));
            }
            /* From the start tick (timer 14) to the last drawn frame (timer 3, or 9 for the throw). */
            n = lvl<2 ? 12 : 6;
            for(i=0;i<n;i++){
                assert(vk_whip_arm_frame(&w)==1);
                assert(vk_whip_crouch_visual_step(&w)==(lvl<2 ? steps12[i] : steps6[i]));
                vk_whip_tick(&w,&p,0);
            }
            assert(vk_whip_crouch_visual_step(&w)==0 || lvl>=2);
        }
        p.crouched=0; p.facing=0;
    }
    /* Body duration per level, counted from attack start (timer 14) until the
     * arm frame returns to 0. Level 0 and 1: 12 frames (2/2/8). Level 2: 6 frames
     * (2/2/2, measured in openMSX), with the standing frames 0,0,1,1,2,2. */
    {
        static const int level2_standing[6] = {0,0,1,1,2,2};
        int frames;
        vk_whip_reset(&w);
        assert(vk_whip_set_level(&w,2)==0);
        p.facing=0; p.crouched=0;
        vk_whip_tick(&w,&p,1);
        for(frames=0; vk_whip_arm_frame(&w); frames++) {
            assert(vk_whip_visual_step(&w)==level2_standing[frames]);
            vk_whip_tick(&w,&p,0);
        }
        assert(frames==6);
        assert(vk_whip_arm_frame(&w)==0 && vk_whip_visual_step(&w)==0);
        /* Level 0 is unchanged: 12 frames with the 2/2/8 standing sequence. */
        vk_whip_reset(&w);
        vk_whip_tick(&w,&p,1);
        for(frames=0; vk_whip_arm_frame(&w); frames++) {
            assert(vk_whip_level(&w)==0);
            assert(vk_whip_visual_step(&w)==(frames < 2 ? 0 : (frames < 4 ? 1 : 2)));
            vk_whip_tick(&w,&p,0);
        }
        assert(frames==12);
        /* 7D7E: no whip test for the thrown weapons (levels 2..4); the throw ends at
         * the 4th pass (timer 8, 7286): vk_whip_tick returns 1 once, timer 0. */
        for(i=2;i<=4;i++){
            int k, launches = 0, frames = 0;
            vk_whip_reset(&w);
            assert(vk_whip_set_level(&w,i)==0);
            assert(vk_whip_tick(&w,&p,1)==0 && w.timer==14);
            for(k=0;k<10;k++){
                assert(!vk_whip_active(&w));
                launches += vk_whip_tick(&w,&p,0);
                frames++;
                if(!w.timer) break;
            }
            assert(launches==1 && frames==(i>=3 ? 9 : 6) && w.timer==0);
        }
        /* Axe and cross (traced, research/throw-timing.md): C422 = 1 idle tick, then upper 12 x2,
         * 13 x3, 14 x3, launch on the 9th tick after the press (the dagger: 6). */
        for(i=3;i<=4;i++){
            static const int heavy[9] = {-1,0,0,1,1,1,2,2,2};
            int k;
            vk_whip_reset(&w);
            assert(vk_whip_set_level(&w,i)==0);
            vk_whip_tick(&w,&p,1);
            for(k=0;k<9;k++){
                assert(vk_whip_arm_frame(&w)==(heavy[k]>=0));
                if(heavy[k]>=0) assert(vk_whip_visual_step(&w)==heavy[k]);
                /* Crouched throw (traced, same C422/C42F as standing, C42E = 6): same steps, crouch frames 1..3. */
                assert(vk_whip_crouch_visual_step(&w)==(heavy[k]>=0 ? heavy[k]+1 : 0));
                vk_whip_tick(&w,&p,0);
            }
            assert(w.timer==0 && vk_whip_arm_frame(&w)==0);
        }
        vk_whip_reset(&w);
        assert(vk_whip_set_level(&w,1)==0);
        vk_whip_tick(&w,&p,1);
        for(i=0;i<3;i++) vk_whip_tick(&w,&p,0);
        assert(vk_whip_active(&w) && w.timer==11);
    }
    vk_whip_reset(&w);
    assert(vk_whip_level(&w)==0);
    return 0;
}
