/* Object 0E of the stage 11 (dragon, A9B7) : vectors of tools/gen_dragon_vectors.py (ROM routines in a Z80 emulator), aim A0E5, rules. research/stage11-enemies.md. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "enemies.c"
#include "gameplay.h"
#include "room_manager.h"
#include "dragon_vectors.h"

int main(void)
{
    EnemyState s;
    EnemyRoomVars r;
    VKGame g = { 0 };
    int i, p, n, bad = 0, passes = 0, shots = 0;
    const int nv = (int)(sizeof dragon_vectors / sizeof dragon_vectors[0]);
    const int na = (int)(sizeof dragon_aims / sizeof dragon_aims[0]);
    memset(&r, 0, sizeof r);
    assert(vk_game_debug_start(&g, 11, 5)); vk_game_room_map(&g);
    r.stage = 11; r.room = 5; r.map = vk_game_collision(&g);
    for (i = 0; i < na; i++) {
        const DragonAim *a = &dragon_aims[i];
        EnemyS16 vy, vx;
        r.simonX = (EnemyU8)a->sx; r.simonY = (EnemyU8)a->sy; r.difficulty = (EnemyU8)a->difficulty;
        dragonAim((EnemyU8)a->dx, (EnemyU8)a->dy, &r, &vy, &vx);
        if ((vy & 0xFFFF) != a->hl || (vx & 0xFFFF) != a->de) { if (bad < 5) printf("aim %d : ROM %04X %04X port %04X %04X\n", i, a->hl, a->de, vy & 0xFFFF, vx & 0xFFFF); bad++; }
    }
    assert(bad == 0);
    for (i = 0; i < nv; i++) {
        const DragonVector *v = &dragon_vectors[i];
        const int span = v->hi - v->lo;
        Enemy *d;
        int k;
        r.difficulty = (EnemyU8)v->difficulty; r.simonY = (EnemyU8)v->sy;
        Enemies_init(&s);
        Enemies_god = 1;
        Enemies_s7R = v->r;
        d = dragonSpawn(&s, 208, 112);
        assert(d);
        k = (int)(d - s.enemy);
        for (p = 0; p < v->passes; p++) {
            const int *row = &dragon_rows[(v->first + p) * DRAGON_ROW];
            const int tt = span ? (p * v->step) % (2 * span) : 0;
            const DragonExt *x = &dg[k];
            int j, ok;
            r.simonX = (EnemyU8)(v->lo + (tt > span ? 2 * span - tt : tt));
            /* the C800 loop (the dragon, then its shot is made and moved at once), then the D700 loop for the older shots */
            for (n = 0; n < ENEMY_ALL; n++) if (Enemies_slot(&s,n)->active && Enemies_slot(&s,n)->type == ENEMY_TYPE_FIREBALL) Enemies_slot(&s,n)->age = 7;   /* older shots */
            dragonStep(&s, d, &r);
            for (n = 0; n < ENEMY_ALL; n++)
                if (Enemies_slot(&s,n)->active && Enemies_slot(&s,n)->type == ENEMY_TYPE_FIREBALL && (Enemies_slot(&s,n)->age == 7 || Enemies_slot(&s,n)->age == 0)) fireballStep(Enemies_slot(&s,n));
            ok = d->state == row[0] && d->x == row[1] && d->y == row[2] && x->c0c == row[3] && x->t18 == row[4] && x->i19 == row[5];
            for (j = 0; j < 8; j++) ok = ok && x->tgt[j] == row[6 + j];
            for (j = 0; j < 8; j++) ok = ok && x->ent[j][0] == row[14 + 4 * j] && x->ent[j][1] == row[15 + 4 * j] && x->ent[j][2] == row[16 + 4 * j] && x->ent[j][3] == row[17 + 4 * j];
            if (!ok) { fprintf(stderr, "vector %d pass %d : state ROM %d port %d, X %d/%d, Y %d/%d, +C %d/%d\n", i, p, row[0], d->state, row[1], d->x, row[2], d->y, row[3], x->c0c); bad++; break; }
            if (row[46]) {      /* the first D700 block of the ROM : the oldest live shot */
                int found = 0;
                for (n = 0; n < ENEMY_ALL; n++) {
                    const Enemy *b = Enemies_slot(&s,n);
                    if (b->active && b->type == ENEMY_TYPE_FIREBALL && b->x == row[47] && b->y == row[48] && b->vy == (EnemyS16)row[50] &&
                        (EnemyS16)((b->pose << 8) | b->anim) == (EnemyS16)row[49]) found = 1;
                }
                if (!found) { fprintf(stderr, "vector %d pass %d : shot (%d,%d) vx %d vy %d not found\n", i, p, row[47], row[48], row[49], row[50]); bad++; break; }
                shots++;
            }
        }
        passes += v->passes;
    }
    assert(bad == 0 && shots > 100);
    assert(isObjS4(ENEMY_TYPE_DRAGON) && hasHp(ENEMY_TYPE_DRAGON) && s4Score(ENEMY_TYPE_DRAGON) == 10 && satCount(ENEMY_TYPE_DRAGON) == 8);
    printf("object 0E (dragon of the stage 11): %d vectors, %d passes, %d aims against the ROM routines, rules PASS\n", nv, passes, na);
    return 0;
}
