/* Test adapter: calls the production functions, exposes their Z80 field mapping. */
#include <string.h>
#include "../sgdk/src/enemies.c"
static EnemyState state10;
static EnemyRoomVars vars10;
static EnemySpawnTimer timer10;
void ten_init(const unsigned char *map, int stage, int room, int x, int y, int difficulty)
{
    Enemies_init(&state10);
    memset(&vars10, 0, sizeof(vars10));
    vars10.map = map; vars10.stage = (EnemyU8)stage; vars10.room = (EnemyU8)room;
    vars10.difficulty = (EnemyU8)difficulty;
    type10Spawn(&state10, (EnemyU8)x, (EnemyU8)y);
    Enemies_spawnTimerInit(&timer10);
    state10.type5Timer = ENEMY_TYPE5_DELAY;
    state10.spawnPause = 255;
    Enemies_god = 1;
}
void ten_step(int sx, int sy, int frame)
{
    (void)frame;
    vars10.simonX = (EnemyU8)sx; vars10.simonY = (EnemyU8)sy;
    Enemies_frame(&state10, &timer10, &vars10);
    Enemies_frame(&state10, &timer10, &vars10);
}
int ten_record(int slot, unsigned char *b, int *sprites)
{
    int i, k = 0, n; Enemy *e = &state10.enemy[0]; S8Ext *q; EnemySprite sat[ENEMY_SPRITES];
    if (slot >= 0) {
        for (i = 1; i < ENEMY_ALL; i++)
            if (Enemies_slot(&state10,i)->active && Enemies_slot(&state10,i)->type == ENEMY_TYPE_10_SHOT && ext(&state10, Enemies_slot(&state10,i))->c10 == slot) { k = i; break; }
        if (!k) return 0;
        e = Enemies_slot(&state10,k);
    }
    q = ext(&state10, e); memset(b, 0, 32);
    b[0] = e->active ? (slot < 0 ? 0x10 : 9) : 0;
    b[1] = slot < 0 ? e->state : 0;
    b[2] = e->sub; b[3] = e->y; b[4] = e->timer; b[5] = (EnemyU8)e->x;
    b[6] = q->mobile; b[7] = (EnemyU8)e->vy; b[8] = (EnemyU8)(e->vy >> 8);
    b[9] = (EnemyU8)q->vx; b[10] = (EnemyU8)(q->vx >> 8); b[11] = e->pose;
    if (slot < 0) { b[13] = e->extraPhase; b[14] = e->active ? 7 : 0; b[16] = q->c10; b[17] = q->c11; }
    else { b[12] = q->c12; b[16] = e->state; b[17] = 0; b[18] = 0xC8; b[19] = q->c11; }
    n = Enemies_sprites(&state10, (EnemyU8)k, sat);
    for (i = 0; i < n; i++) { sprites[i*3] = (EnemyU8)(sat[i].y - 1); sprites[i*3+1] = (EnemyU8)sat[i].x; sprites[i*3+2] = sat[i].pattern; }
    return n;
}
int ten_contact(int sx, int sy, int posture)
{
    vars10.simonX = (EnemyU8)sx; vars10.simonY = (EnemyU8)sy; vars10.simonState = (EnemyU8)posture;
    return Enemies_touches(&state10.enemy[0], &vars10);
}
int ten_whip(int sx, int sy, int left, int crouch, int chain)
{
    int hits; Enemy saved = state10.enemy[0];
    Enemies_whipHitStage4(&state10, sx, sy, left, crouch, chain, &hits);
    state10.enemy[0] = saved;
    return hits;
}
