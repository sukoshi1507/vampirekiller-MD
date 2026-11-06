/* Stage 7 objects 0B (skeleton), 0C, 0D (flea) and the bone, pass by pass against the ROM routines run in a Z80
 * emulator (tools/gen_stage7_vectors.py), plus the contact, whip, shot and score rules read in the ROM. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enemies.h"
#include "gameplay.h"
#include "generated/enemy_rooms.h"
#include "stage7_vectors.h"

typedef struct { int x, y, pose; } Bone3;

static int cmp_bone(const void *a, const void *b)
{
    const Bone3 *p = a, *q = b;
    if (p->x != q->x) return p->x - q->x;
    if (p->y != q->y) return p->y - q->y;
    return p->pose - q->pose;
}

static void setup(EnemyState *s, EnemySpawnTimer *t, EnemyRoomVars *r, VKGame *g, int room, int sx, int sy)
{
    memset(g, 0, sizeof *g);
    assert(vk_game_debug_start(g, 7, room));
    vk_game_room_map(g);
    Enemies_init(s);
    Enemies_spawnTimerInit(t);
    memset(r, 0, sizeof *r);
    r->stage = 7; r->room = (EnemyU8)room; r->simonX = (EnemyU8)sx; r->simonY = (EnemyU8)sy;
    r->map = vk_game_collision(g);
}

static int run_vector(const S7Vector *v, int bonePos)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r;
    VKGame g;
    static const EnemyU8 types[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ENEMY_TYPE_HOPPER, ENEMY_TYPE_LEAPER, ENEMY_TYPE_FLEA };
    int p, i, target = -1, span = v->hi - v->lo, bad = 0;
    setup(&s, &t, &r, &g, v->room, v->lo, v->sy);
    Enemies_s7R = (EnemyS16)v->r;
    Enemies_god = 1;                                        /* the vectors have no contact */
    Enemies_frame(&s, &t, &r);                              /* the room is loaded : the fixed objects are created */
    for (i = 0; i < ENEMY_MAX; i++)
        if (s.enemy[i].active && s.enemy[i].type == types[v->type] && s.enemy[i].x == v->x && s.enemy[i].y == v->y)
            target = i;
    assert(target >= 0);
    for (p = 0; p < v->passes; p++) {
        const int row = (v->first + p) * 6;
        const int tt0 = span ? (p * v->step) % (2 * span) : 0;
        const Enemy *e = &s.enemy[target];
        Bone3 want[8], got[8];
        int nb = s7_rows[row + 4], ng = 0, ok = 1;
        r.simonX = (EnemyU8)(v->lo + (tt0 > span ? 2 * span - tt0 : tt0));
        Enemies_frame(&s, &t, &r);                          /* the pass */
        Enemies_frame(&s, &t, &r);
        if ((s7_rows[row + 5] != 0) != (e->active != 0)) ok = 0;
        else if (e->active && (e->state != s7_rows[row] || e->x != s7_rows[row + 1] || e->y != s7_rows[row + 2] ||
                               e->pose != s7_rows[row + 3])) ok = 0;
        if (v->type == 0x0B) {
            for (i = 0; i < nb; i++) {
                want[i].x = s7_bones[bonePos + 3 * i]; want[i].y = s7_bones[bonePos + 3 * i + 1];
                want[i].pose = s7_bones[bonePos + 3 * i + 2];
            }
            for (i = 0; i < ENEMY_MAX; i++)
                if (s.enemy[i].active && s.enemy[i].type == ENEMY_TYPE_BONE && ng < 8) {
                    got[ng].x = s.enemy[i].x; got[ng].y = s.enemy[i].y; got[ng].pose = s.enemy[i].pose; ng++;
                }
            qsort(want, (size_t)nb, sizeof want[0], cmp_bone);
            qsort(got, (size_t)ng, sizeof got[0], cmp_bone);
            if (ng != nb || memcmp(want, got, (size_t)nb * sizeof want[0])) ok = 0;
        }
        bonePos += 3 * nb;
        if (!ok) {
            if (bad < 3)
                printf("room %d type %02X R %d pass %d : ROM st %d (%d,%d) pose %02X bones %d alive %d, port st %d (%d,%d) pose %02X active %d\n",
                       v->room, v->type, v->r, p, s7_rows[row], s7_rows[row + 1], s7_rows[row + 2], s7_rows[row + 3],
                       nb, s7_rows[row + 5], e->state, e->x, e->y, e->pose, e->active);
            if (bad < 3 && v->type == 0x0B) {
                int z;
                for (z = 0; z < nb; z++) printf("   want bone %d,%d,%02X\n", want[z].x, want[z].y, want[z].pose);
                for (z = 0; z < ng; z++) printf("   got  bone %d,%d,%02X\n", got[z].x, got[z].y, got[z].pose);
            }
            bad++;
        }
    }
    return bad;
}

