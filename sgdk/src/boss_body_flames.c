/* Pictures of the 1E death flames of the bodies of stages 3, 6 and 9 (7806 -> 9A3A; research/boss-body-flame.md).
 *
 * openMSX, stages 3, 6, 9: the flame is built at (X, Y - 18h) of the body (table 9B33), drawn as F4 / F8 at (X - 8, Y - 16 / Y - 15) + 1 on screen, colour 8,
 * and stays on screen 34 frames whatever the pass phase: F4 x 6, F8 x 8, F4 x 8, F8 x 8, F4 x 4 (B6AB calls 7806 again about 3 frames after the kill, which
 * kills the flame itself and builds a new one in the same block: 16 passes again).  The hit lands on any frame of the port while the boss passes only on the
 * even ones, and a heavy frame can take two VBlanks: the picture is therefore timed on VBlanks (vtimer), from the first frame where the pass counter
 * flame[k] (boss.c, 16 at the kill) is seen.  The body coordinates stay where they were at the kill (alive = 0: nothing moves them).
 * Kept in a file of its own, with no new field and no change of boss.c / vk_boss_frame, called only for the bodies of stages 3, 6, 9: the frame-exact
 * replay tests/run_playthrough_12_18_md.sh follows the link-time inlining and the cycles of the whole program (see the note in the research file).
 * State: shot_state[1] = vtimer (low byte) of the first frame, shot_state[2] = bit k while the picture of the flame of body k lasts, bit 7 once it is
 * over (the array is used by the stage 15 boss only). */
#include "boss.h"

#define HI(v) ((VKBossU8)((v) >> 8))
#define DONE 0x80

int vk_boss_body_flames(VKBoss *b, VKBossSprite *out, int n, unsigned char now)
{
    int k, e, second;
    if (b->kind > 2) return n;
    if (!b->shot_state[2]) {
        if (!b->flame[0] && !b->flame[1]) return n;
        b->shot_state[2] = (VKBossU8)((b->flame[0] != 0) | ((b->flame[1] != 0) << 1));
        b->shot_state[1] = now;
    }
    if (b->shot_state[2] & DONE) return n;
    e = (unsigned char)(now - b->shot_state[1]);
    if (e >= VK_BOSS_FLAME_FRAMES) { b->shot_state[2] = DONE; return n; }
    second = ((e + 2) >> 3) & 1;   /* F8 for the 6th to 13th and 22nd to 29th frames */
    for (k = 0; k < 2; k++) {
        const int fx = b->kind == 2 ? HI(b->walker[k].x) : HI(b->x), fy = (b->kind == 2 ? HI(b->walker[k].y) : HI(b->y)) - 24;
        if (!(b->shot_state[2] & (1 << k))) continue;
        out[n].x = (short)(fx - 8);
        out[n].y = (short)(fy - (second ? 14 : 15));
        out[n].tile = (unsigned short)(VK_BOSS_SHARED_FLAME_TILE + (second ? 4 : 0));
        out[n].size = 2;
        n++;
    }
    return n;
}