static int find(const EnemyState *s, EnemyU8 type, int bone)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == type && (type != ENEMY_TYPE_BONE || bone)) return i;
    return -1;
}

/* Contact damage (81CE pairs doubled : 2 for 0B, 0C, 0D ; 1 for the bone, 7F3D), hit points, scores and boxes (82CB, 8378, 8390,
 * 8358) read in the ROM ; damage 2 and 1 also seen in the openMSX trace (research/stage7-enemies.md). */
static void rules(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars r;
    VKGame g;
    int i, k, hits, n;
    struct { int room, type; EnemyU8 etype; int hp, score; } cases[3] = {
        { 2, 0x0B, ENEMY_TYPE_HOPPER, 4, 2 }, { 1, 0x0D, ENEMY_TYPE_FLEA, 1, 2 }, { 7, 0x0C, ENEMY_TYPE_LEAPER, 1, 1 } };
    Enemies_god = 0;
    Enemies_s7R = 0;
    for (k = 0; k < 3; k++) {
        int e;
        setup(&s, &t, &r, &g, cases[k].room, 232, 176);
        Enemies_frame(&s, &t, &r);
        e = find(&s, cases[k].etype, 0);
        assert(e >= 0);
        /* fixed list (enemy_rooms.h) : the object is where the ROM list puts it */
        assert(s.enemy[e].extraPhase == cases[k].hp);
        /* contact box : |dX| < L + 5, Y from the object foot */
        {
            const int l = k == 0 ? 10 : 13;
            Enemy c = s.enemy[e];
            r.simonX = (EnemyU8)(c.x + l - 1); r.simonY = c.y;
            assert(Enemies_touches(&c, &r));
            r.simonX = (EnemyU8)(c.x + l); assert(!Enemies_touches(&c, &r));
            r.simonX = (EnemyU8)(c.x - l + 1); assert(Enemies_touches(&c, &r));
            r.simonX = (EnemyU8)(c.x - l); assert(!Enemies_touches(&c, &r));
        }
        /* whip : cases[k].hp strokes of 1, or half of them with the chain whip, then the score */
        for (n = 0; n < cases[k].hp; n++) {
            const Enemy c = s.enemy[e];
            assert(Enemies_whipHitStage4(&s, c.x - 20, c.y + 12, 0, 0, 0, &hits) == (n == cases[k].hp - 1) && hits == 1);
            Enemies_rearm(&s);
        }
        assert(!Enemy_alive(s.enemy[e]) && Enemies_killScore == cases[k].score);
        setup(&s, &t, &r, &g, cases[k].room, 232, 176);
        Enemies_frame(&s, &t, &r);
        e = find(&s, cases[k].etype, 0);
        for (n = 0; n < (cases[k].hp + 1) / 2; n++) {
            const Enemy c = s.enemy[e];
            assert(Enemies_whipHitStage4(&s, c.x + 24, c.y + 12, 1, 0, 1, &hits) == (n == (cases[k].hp + 1) / 2 - 1) && hits == 1);
            Enemies_rearm(&s);
        }
        assert(!Enemy_alive(s.enemy[e]));
        /* contact : damage 2, recoil, invulnerability */
        setup(&s, &t, &r, &g, cases[k].room, 232, 176);
        Enemies_frame(&s, &t, &r);
        e = find(&s, cases[k].etype, 0);
        r.simonX = s.enemy[e].x; r.simonY = s.enemy[e].y;
        for (i = 0; i < 4 && s.energy == 32; i++) Enemies_frame(&s, &t, &r);
        assert(s.energy == 30 && s.invuln);
    }
    /* Bone : thrown by the skeleton (room 2, R = 0, first throw on pass 55), box (6, 12), 100 points, damage 1, vanishes on contact. */
    setup(&s, &t, &r, &g, 2, 232, 176);
    Enemies_god = 1;
    Enemies_frame(&s, &t, &r);
    for (i = 0; i < 2 * 55 && find(&s, ENEMY_TYPE_BONE, 1) < 0; i++) Enemies_frame(&s, &t, &r);
    k = find(&s, ENEMY_TYPE_BONE, 1);
    assert(k >= 0 && s.enemy[k].x == 217 && s.enemy[k].y == 171);
    {
        const Enemy c = s.enemy[k];
        assert(Enemies_whipHitStage4(&s, c.x - 20, c.y + 12, 0, 0, 0, &hits) == 1 && hits == 1 && Enemies_killScore == 1);
        assert(!s.enemy[k].active);
    }
    setup(&s, &t, &r, &g, 2, 232, 176);
    Enemies_god = 0;
    Enemies_frame(&s, &t, &r);
    r.simonX = 232; r.simonY = 176;
    for (i = 0; i < 2 * 70 && find(&s, ENEMY_TYPE_BONE, 1) < 0; i++) Enemies_frame(&s, &t, &r);
    k = find(&s, ENEMY_TYPE_BONE, 1);
    assert(k >= 0);
    for (i = 0; i < 80 && s.energy == 32; i++) Enemies_frame(&s, &t, &r);
    assert(s.energy == 31 && !s.enemy[k].active);
    Enemies_god = 1;
    Enemies_s7R = -1;
}

/* LD A,R stand-in: traced values are < 80h with uniform low bits (research/hasard-R.md). */
static void random_r(void)
{
    static int count[128];
    int i, low2 = 0, bit0 = 0, hi4[16] = { 0 };
    Enemies_s7R = -1;
    for (i = 0; i < 65536; i++) {
        int a = Enemies_randomR();
        assert(a < 0x80);
        count[a]++;
        if ((a & 3) == 0) low2++;
        bit0 += a & 1;
        hi4[a >> 3]++;
    }
    for (i = 0; i < 128; i++) assert(count[i] == 512);
    assert(low2 == 16384 && bit0 == 32768);
    for (i = 0; i < 16; i++) assert(hi4[i] == 4096);
    Enemies_s7R = 0x44;
    assert(Enemies_randomR() == 0x44);
    Enemies_s7R = -1;
}

int main(void)
{
    random_r();
    rules();
    int q, bad = 0, rows = 0, bonePos = 0;
    for (q = 0; q < (int)(sizeof s7_vectors / sizeof s7_vectors[0]); q++) {
        int k;
        bad += run_vector(&s7_vectors[q], bonePos);
        rows += s7_vectors[q].passes;
        for (k = 0; k < s7_vectors[q].passes; k++) bonePos += 3 * s7_rows[(s7_vectors[q].first + k) * 6 + 4];
    }
    printf("Stage 7 : %d passes against the ROM, %d differences\n", rows, bad);
    assert(bad == 0);
    printf("Stage 7 tests: PASS\n");
    return 0;
}
