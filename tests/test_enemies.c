/*
 * Tests natifs de la logique des ennemis (sgdk/src/enemies.c).
 * Compilation : gcc -std=c99 -Wall -Wextra -Werror -pedantic -I sgdk/src \
 *   tests/test_enemies.c sgdk/src/enemies.c -o /tmp/test_enemies && /tmp/test_enemies
 *
 * Trois familles :
 *   1. Tables ROM (valeurs lues dans la ROM, voir research/enemy-spawn.md section 7).
 *   2. Minuterie et garde de spawn : code Z80 (9CC3, 9CE6, 9CFC, 9E16) rejoue ici.
 *      Intervalles verifies contre la trace : 104 images (908 -> 1012) et 152 (1012 -> 1208).
 *   3. Trajectoire E1/E2 : valeurs lues dans la trace openMSX (frames.txt).
 *      Image absolue n = 857 + image relative. Les images de spawn 908 et 1012 sont
 *      ici des donnees d'observation du driver de test, pas une sortie de la minuterie.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "enemies.h"
#include "generated/stage2_maps.h"

static int failures = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            failures++;                                   \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);   \
            printf(__VA_ARGS__);                          \
            printf("\n");                                 \
        }                                                 \
    } while (0)

#define ROOM_LOAD_IMAGE 857
#define SPAWN_E1_REL 51    /* image 908 */
#define SPAWN_E2_REL 155   /* image 1012 */
#define ENEMY_SPAWN_X_RIGHT_TEST 0xE5  /* SAT X observe au spawn droit (909) */
#define ENEMY_SPAWN_X_LEFT_TEST  0x0A  /* SAT X observe au spawn gauche (1013) */

/* Driver de test : une image = Enemies_update, puis spawns observes (908 et 1012), marques
 * romBox comme ceux de Enemies_spawnTimerTick (objet de la ROM, sortie 99E5). */
static void frame_step(EnemyState *s, int *rf)
{
    int k = -1;
    Enemies_update(s);
    (*rf)++;
    if (*rf == SPAWN_E1_REL) k = Enemies_spawn(s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    if (*rf == SPAWN_E2_REL) k = Enemies_spawn(s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    if (k >= 0) s->enemy[k].romBox = 1;
}

static void advance_to(EnemyState *s, int *rf, int n)
{
    while (*rf < n - ROOM_LOAD_IMAGE) frame_step(s, rf);
}

/* 1. Tables ROM. */
static void test_rom_tables(void)
{
    static const uint8_t period_expected[8] = { 0x0C, 0x12, 0x0C, 0x0C, 0x0C, 0x12, 0x0C, 0x0C };
    static const uint8_t flags_expected[8] = { 0x01, 0x01, 0x00, 0x00, 0x08, 0x01, 0x01, 0x00 };
    int i;
    for (i = 0; i < 8; i++) {
        CHECK(kEnemySpawnPeriod[i] == period_expected[i], "periode[%d] attendue %02X", i, period_expected[i]);
        CHECK(kEnemyStage1RoomFlags[i] == flags_expected[i], "drapeau salle %d attendu %02X", i, flags_expected[i]);
    }
    CHECK(ENEMY_TYPE1_SAT_COUNT == 4, "type 1 : 4 entrees SAT (ROM 6058)");
}

/* Environnement de minuterie : Simon a x=0x7A (pas de refus), cote droit par defaut. */
static void vars_default(EnemyRoomVars *v)
{
    v->stage = 1;
    v->room = 0;
    v->simonX = 0x7A;
    v->simonY = 0xC0;
    v->sideFlag = 0;
    v->difficulty = 0;
    v->simonState = ENEMY_SIMON_GROUND;
    v->map = 0;
}

/* 2a. Intervalles : tirs aux appels 16, 68, 144, 196 (un appel = 2 images). */
static void test_timer_intervals(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int call, fires[4], nfire = 0;

    vars_default(&v);
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 200 && nfire < 4; call++) {
        if (Enemies_spawnTimerTick(&t, &v, &s)) {
            fires[nfire++] = call;
        }
    }
    CHECK(nfire == 4, "4 tirs attendus en 200 appels, obtenu %d", nfire);
    if (nfire == 4) {
        CHECK(fires[0] == 16, "premier tir a l'appel 16, obtenu %d", fires[0]);
        CHECK(fires[1] == 68, "second tir a l'appel 68, obtenu %d", fires[1]);
        CHECK(fires[2] == 144, "troisieme tir a l'appel 144, obtenu %d", fires[2]);
        CHECK(fires[3] == 196, "quatrieme tir a l'appel 196, obtenu %d", fires[3]);
        /* Intervalles en images (2 images par appel) : 104 et 152, comme la trace. */
        CHECK((fires[1] - fires[0]) * 2 == 104, "intervalle 1 attendu 104 images");
        CHECK((fires[2] - fires[1]) * 2 == 152, "intervalle 2 attendu 152 images");
    }
}

/* 2b. Reload et garde de cote : periode - 2*D012 + 1. */
static void test_timer_reload_and_difficulty(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int call;

    vars_default(&v);
    v.difficulty = 1;
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(t.counter == (uint8_t)(0x0C - 2 + 1), "reload avec D012=1 attendu %d, obtenu %d", 0x0C - 2 + 1, t.counter);
    CHECK(t.index == 1, "index CF01 apres premier tir : 1");
}

/* 2c. Drapeau : pas de minuterie si bit0 = 0 (salle D001 = 2). */
static void test_flag_gate(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int call, fired = 0;
    uint8_t before;

    vars_default(&v);
    v.room = 2;
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    before = t.counter;
    for (call = 0; call < 100; call++) fired += Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(fired == 0 && t.counter == before && t.phase == 0,
          "salle sans drapeau : minuterie figee et aucun tir");

    /* Stage autre que 1 : non porte, aucun tir. */
    vars_default(&v);
    v.stage = 0;
    for (call = 0; call < 100; call++) fired += Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(fired == 0, "stage != 1 : pas de tir (non porte)");
}

/* 2d. Garde 9E16 : refus a droite quand Simon est a droite (x >= C0, D >= C0). */
static void test_guard_right_refused(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int call, fired = 0;
    int idx_before;

    vars_default(&v);
    v.simonX = 0xCE;   /* Simon a droite ; D = F0 (sideFlag 0) : refus */
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) fired += Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(fired == 0, "spawn droit refuse quand Simon x=CE");
    CHECK(!s.enemy[0].active, "aucun ennemi cree au refus");
    /* Le refus consomme la minuterie : CF01 avance et CF00 est recharge (9CE6 apres 9CC3). */
    idx_before = t.index;
    CHECK(idx_before == 1, "CF01 incremente meme en cas de refus (attendu 1)");
    CHECK(t.counter == 0x0D, "CF00 recharge a 0D en cas de refus");
}

/* 2e. Garde 9E16 : refus a gauche quand Simon est a gauche (x < 40, D < 40). */
static void test_guard_left_refused(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int call, fired = 0;

    vars_default(&v);
    v.simonX = 0x28;
    v.sideFlag = 1;    /* D = 10 : refus */
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) fired += Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(fired == 0, "spawn gauche refuse quand Simon x=28");

    /* Meme position, spawn droit : autorise (D = F0 >= 40). */
    vars_default(&v);
    v.simonX = 0x28;
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    fired = 0;
    for (call = 1; call <= 16; call++) fired += Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(fired == 1 && s.enemy[0].active && s.enemy[0].dx == -1 && s.enemy[0].x == ENEMY_SPAWN_X_RIGHT_TEST,
          "spawn droit autorise quand Simon x=28");
}

/* 2f. Spawn gauche : cote selon C42C ; le cote donne le type (B a gauche). */
static void test_side_left(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int call;

    vars_default(&v);
    v.sideFlag = 1;   /* D = 10 : spawn a gauche */
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(s.enemy[0].active && s.enemy[0].dx == 1, "spawn a gauche : dx=+1 attendu");
    CHECK(s.enemy[0].x == ENEMY_SPAWN_X_LEFT_TEST, "spawn a gauche : x=0A attendu");
    CHECK(s.enemy[0].type == ENEMY_TYPE_B, "cote gauche : type B");
    CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_SPRITES && sp[0].pattern == 0x58, "motif type B : 58");
    CHECK(s.enemy[0].y == 0xA0, "Y = C0 - 20 = A0 attendu");
    CHECK(s.enemy[0].romBox == 1, "hall : objet de la ROM (romBox), comme au stage 2");
}

/* 2g. Salle D001 = 1 avec Simon haut (C425 < 88) : E = 60, D = F0. Code seul. */
static void test_room1_low_simon(void)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int call;

    vars_default(&v);
    v.room = 1;
    v.simonY = 0x60;   /* < 88 : DE = F060 (9D38) */
    v.sideFlag = 1;    /* ignore dans ce cas */
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(s.enemy[0].active && s.enemy[0].dx == -1, "E=60 : spawn a droite (D=F0), pas de cote C42C");
    CHECK(s.enemy[0].y == 0x40, "E=60 : Y = 60 - 20 = 40 (code, sprite non verifie)");
}

/* 3. Trajectoire E1 : spawn image 908 (observe), trace frames.txt. */
static void test_e1_trajectory(void)
{
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int rf = 0;
    typedef struct { int n; int x; uint8_t bottom; } Expect;
    static const Expect e1[] = {
        { 909, 229, 0x48 },  /* E5, pose A */
        { 911, 227, 0x48 },  /* E3 */
        { 913, 225, 0x48 },  /* E1 */
        { 923, 215, 0x50 },  /* D7 : pose B commence a l'age 15 */
        { 925, 212, 0x50 },  /* D4 : pas supplementaire a l'age 17 */
        { 931, 206, 0x48 },  /* CE : retour pose A */
        { 941, 195, 0x50 },  /* C3 */
        { 1125, 0,  0x48 },  /* 00 (trace du hall, ombre SAT D638 : 00 a l'age 217) */
        { 1126, 0,  0x48 },  /* 00 tenu une image de plus (99E5 a l'age pair, comme au stage 2) */
    };
    size_t i;

    Enemies_setRoom(&s);
    advance_to(&s, &rf, 908);
    CHECK(s.enemy[0].active && s.enemy[0].age == 0, "E1 doit etre actif a l'age 0 (image 908)");
    CHECK(s.enemy[0].dx == -1, "E1 spawn a droite doit avancer vers la gauche");

    for (i = 0; i < sizeof(e1) / sizeof(e1[0]); i++) {
        advance_to(&s, &rf, e1[i].n);
        CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_SPRITES, "E1 inactif a l'image %d", e1[i].n);
        CHECK(sp[0].x == e1[i].x, "E1 x a l'image %d : attendu %d, obtenu %d", e1[i].n, e1[i].x, sp[0].x);
        CHECK(sp[0].y == 0xA0 && sp[1].y == 0xA0, "E1 rang haut doit etre en Y=A0 (image %d)", e1[i].n);
        CHECK(sp[2].y == 0xB0 && sp[3].y == 0xB0, "E1 rang bas doit etre en Y=B0 (image %d)", e1[i].n);
        CHECK(sp[0].pattern == 0x40 && sp[1].pattern == 0x44, "E1 motifs haut (image %d)", e1[i].n);
        CHECK(sp[2].pattern == e1[i].bottom, "E1 motif bas a l'image %d : attendu %02X, obtenu %02X",
              e1[i].n, e1[i].bottom, sp[2].pattern);
    }

    advance_to(&s, &rf, 1127);
    CHECK(!s.enemy[0].active, "E1 doit etre inactif a l'image 1127");
    CHECK(Enemies_sprites(&s, 0, sp) == 0, "E1 inactif ne doit rien produire");
}

/* 3. Trajectoire E2 : spawn image 1012 (observe), avance vers la droite. */
static void test_e2_trajectory(void)
{
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int rf = 0;
    typedef struct { int n; int x; uint8_t bottom; } Expect;
    static const Expect e2[] = {
        { 1013, 10, 0x60 },  /* 0A, pose A */
        { 1015, 12, 0x60 },  /* 0C */
        { 1027, 25, 0x68 },  /* 19 : pose B a l'age 15, pas supplementaire */
        { 1035, 33, 0x60 },  /* 21 */
        { 1043, 42, 0x68 },  /* 2A */
        { 1207, 216, 0x68 }, /* D8 : ancienne borne du port, l'objet continue */
        { 1221, 231, 0x68 }, /* E7 (trace du hall : E7 a l'age 209, objet supprime a X F1.25) */
        { 1222, 231, 0x68 }, /* E7 tenu une image de plus (99E5 a l'age pair) */
    };
    size_t i;

    Enemies_setRoom(&s);
    advance_to(&s, &rf, 1012);
    advance_to(&s, &rf, 1013);
    CHECK(s.enemy[1].active && s.enemy[1].dx == 1, "E2 doit etre actif, avance vers la droite");

    for (i = 0; i < sizeof(e2) / sizeof(e2[0]); i++) {
        advance_to(&s, &rf, e2[i].n);
        CHECK(Enemies_sprites(&s, 1, sp) == ENEMY_SPRITES, "E2 inactif a l'image %d", e2[i].n);
        CHECK(sp[0].x == e2[i].x, "E2 x a l'image %d : attendu %d, obtenu %d", e2[i].n, e2[i].x, sp[0].x);
        CHECK(sp[0].pattern == 0x58 && sp[1].pattern == 0x5C, "E2 motifs haut (image %d)", e2[i].n);
        CHECK(sp[2].pattern == e2[i].bottom, "E2 motif bas a l'image %d : attendu %02X, obtenu %02X",
              e2[i].n, e2[i].bottom, sp[2].pattern);
    }

    advance_to(&s, &rf, 1223);
    CHECK(!s.enemy[1].active, "E2 doit etre inactif a l'image 1223");
}

/* 4. Limites de la liste d'ennemis et de la visibilite. */
static void test_spawn_limits(void)
{
    EnemyState s;
    int idx, k;

    Enemies_setRoom(&s);
    for (k = 0; k < ENEMY_MAX; k++) {
        idx = Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
        CHECK(idx == k, "spawn %d attendu en slot %d, obtenu %d", k, k, idx);
    }
    idx = Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    CHECK(idx == -1, "spawn sur liste pleine doit echouer (obtenu %d)", idx);

    CHECK(Enemies_isVisible(&s.enemy[0]), "x=E5 doit etre visible (spawn droit)");
    s.enemy[0].x = -1;
    CHECK(!Enemies_isVisible(&s.enemy[0]), "x=-1 ne doit pas etre visible");
}

/* 5. Type 4 : 3 px toutes les 2 images, courbe Y, 4 poses de 16 images (mesure image par
 * image en salle 4, scratchpad/type4/gfx) ; sortie apres X 01 (gauche) et E6 (droite). */
static void test_type4_motion(void)
{
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int i;
    const EnemyU8 T4 = ENEMY_PATTERN_TYPE4;

    Enemies_setRoom(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_4, ENEMY_TYPE4_Y, 0);
    CHECK(s.enemy[0].x == 0xE5 && s.enemy[0].dx == -1, "type 4 droite : x=E5, dx=-1");
    CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_TYPE4_SPRITES, "type 4 : deux entrees SAT");
    CHECK(sp[0].pattern == (T4 | 0x48) && sp[1].pattern == (T4 | 0x4C) && sp[0].y == 0xAA,
          "type 4 vers la gauche : 48/4C a Y=AA");
    Enemies_update(&s);
    CHECK(s.enemy[0].x == 229 && s.enemy[0].y == 170, "type 4 : age 1, pas encore de pas");
    Enemies_update(&s);
    CHECK(s.enemy[0].x == 226 && s.enemy[0].y == 171, "type 4 : age 2, X-3, Y 171");
    for (i = 2; i < 16; i++) Enemies_update(&s);
    (void)Enemies_sprites(&s, 0, sp);
    CHECK(sp[0].pattern == (T4 | 0x50) && s.enemy[0].x == 205 && s.enemy[0].y == 178,
          "type 4 : age 16, pose 50, X 205, Y 178 (image 2713)");
    for (i = 16; i < 48; i++) Enemies_update(&s);
    (void)Enemies_sprites(&s, 0, sp);
    CHECK(sp[0].pattern == (T4 | 0x58) && s.enemy[0].x == 157 && s.enemy[0].y == 174,
          "type 4 : age 48, pose 58, X 157, Y 174 (image 2745)");
    for (i = 48; i < 84; i++) Enemies_update(&s);
    CHECK(s.enemy[0].y == 158, "type 4 : haut de la courbe, Y 158 (age 84)");
    for (i = 84; i < 153; i++) Enemies_update(&s);
    CHECK(s.enemy[0].active && s.enemy[0].exiting && s.enemy[0].x == 1 && s.enemy[0].y == 178,
          "type 4 droite : X 01 tenu jusqu'a l'age 153");
    Enemies_update(&s);
    CHECK(!s.enemy[0].active, "type 4 droite : inactif a l'age 154 (image 2851)");

    Enemies_setRoom(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_4, ENEMY_TYPE4_Y, 0);
    CHECK(s.enemy[0].x == 0x0B && s.enemy[0].dx == 1, "type 4 gauche : x=0B, dx=+1");
    CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_TYPE4_SPRITES && sp[0].pattern == (T4 | 0x60) &&
          sp[1].pattern == (T4 | 0x64), "type 4 vers la droite : 60/64");
    for (i = 0; i < 16; i++) Enemies_update(&s);
    (void)Enemies_sprites(&s, 0, sp);
    CHECK(sp[0].pattern == (T4 | 0x68) && s.enemy[0].x == 35, "type 4 vers la droite : age 16, 68, X 35");
    for (i = 16; i < 147; i++) Enemies_update(&s);
    CHECK(s.enemy[0].exiting && s.enemy[0].x == 0xE6, "type 4 gauche : X E6 a l'age 147");
    Enemies_update(&s);
    CHECK(!s.enemy[0].active, "type 4 gauche : inactif a l'age 148 (image 3013)");
}

/* 6. Type 4 par Enemies_frame (canal 9D97) : CF06 = 8 a l'entree, decremente quand CF10 & 3
 * tombe a 0 (un appel toutes les 2 images) ; avec CF10 = 0, premier tir a l'appel 32 (image 63),
 * puis 168, 168, 168, 328 images (table 9DBB), cote droit puis gauche. */
static void test_type4_timer(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int f;

    v.stage = 1;
    v.room = 4;
    v.map = 0;
    v.simonX = 0x7A;
    v.simonY = 0xC0;   /* au sol du hall : spawn en Y SAT 0xAA (C425 - 22) */
    v.sideFlag = 0;
    v.difficulty = 0;
    v.simonState = ENEMY_SIMON_GROUND;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    s.invuln = 0xFF;   /* pas de contact dans ce test (510 images) */
    for (f = 1; f <= 62; f++) (void)Enemies_frame(&s, &t, &v);
    CHECK(!s.enemy[0].active, "type 4 : aucun tir avant l'image 63");
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.enemy[0].active && s.enemy[0].type == ENEMY_TYPE_4 && s.enemy[0].dx == -1 && s.enemy[0].x == 0xE5,
          "type 4 : premier tir a l'image 63 (CF10 = 0), cote droit");
    CHECK(s.enemy[0].y == ENEMY_TYPE4_Y, "type 4 : Y SAT 0xAA pour C425 = C0");
    CHECK(s.type4Count == 21 && s.type4Shots == 1, "type 4 : CF06 recharge a 21, CF07 = 1");
    for (f = 64; f <= 230; f++) (void)Enemies_frame(&s, &t, &v);
    (void)Enemies_frame(&s, &t, &v);
    /* Le premier tir est sorti vers 150 images d'age : son slot (0) est reutilise. */
    CHECK(s.enemy[0].active && s.enemy[0].type == ENEMY_TYPE_4 && s.enemy[0].dx == 1 && s.enemy[0].x == 0x0B,
          "type 4 : second tir a l'image 231 (+168), cote gauche (slot libere reutilise)");
    CHECK(!s.enemy[1].active, "type 4 : un seul ennemi actif apres le second tir");
    CHECK(s.energy == ENEMY_ENERGY_MAX, "pas de contact dans ce test");

    /* Stage 2, salle 0 (drapeau 08) : Simon en Y 96 -> Y SAT 74, courbe decalee d'autant ;
     * periodes 168, 168, 168 puis 328 (trace stage 2 : 6068, 6236, 6404, 6572, 6900). */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    s.invuln = 0xFF;
    v.stage = 2; v.room = 0; v.simonX = 0x10; v.simonY = 96;
    {
        static const int shots[5] = { 63, 231, 399, 567, 895 };
        int n = 0;
        for (f = 1; f <= 1000; f++) {
            int k, before = 0, after = 0;
            for (k = 0; k < ENEMY_MAX; k++) before += s.enemy[k].active && s.enemy[k].age == 0;
            (void)Enemies_frame(&s, &t, &v);
            if (f % 200 == 0) s.invuln = 0xFF;
            for (k = 0; k < ENEMY_MAX; k++) after += s.enemy[k].type == ENEMY_TYPE_4 && s.enemy[k].active && s.enemy[k].age == 0;
            if (after > before) {
                CHECK(n < 5 && f == shots[n], "stage 2 : tir %d a l'image %d (attendu %d)", n, f, n < 5 ? shots[n] : -1);
                /* 9E16 : Simon en X 10 (< 40), le tir de gauche passe a droite (D = F0). */
                for (k = 0; k < ENEMY_MAX; k++)
                    if (s.enemy[k].active && s.enemy[k].age == 0) CHECK(s.enemy[k].dx == -1, "stage 2 : 9E16, tir a droite");
                for (k = 0; k < ENEMY_MAX; k++)
                    if (s.enemy[k].active && s.enemy[k].age == 0) CHECK(s.enemy[k].y == 74, "stage 2 : Y SAT 74");
                n++;
            }
        }
        CHECK(n == 5, "stage 2 : 5 tirs en 1000 images (obtenu %d)", n);
    }
    v.stage = 1;

    /* Phase de CF10 a l'entree (trace scratchpad/fidelity/out1, hall salle 4, Simon en X 16) :
     * CF10 = 206 au premier appel -> tir a la premiere passe + 58 (image 59) ; CF10 = 25 ->
     * premiere passe + 56 (image 57). CF10 n'est pas remis a zero par le changement de salle. */
    {
        static const struct { EnemyU8 cf10; int frame; } kEntry[2] = { { 206, 59 }, { 25, 57 } };
        int n;
        for (n = 0; n < 2; n++) {
            int first = 0;
            Enemies_init(&s);
            Enemies_spawnTimerInit(&t);
            t.phase = kEntry[n].cf10;
            Enemies_spawnTimerRoom(&t);
            CHECK(t.phase == kEntry[n].cf10, "CF10 garde sa valeur au chargement de salle (9CA9)");
            s.invuln = 0xFF;
            v.room = 4; v.simonX = 0x10; v.simonY = 0xC0;
            for (f = 1; f <= 80 && !first; f++) {
                (void)Enemies_frame(&s, &t, &v);
                if (s.enemy[0].active) first = f;
            }
            CHECK(first == kEntry[n].frame, "CF10 = %d : premier tir a l'image %d (attendu %d)",
                  kEntry[n].cf10, first, kEntry[n].frame);
            CHECK(s.enemy[0].dx == -1, "Simon en X 10 : premier tir a droite (D = F0)");
        }
        v.simonX = 0x7A;
    }

    /* Salle 2 (drapeau 0) : ni type 1 ni type 4. */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    v.room = 2;
    for (f = 1; f <= 400; f++) (void)Enemies_frame(&s, &t, &v);
    for (f = 0; f < ENEMY_MAX; f++)
        CHECK(!s.enemy[f].active || s.enemy[f].type == ENEMY_TYPE_5, "salle 2 : aucun ennemi de type 1 ni 4");
}

/* 7. Contact : -2, invulnerabilite de 180 images ; pas de contact pendant le recul
 * (C420 = 5, mis par le contact et efface par l'appelant : ici 27 images, comme
 * vk_game_hit au sol). Ennemi maintenu sur Simon. */
static void test_contact_recoil_invuln(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int f, contacts[8], n = 0, recoil_end = 0;

    v.stage = 1;
    v.room = 2;        /* salle sans spawn : seul l'ennemi de test existe */
    v.simonX = 120;
    v.simonY = 192;
    v.sideFlag = 0;
    v.difficulty = 0;
    v.simonState = ENEMY_SIMON_GROUND;
    v.map = 0;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    CHECK(s.energy == 0x20, "energie de depart 0x20");
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    for (f = 1; f <= 400; f++) {
        s.enemy[0].x = 126;            /* maintenu en contact avec Simon */
        s.enemy[0].exiting = 0;
        if (f == recoil_end) s.recoil = 0;
        if (Enemies_frame(&s, &t, &v) && n < 8) {
            contacts[n++] = f;
            recoil_end = f + 27;
        }
        if (f == 1) CHECK(s.recoil == 1 && s.invuln == ENEMY_INVULN_START, "contact image 1 : recul et invulnerabilite 0x5A");
    }
    CHECK(n == 3 && contacts[0] == 1 && contacts[1] == 181 && contacts[2] == 361,
          "contacts aux images 1, 181, 361 (obtenu n=%d)", n);
    CHECK(s.energy == 0x20 - 3 * ENEMY_CONTACT_DAMAGE, "energie : 0x20 - 3 contacts x 2");
}

/* 7b. Objets ramasses (8D3E) : 07 (energie + 8), 09 (C434 : contact destructeur), 08 (C43A :
 * aucun contact, effet 17 a 10h), 03 (armure : degat de face non double, 16 coups), 05
 * (tous les ennemis detruits), C431 bit 2 (minuteries F0h), fin de mort (70BF). */
static int first_spawn_frame(int pause_kind)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int f;
    vars_default(&v);
    v.facingLeft = 0;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    s.invuln = 0xFF;
    if (pause_kind) Enemies_item(&s, (EnemyU8)pause_kind, 0);
    for (f = 1; f <= 1000; f++) {
        (void)Enemies_frame(&s, &t, &v);
        if (s.enemy[0].active || s.enemy[1].active) return f;
    }
    return 0;
}

static void test_items(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int f, hits, sounds17 = 0, frames = 0;

    v.stage = 1;
    v.room = 2;        /* salle sans spawn */
    v.simonX = 120;
    v.simonY = 192;
    v.sideFlag = 0;
    v.difficulty = 0;
    v.map = 0;
    v.facingLeft = 0;
    v.simonState = ENEMY_SIMON_GROUND;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);

    /* 07 : 4605, C415 += 8, au plus 20h ; 16 : + 20h. */
    s.energy = 10;
    Enemies_item(&s, 0x07, 0);
    CHECK(s.energy == 18, "objet 07 : energie 10 + 8 (obtenu %d)", s.energy);
    s.energy = 30;
    Enemies_item(&s, 0x07, 0);
    CHECK(s.energy == ENEMY_ENERGY_MAX, "objet 07 : plafond 20h");
    s.energy = 1;
    Enemies_item(&s, 0x16, 0);
    CHECK(s.energy == ENEMY_ENERGY_MAX, "objet 16 : energie pleine");

    /* 09 : C434 = 96h, le contact detruit l'ennemi (100 points, effet 0D), sans degat ni
     * recul, meme pendant l'invulnerabilite ; 300 images (une baisse toutes les 2). */
    Enemies_item(&s, 0x09, 0);
    CHECK(s.killTimer == ENEMY_ITEM_TIMER, "objet 09 : C434 = 96h");
    s.invuln = 0x20;
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    s.enemy[0].x = 126;
    CHECK(Enemies_frame(&s, &t, &v) == 0, "C434 : pas de recul");
    CHECK(!s.enemy[0].active && s.contactScore == 1 && s.itemSound == 0x0D,
          "C434 : ennemi detruit, 100 points, effet 0D");
    CHECK(s.energy == ENEMY_ENERGY_MAX && s.recoil == 0, "C434 : pas de degat");
    s.invuln = 0;
    for (f = 2; s.killTimer; f++) (void)Enemies_frame(&s, &t, &v);
    CHECK(f - 1 == 2 * ENEMY_ITEM_TIMER || f - 1 == 2 * ENEMY_ITEM_TIMER - 1,
          "C434 : 150 passes de 2 images (obtenu %d images)", f - 1);
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    s.enemy[0].x = 126;
    CHECK(Enemies_frame(&s, &t, &v) == 1 && s.energy == ENEMY_ENERGY_MAX - ENEMY_CONTACT_DAMAGE,
          "C434 a 0 : contact normal");
    s.recoil = 0;
    s.invuln = 0;
    s.energy = ENEMY_ENERGY_MAX;

    /* 08 : C43A = 96h, aucun contact ; effet 17 une fois, quand C43A tombe a 10h. */
    Enemies_item(&s, 0x08, 0);
    hits = 0;
    while (s.guardTimer) {
        s.enemy[0].x = 126;
        s.enemy[0].active = 1;
        s.enemy[0].exiting = 0;
        hits += Enemies_frame(&s, &t, &v);
        if (s.itemSound == 0x17) {
            sounds17++;
            CHECK(s.guardTimer == ENEMY_GUARD_WARNING, "effet 17 a 10h");
        }
        frames++;
    }
    CHECK(hits == 0 && sounds17 == 1 && s.energy == ENEMY_ENERGY_MAX, "C43A : ni contact ni degat, un effet 17");
    CHECK(frames >= 2 * ENEMY_ITEM_TIMER - 1 && frames <= 2 * ENEMY_ITEM_TIMER, "C43A : 300 images (%d)", frames);
    CHECK(Enemies_frame(&s, &t, &v) == 1, "C43A a 0 : contact");
    s.recoil = 0;
    s.invuln = 0;

    /* 790C : silhouette de Simon, 0Eh avec C43A, sinon 8 avec C434, quand le bit 0 de
     * C003 est mis ; C43A l'emporte. */
    {
        EnemyState w;                    /* etat a part : s garde son ennemi pour l'armure */
        Enemies_init(&w);
        w.tick = 1;
        CHECK(Enemies_simonColor(&w) == 0, "790C : sans C43A ni C434, couleurs normales");
        w.killTimer = 0x40;
        CHECK(Enemies_simonColor(&w) == ENEMY_SIMON_KILL_COLOR, "7932 : C434 -> couleur 8");
        w.guardTimer = 0x40;
        CHECK(Enemies_simonColor(&w) == ENEMY_SIMON_GUARD_COLOR, "7924 : C43A -> couleur 0Eh");
        w.tick = 2;
        CHECK(Enemies_simonColor(&w) == 0, "792D : bit 0 de C003 a 0, couleurs normales");
        {
            /* Une passe sur deux : 2 images de silhouette, 2 normales. */
            int on = 0, changes = 0, last = Enemies_simonColor(&w);
            for (f = 0; f < 40; f++) {
                int c;
                (void)Enemies_frame(&w, &t, &v);
                c = Enemies_simonColor(&w);
                on += c != 0;
                changes += c != last;
                last = c;
            }
            CHECK(on == 20 && changes == 20, "790C : 2 images sur 4 (allumees %d, bascules %d)", on, changes);
        }

        /* 05 : C43E = 18h ; 7604 : F3EB = 0Eh si le bit 1 de C43E est mis avant la baisse.
         * 24 passes de 2 images : 0, puis 4 images de chaque couleur, fin a 0. */
        w.enemy[0].active = 1;
        Enemies_item(&w, 0x05, 0);
        CHECK(w.flash == ENEMY_FLASH_START && !w.enemy[0].active, "objet 05 : C43E = 18h, ennemis detruits");
        {
            static const EnemyU8 expect[24] = { 0, 14, 14, 0, 0, 14, 14, 0, 0, 14, 14, 0,
                                                0, 14, 14, 0, 0, 14, 14, 0, 0, 14, 14, 0 };
            int pass = 0, ok = 1;
            for (f = 0; f < 60; f++) {
                const EnemyU8 before = w.flash;
                (void)Enemies_frame(&w, &t, &v);
                if (w.flash != before) {
                    if (pass >= 24 || w.border != expect[pass]) ok = 0;
                    pass++;
                }
            }
            CHECK(ok && pass == 24 && w.flash == 0 && w.border == 0,
                  "7604 : 24 passes, cadence 0/0Eh par 2 passes, fin a 0 (passes %d)", pass);
        }
    }

    /* C431 bit 2 (objet 0B) : F0h. */
    Enemies_item(&s, 0x08, 0x04);
    Enemies_item(&s, 0x09, 0x04);
    Enemies_item(&s, 0x06, 0x04);
    CHECK(s.guardTimer == ENEMY_ITEM_TIMER_LONG && s.killTimer == ENEMY_ITEM_TIMER_LONG &&
          s.spawnPause == ENEMY_ITEM_TIMER_LONG, "C431 bit 2 : minuteries F0h");
    Enemies_item(&s, 0x1A, 0);
    CHECK(s.spawnPause == ENEMY_ITEM_TIMER, "fouet 1A : C440 = 96h (8D70 -> 8D7C)");
    Enemies_death(&s);
    CHECK(!s.guardTimer && !s.killTimer && !s.spawnPause && !s.armor, "70BF : minuteries a 0");

    /* 03 : de face, degat 1 au lieu de 2 (C441 - 1) ; de dos, 2. Ennemi a droite. */
    Enemies_item(&s, 0x03, 0);
    CHECK(s.armor == ENEMY_ARMOR_HITS, "objet 03 : C441 = 10h");
    s.energy = ENEMY_ENERGY_MAX;
    s.enemy[0].active = 1;
    s.enemy[0].x = 126;
    v.facingLeft = 0;
    CHECK(Enemies_frame(&s, &t, &v) == 1 && s.energy == ENEMY_ENERGY_MAX - ENEMY_CONTACT_DAMAGE / 2 &&
          s.armor == ENEMY_ARMOR_HITS - 1, "armure de face : -1, C441 - 1");
    s.recoil = 0;
    s.invuln = 0;
    v.facingLeft = 1;
    CHECK(Enemies_frame(&s, &t, &v) == 1 && s.energy == ENEMY_ENERGY_MAX - 3 * ENEMY_CONTACT_DAMAGE / 2 &&
          s.armor == ENEMY_ARMOR_HITS - 1, "armure de dos : -2");
    for (f = 0; f < 15; f++) {
        s.recoil = 0;
        s.invuln = 0;
        s.energy = ENEMY_ENERGY_MAX;
        s.enemy[0].x = 126;
        v.facingLeft = 0;
        (void)Enemies_frame(&s, &t, &v);
    }
    CHECK(s.armor == 0, "armure : 16 coups de face");

    /* 05 : 7806, tous les ennemis detruits. */
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    (void)Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 1);
    Enemies_item(&s, 0x05, 0);
    for (f = 0; f < ENEMY_MAX; f++) CHECK(!s.enemy[f].active, "objet 05 : ennemi %d detruit", f);
}

/* 7c. 06 (C440) : la boucle d'apparition 5ED8 est sautee 150 passes (300 images). */
static void test_spawn_pause(void)
{
    const int normal = first_spawn_frame(0);
    const int paused = first_spawn_frame(0x06);
    CHECK(normal > 0 && paused == normal + 2 * ENEMY_ITEM_TIMER,
          "C440 : premiere apparition %d images plus tard (obtenu %d -> %d)",
          2 * ENEMY_ITEM_TIMER, normal, paused);
}

/* 8. Chute : sans sol sous lui un ennemi descend ; posé sur le sol de la salle (ligne 22 :
 * pieds en Y 192), il s'arrete. Carte synthetique : une seule rangee de tuiles pleines. */
static void test_fall_to_floor(void)
{
    static uint8_t map[768];
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int i;

    memset(map, 0, sizeof map);
    v.stage = 1;
    v.room = 2;
    v.simonX = 0xF0;      /* loin de l'ennemi : pas de contact dans ce test */
    v.simonY = 0x40;
    v.sideFlag = 0;
    v.difficulty = 0;
    v.simonState = ENEMY_SIMON_GROUND;
    v.map = map;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    (void)Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    /* Pas de sol : chute de ENEMY_FALL_SPEED par image. */
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.enemy[0].y == 0xA0 + ENEMY_FALL_SPEED, "chute sans sol : 2 px par image (obtenu %d)", s.enemy[0].y);
    /* Sol en rangee 22 de la carte (ligne de 32 tuiles, pieds a Y 192). Le spawn a 0xA0
     * (pieds 191) tombe d'un pixel et s'arrete. */
    memset(map, 0, sizeof map);
    for (i = 0; i < 32; i++) map[22 * 32 + i] = 1;
    Enemies_init(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.enemy[0].y == 0xA1, "chute sur sol : s'arrete a Y 0xA1 (pieds 192), obtenu %d", s.enemy[0].y);
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.enemy[0].y == 0xA1, "sur le sol : reste a Y 0xA1");
}

/* 9. Coup de fouet : tue un ennemi de type 1 ou 4 touche (boite du fouet, coordonnees de jeu),
 * laisse le type 4 et un ennemi hors de portee. */
static void test_whip_kills_type1(void)
{
    EnemyState s;
    int killed;

    Enemies_init(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 160, 1);
    s.enemy[0].x = 120;               /* jeu x 128 : dans la boite (128..152, y 170..177) */
    killed = Enemies_whipHit(&s, 128, 170, 152, 177);
    CHECK(killed == 1 && !s.enemy[0].active, "fouet : ennemi de type 1 touche, mort");

    Enemies_init(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 160, 1);
    s.enemy[0].x = 150;               /* jeu x 158 : hors de la boite (droite 152) */
    killed = Enemies_whipHit(&s, 128, 170, 152, 177);
    CHECK(killed == 0 && s.enemy[0].active, "fouet : ennemi hors de portee, vivant");

    Enemies_init(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_4, ENEMY_TYPE4_Y, 0);
    s.enemy[0].x = 120;
    killed = Enemies_whipHit(&s, 128, 170, 152, 177);
    CHECK(killed == 1 && !s.enemy[0].active, "fouet : type 4 touche, mort (table C800, scratchpad/type4)");
}

/* Type 5 (panthere), research/enemy-behavior.md section Type 5 : creation 21 images apres
 * l'entree en salle 2 (80,80), activation a 62 px, courbe du saut table complete,
 * atterrissage Y & F8, poses et decalages SAT, contact -6, fouet debout / accroupi. */
static void test_type5(void)
{
    static EnemyU8 map[768];
    static const EnemyU8 jumpY[13] = { 0x7E, 0x7E, 0x7E, 0x7F, 0x81, 0x83, 0x85, 0x89,
                                        0x8D, 0x91, 0x97, 0x9C, 0xA3 };
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemySprite sp[ENEMY_SPRITES];
    Enemy *e = &s.enemy[0];
    int f, k;
    memset(map, 0, sizeof map);
    for (k = 0; k < 32; k++) map[18 * 32 + k] = 1;   /* sol en Y A0..A7 */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    memset(&v, 0, sizeof v);
    v.stage = 1; v.room = 2; v.simonX = 10; v.simonY = 192; v.map = map;
    for (f = 1; f < ENEMY_TYPE5_DELAY; f++) (void)Enemies_frame(&s, &t, &v);
    CHECK(!e->active, "type 5 : pas encore cree a l'image %d", f - 1);
    (void)Enemies_frame(&s, &t, &v);
    CHECK(e->active && e->type == ENEMY_TYPE_5 && e->x == 0x80 && e->y == 0x80 && e->pose == 0x3F,
          "type 5 : cree en 80,80 assis a gauche");
    CHECK(Enemies_sprites(&s, 0, sp) == 4 && sp[0].pattern == 0x40 && sp[1].pattern == 0x44 &&
          sp[2].pattern == 0x48 && sp[3].pattern == 0x4C && sp[0].x == 0x80 - 9 && sp[2].x == 0x80 + 7 &&
          sp[0].y == 0x80 - 15 && sp[0].bank == 1, "type 5 : pose 3F en X-9 / X+7");
    for (k = 0; k < 4; k++) (void)Enemies_frame(&s, &t, &v);
    CHECK(e->state == 0 && e->x == 0x80, "type 5 : Simon a 118 px, reste assise");
    v.simonX = 0x80 + 62;
    (void)Enemies_frame(&s, &t, &v);
    if (e->state == 0) (void)Enemies_frame(&s, &t, &v);
    CHECK(e->state == 1 && e->dx == 1 && e->x == 0x84 && e->pose == 0x3F,
          "type 5 : activation a 62 px, 4 px vers Simon, pose assise une mise a jour");
    v.simonX = 0;   /* pas de demi-tour en course ni en saut */
    for (k = 0; k < 13; k++) {
        (void)Enemies_frame(&s, &t, &v);
        (void)Enemies_frame(&s, &t, &v);
        CHECK(e->y == jumpY[k], "type 5 : saut pas %d, Y %02X attendu %02X", k, jumpY[k], e->y);
        CHECK(e->state == 2 && e->pose == 0x46, "type 5 : saut pas %d, pose 46", k);
    }
    CHECK(e->x == 0x84 + 4 * 13, "type 5 : 4 px par mise a jour en saut (X %d)", e->x);
    (void)Enemies_frame(&s, &t, &v);
    (void)Enemies_frame(&s, &t, &v);
    CHECK(e->state == 1 && e->y == 0xA0 && e->sub == 0 && e->vy == 0 && e->dx == -1,
          "type 5 : posee en A0, repart vers Simon");
    (void)Enemies_frame(&s, &t, &v);
    (void)Enemies_frame(&s, &t, &v);
    CHECK(e->anim == 1 && e->pose == 0x41, "type 5 : course gauche, pose 41 au compteur 1");
    CHECK(Enemies_sprites(&s, 0, sp) == 4 && sp[0].pattern == 0x60 && sp[2].pattern == 0x68 &&
          sp[0].x == e->x - 16 && sp[2].x == e->x, "type 5 : pose 41 en X-16 / X");
    e->pose = 0x43;
    CHECK(Enemies_sprites(&s, 0, sp) == 4 && sp[0].pattern == 0x88 && sp[1].pattern == 0x8C &&
          sp[2].pattern == 0x80 && sp[0].x == e->x - 23 && sp[2].x == e->x - 7,
          "type 5 : pose 43 en X-23 / X-7");

    /* Contact : |C427 - X| < 17, Y de C425 - 27 a C425 + 8 ; -6. */
    e->x = 100; e->y = 0xA0; e->state = 0; e->dx = 0;
    v.simonX = 116; v.simonY = 0xA0 - 8;
    s.energy = 32; s.invuln = 0; s.recoil = 0;
    CHECK(Enemies_frame(&s, &t, &v) == 1 && s.energy == 26 && s.recoilDir == 1,
          "type 5 : contact -6, recul a l'oppose");
    /* La panthere s'est reveillee : Simon 28 px plus bas qu'elle est hors de la boite. */
    s.invuln = 0; s.recoil = 0; s.energy = 32; v.simonX = (EnemyU8)e->x; v.simonY = 0xA0 + 28;
    CHECK(Enemies_frame(&s, &t, &v) == 0 && s.energy == 32, "type 5 : Y = C425 - 28, pas de contact");

    /* Fouet : debout, il passe au-dessus d'une panthere au niveau de Simon ; accroupi, il la tue. */
    e->x = 100; e->y = 0xA0;
    CHECK(Enemies_whipHitType5(&s, 120, 0xA0, 1, 0) == 0 && e->active, "type 5 : fouet debout au-dessus");
    CHECK(Enemies_whipHitType5(&s, 120, 0xA0, 0, 1) == 0 && e->active, "type 5 : fouet accroupi du mauvais cote");
    CHECK(Enemies_whipHitType5(&s, 120, 0xA0, 1, 1) == 1 && !e->active, "type 5 : fouet accroupi, mort");
}

/* 7. Type 1 du stage 2, salle 2 (trace scratchpad/s2t1/out/O1 et W1, research/enemy-behavior.md
 * section « Type 1 au stage 2 »). Age = image depuis l'ecriture de C800 - 1. */
static void vars_stage2_room2(EnemyRoomVars *v)
{
    v->stage = 2;
    v->room = 2;
    v->simonX = 145;   /* Simon en haut de l'escalier de la salle 2 */
    v->simonY = 160;
    v->sideFlag = 0;
    v->difficulty = 0;
    v->simonState = ENEMY_SIMON_GROUND;
    v->map = 0;
}

static void test_stage2_type1_spawn(void)
{
    /* Cotes D, G, G, D, D, G, G (9D1F : bit 1 de CF01) ; 104, 152, 104, 104, 104, 152 images
     * entre les tirs (6238, 6342, 6494, 6598, 6702, 6806, 6958 dans la trace O1). */
    static const int side[7] = { ENEMY_SIDE_RIGHT, ENEMY_SIDE_LEFT, ENEMY_SIDE_LEFT,
                                 ENEMY_SIDE_RIGHT, ENEMY_SIDE_RIGHT, ENEMY_SIDE_LEFT, ENEMY_SIDE_LEFT };
    static const int gapFrames[6] = { 104, 152, 104, 104, 104, 152 };
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int call, shots = 0, last = 0, room;

    vars_stage2_room2(&v);
    v.sideFlag = 1;    /* ignore au stage 2 */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 600 && shots < 7; call++) {
        if (!Enemies_spawnTimerTick(&t, &v, &s)) continue;
        if (shots == 0) CHECK(call == 16, "stage 2 : premier tir a l'appel 16 (obtenu %d)", call);
        else CHECK(2 * (call - last) == gapFrames[shots - 1], "stage 2 : ecart %d attendu %d images, obtenu %d",
                   shots, gapFrames[shots - 1], 2 * (call - last));
        CHECK(s.enemy[0].active && s.enemy[0].romBox, "stage 2 : tir %d marque romBox", shots);
        CHECK(s.enemy[0].y == 0x80, "stage 2 : Y SAT 80 (E = A0), obtenu %02X", s.enemy[0].y);
        CHECK(s.enemy[0].dx == (side[shots] == ENEMY_SIDE_RIGHT ? -1 : 1), "stage 2 : cote du tir %d", shots);
        CHECK(s.enemy[0].x == (side[shots] == ENEMY_SIDE_RIGHT ? 0xE5 : 0x0A), "stage 2 : X SAT du tir %d", shots);
        CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_SPRITES &&
              sp[0].pattern == (side[shots] == ENEMY_SIDE_RIGHT ? 0x40 : 0x58) &&
              sp[0].y == 0x80 && sp[2].y == 0x90, "stage 2 : motifs et rangs (80, 90) du tir %d", shots);
        last = call;
        shots++;
        Enemies_setRoom(&s);
    }
    CHECK(shots == 7, "stage 2 : 7 tirs attendus, obtenu %d", shots);

    /* Drapeau bit 0 seulement en salle 2 (85D7 : 08 08 01 08 02 02). */
    for (room = 0; room < 6; room++) {
        if (room == 2) continue;
        v.room = (EnemyU8)room;
        Enemies_setRoom(&s);
        Enemies_spawnTimerInit(&t);
        for (call = 1; call <= 400; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
        CHECK(!s.enemy[0].active, "stage 2 salle %d : pas de type 1", room);
    }

    /* Garde 9E16 : Simon a droite (X >= C0) refuse le cote droit. */
    vars_stage2_room2(&v);
    v.simonX = 0xD0;
    Enemies_setRoom(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(!s.enemy[0].active, "stage 2 : garde 9E16, Simon en D0, tir droit refuse");
}

static void test_stage2_type1_trajectory(void)
{
    typedef struct { int age; int x; uint8_t bottom; } Expect;
    /* SAT lue dans la trace O1 (premier tir droit, image 6238 ; premier tir gauche, 6342). */
    static const Expect right[] = {
        { 1, 229, 0x48 }, { 3, 227, 0x48 }, { 15, 215, 0x50 }, { 17, 212, 0x50 },
        { 23, 206, 0x48 }, { 31, 198, 0x50 }, { 33, 195, 0x50 }, { 217, 0, 0x48 }, { 218, 0, 0x48 },
    };
    static const Expect left[] = {
        { 1, 10, 0x60 }, { 3, 12, 0x60 }, { 15, 25, 0x68 }, { 23, 33, 0x60 }, { 31, 42, 0x68 },
        { 205, 226, 0x60 }, { 207, 229, 0x68 }, { 209, 231, 0x68 }, { 210, 231, 0x68 },
    };
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int call, age;
    size_t i;

    vars_stage2_room2(&v);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    for (call = 1; call <= 16; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(s.enemy[0].active && s.enemy[0].dx < 0, "trajectoire : premier tir a droite");
    for (age = 0, i = 0; i < sizeof right / sizeof right[0]; i++) {
        while (age < right[i].age) { Enemies_update(&s); age++; }
        CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_SPRITES, "droite : inactif a l'age %d", age);
        CHECK(sp[0].x == right[i].x, "droite age %d : X %d attendu, obtenu %d", age, right[i].x, sp[0].x);
        CHECK(sp[2].pattern == right[i].bottom, "droite age %d : motif bas %02X attendu, obtenu %02X",
              age, right[i].bottom, sp[2].pattern);
    }
    Enemies_update(&s);
    CHECK(!s.enemy[0].active, "droite : disparu a l'age 219");

    /* Deuxieme tir (gauche) : 52 appels plus tard. */
    Enemies_setRoom(&s);
    for (call = 1; call <= 52; call++) (void)Enemies_spawnTimerTick(&t, &v, &s);
    CHECK(s.enemy[0].active && s.enemy[0].dx > 0, "trajectoire : deuxieme tir a gauche");
    for (age = 0, i = 0; i < sizeof left / sizeof left[0]; i++) {
        while (age < left[i].age) { Enemies_update(&s); age++; }
        CHECK(Enemies_sprites(&s, 0, sp) == ENEMY_SPRITES, "gauche : inactif a l'age %d", age);
        CHECK(sp[0].x == left[i].x, "gauche age %d : X %d attendu, obtenu %d", age, left[i].x, sp[0].x);
        CHECK(sp[2].pattern == left[i].bottom, "gauche age %d : motif bas %02X attendu, obtenu %02X",
              age, left[i].bottom, sp[2].pattern);
    }
    Enemies_update(&s);
    CHECK(!s.enemy[0].active, "gauche : disparu a l'age 211 (objet a X F1)");
}

/* Un ennemi romBox pose (Y affiche 81 : pieds en A0) a l'objet X donne. */
static void put_stage2_type1(EnemyState *s, int objX, int objY, int side)
{
    const int k = Enemies_spawn(s, (EnemyU8)side, side == ENEMY_SIDE_RIGHT ? ENEMY_TYPE_A : ENEMY_TYPE_B,
                                0x80, side == ENEMY_SIDE_RIGHT ? 1 : 15);
    s->enemy[k].romBox = 1;
    s->enemy[k].x = (EnemyS16)(objX - 8);
    s->enemy[k].y = (EnemyU8)(objY - 31);
}

static int stage2_contact(int objX, int objY)
{
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;

    vars_stage2_room2(&v);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    put_stage2_type1(&s, objX, objY, ENEMY_SIDE_RIGHT);
    return Enemies_frame(&s, &t, &v) && s.energy == ENEMY_ENERGY_MAX - 2;
}

static void test_stage2_type1_contact_whip(void)
{
    static uint8_t map[768];
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemyState s;
    int i;

    /* Contact 8290 classe 1 : |C427 - X| < 10, Y de C425 - 27 a C425 + 22 ; degat 2. */
    CHECK(stage2_contact(145 + 9, 160), "contact a X + 9");
    CHECK(stage2_contact(145 - 9, 160), "contact a X - 9");
    CHECK(!stage2_contact(145 + 10, 160), "pas de contact a X + 10");
    CHECK(!stage2_contact(145 - 10, 160), "pas de contact a X - 10");
    CHECK(stage2_contact(145, 160 + 22) && !stage2_contact(145, 160 + 23), "contact : borne basse C425 + 22");
    CHECK(stage2_contact(145, 160 - 27) && !stage2_contact(145, 160 - 28), "contact : borne haute C425 - 27");

    /* Fouet 7DAD classe 1 : X du fouet = 145 + 20 = 165, |165 - X| < 17. */
    Enemies_init(&s);
    put_stage2_type1(&s, 165 + 16, 160, ENEMY_SIDE_RIGHT);
    put_stage2_type1(&s, 165 - 16, 160, ENEMY_SIDE_LEFT);
    put_stage2_type1(&s, 165 + 17, 160, ENEMY_SIDE_RIGHT);
    put_stage2_type1(&s, 165 - 17, 160, ENEMY_SIDE_LEFT);
    CHECK(Enemies_whipHit(&s, 0, 0, 255, 255) == 0, "Enemies_whipHit ignore les ennemis romBox");
    CHECK(Enemies_whipHitType1Rom(&s, 145, 160, 0, 0) == 2, "fouet : |d| <= 16 touche, 17 non");
    CHECK(!s.enemy[0].active && !s.enemy[1].active && s.enemy[2].active && s.enemy[3].active,
          "fouet : les deux ennemis a 17 px restent");
    /* Cas de la trace W1 : Simon 145 a droite, objet en X 156 ; Simon 189 a gauche, X 157. */
    Enemies_init(&s);
    put_stage2_type1(&s, 156, 160, ENEMY_SIDE_LEFT);
    CHECK(Enemies_whipHitType1Rom(&s, 145, 160, 0, 0) == 1, "fouet : mort W1 6473");
    Enemies_init(&s);
    put_stage2_type1(&s, 157, 160, ENEMY_SIDE_RIGHT);
    CHECK(Enemies_whipHitType1Rom(&s, 189, 160, 1, 0) == 1, "fouet : mort W1 6677");
    /* Hauteur : debout Y de C425 - 21 a C425 + 6, accroupi C425 - 13 a C425 + 14. */
    Enemies_init(&s);
    put_stage2_type1(&s, 165, 160 + 6, ENEMY_SIDE_RIGHT);
    put_stage2_type1(&s, 165, 160 + 7, ENEMY_SIDE_RIGHT);
    CHECK(Enemies_whipHitType1Rom(&s, 145, 160, 0, 0) == 1 && s.enemy[1].active, "fouet debout : Y + 6 oui, Y + 7 non");
    CHECK(Enemies_whipHitType1Rom(&s, 145, 160, 0, 1) == 1, "fouet accroupi : Y + 7 touche");
    /* Bord d'ecran : X du fouet hors 0..255, pas de test (84B6). */
    Enemies_init(&s);
    put_stage2_type1(&s, 250, 160, ENEMY_SIDE_LEFT);
    CHECK(Enemies_whipHitType1Rom(&s, 240, 160, 0, 0) == 0, "fouet : retenue a droite, pas de coup");

    /* Chute : pose sur le sol de la salle 2 (rang Y A0), Y affiche 81 comme au hall (A1). */
    memset(map, 0, sizeof map);
    for (i = 0; i < 32; i++) map[18 * 32 + i] = 1;
    vars_stage2_room2(&v);
    v.map = map;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    for (i = 0; i < 40; i++) { (void)Enemies_frame(&s, &t, &v); }
    CHECK(s.enemy[0].active && s.enemy[0].y == 0x81, "stage 2 : pose au sol, Y 81 (obtenu %02X)", s.enemy[0].y);
}

/* Type 2 (homme-poisson), stage 2 salle 4, carte du port (vk_stage2_collision[4]).
 * Image n du port = image n apres l'ecriture de D001 (trace scratchpad/fish/out/P1, D001
 * en 6115) ; Simon en (72, 112) sur le sol gauche, comme dans la trace. */
static void vars_stage2_room4(EnemyRoomVars *v)
{
    memset(v, 0, sizeof *v);
    v->stage = 2;
    v->room = 4;
    v->simonX = 72;
    v->simonY = 112;
    v->map = vk_stage2_collision[4];
}

static int count_type(const EnemyState *s, EnemyU8 type)
{
    int i, n = 0;
    for (i = 0; i < ENEMY_MAX; i++) n += s->enemy[i].active && s->enemy[i].type == type;
    return n;
}

static int find_type2(const EnemyState *s, int x)
{
    int i;
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_2 && (x < 0 || s->enemy[i].x == x)) return i;
    return -1;
}

/* Images d'etat 9 a l'arrivee en salle 4 ou 5 (21) moins une : la premiere image de jeu
 * (n = 1) est la 21e image apres D001 (research/room-links.md section 9). */
#define FISH_STATE9 20

static void test_type2_spawn(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int n, k, created[4], c = 0;
    vars_stage2_room4(&v);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    CHECK(kEnemyStage2RoomFlags[4] == 0x02 && kEnemyStage2RoomFlags[5] == 0x02, "drapeaux 85DB-85DC : bit 1");
    /* Tirs mesures : +51 (X D0), +67 (X 30), +267 (X 90), +467 (X A0) apres D001 ; deux gouttes chacun.
     * Images de jeu comptees apres l'etat 9 (FISH_STATE9 = 20 de moins), CF10 = 0 a l'entree. */
    for (n = 1; n <= 467 - FISH_STATE9; n++) {
        int before = count_type(&s, ENEMY_TYPE_2);
        (void)Enemies_frame(&s, &t, &v);
        if (count_type(&s, ENEMY_TYPE_2) > before && c < 4) created[c++] = n;
        if (n == 51 - FISH_STATE9) {
            k = find_type2(&s, 0xD0);
            CHECK(k == 0, "premier tir en X D0, bloc 0");
            CHECK(k >= 0 && s.enemy[k].y == 189 && s.enemy[k].vy == -0x0B00,
                  "premiere mise a jour dans l'image du tir : Y 189, vy -0B00");
            CHECK(k >= 0 && s.enemy[k].pose == 0x08, "pose 08 : Simon a gauche");
            CHECK(count_type(&s, ENEMY_TYPE_SPLASH) == 2, "deux gouttes a la creation");
            CHECK(s.enemy[1].type == ENEMY_TYPE_SPLASH && s.enemy[1].dx == 1 && s.enemy[1].vy == -0x0500 &&
                  s.enemy[2].dx == -1 && s.enemy[2].vy == -0x0600, "gouttes : +1/-5 puis -1/-6 (CF30)");
            CHECK(s.enemy[1].y == 0xC8 && s.enemy[1].x == 0xD0, "gouttes en (D0, C8), immobiles dans l'image du tir");
        }
        if (n == 67 - FISH_STATE9) CHECK(find_type2(&s, 0x30) >= 0, "second tir 16 images plus tard en X 30");
    }
    CHECK(c == 4 && created[0] == 51 - FISH_STATE9 && created[1] == 67 - FISH_STATE9 &&
          created[2] == 267 - FISH_STATE9 && created[3] == 467 - FISH_STATE9,
          "tirs aux images 51, 67, 267, 467 - 20 (obtenu %d %d %d %d)", created[0], created[1], created[2], created[3]);
    CHECK(s.type2Shots == 4, "CF03 = 4");
    /* Garde 9D6E : Simon a moins de 24 px du X de la table, X decale de 32 px vers le centre
     * (trace salle 5, Simon en 40 : X 30 -> 50, X 40 -> 60). */
    vars_stage2_room4(&v);
    v.room = 5;
    v.simonX = 40;
    Enemies_init(&s);
    for (n = 1; n <= 67 - FISH_STATE9; n++) (void)Enemies_frame(&s, &t, &v);
    CHECK(find_type2(&s, 0x50) >= 0, "garde : X 30 decale en 50");
    /* 9CC3 : CF10 n'est pas remis a zero par 9CA9. Phase p a l'entree : CF02 (4) atteint 0 au
     * 4e appel ou (p - k) & 3 = 0, appel k = (p & 3 ? p & 3 : 4) + 12, image 2k - 1. */
    {
        static const EnemyU8 phases[4] = { 0, 1, 2, 3 };
        static const int first[4] = { 31, 25, 27, 29 };
        int p;
        for (p = 0; p < 4; p++) {
            vars_stage2_room4(&v);
            Enemies_init(&s);
            Enemies_spawnTimerInit(&t);
            t.phase = phases[p];
            for (n = 1; n <= 40 && count_type(&s, ENEMY_TYPE_2) == 0; n++) (void)Enemies_frame(&s, &t, &v);
            CHECK(n - 1 == first[p], "CF10 & 3 = %d a l'entree : premier tir a l'image %d (obtenu %d)",
                  p, first[p], n - 1);
        }
    }
}

static void test_type2_trajectory(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    Enemy *e = &s.enemy[0];
    int n, minY = 255, landed = 0, fell = 0, gone = 0;
    vars_stage2_room4(&v);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    for (n = 1; n <= 200 - FISH_STATE9; n++) {
        (void)Enemies_frame(&s, &t, &v);
        if (n < 51 - FISH_STATE9 || n > 183 - FISH_STATE9) continue;
        if (e->active && e->type == ENEMY_TYPE_2 && e->y < minY) minY = e->y;
        if (!landed && e->type == ENEMY_TYPE_2 && e->state == 1) landed = n;
        if (landed && !fell && e->type == ENEMY_TYPE_2 && e->state == 0) fell = n;
        if (!gone && !(e->active && e->type == ENEMY_TYPE_2)) gone = n;
        if (n == 121 - FISH_STATE9) CHECK(e->state == 1 && e->y == 112 && e->sub == 0x80 && e->x == 208,
                            "pose sur le sol droit en 6236 : (208, 112.80) (obtenu %d, %d.%02X)", e->x, e->y, e->sub);
        if (n == 123 - FISH_STATE9) CHECK(e->dx == -1 && e->pose == 0x09 && e->timer == 0x20 && e->x == 206,
                            "premiere marche : vers Simon, pose 09, +15 = 20 (+1D = 25)");
        if (n == 139 - FISH_STATE9) CHECK(e->pose == 0x08, "pose 08 apres 8 mises a jour");
    }
    CHECK(minY == 73, "sommet du saut Y 73 (obtenu %d)", minY);
    CHECK(landed == 121 - FISH_STATE9, "atterrissage a l'image 121 (obtenu %d)", landed);
    CHECK(fell == 145 - FISH_STATE9, "chute au bord du sol (X 191 -> 190) a l'image 145 (obtenu %d)", fell);
    CHECK(gone == 183 - FISH_STATE9, "replonge sous Y C0 a l'image 183 (obtenu %d)", gone);
    CHECK(count_type(&s, ENEMY_TYPE_SPLASH) >= 2, "deux gouttes au plongeon");
}

static void test_type2_contact_whip(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    EnemySprite sp[ENEMY_SPRITES];
    int k;
    /* Contact (6273) : homme-poisson en chute en (67, 85), Simon (72, 112) : -4. */
    vars_stage2_room4(&v);
    v.room = 0;                       /* pas de tir dans ce test */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    k = 0;
    s.enemy[k].active = 1; s.enemy[k].type = ENEMY_TYPE_2; s.enemy[k].x = 67; s.enemy[k].y = 85;
    s.enemy[k].pose = 0x0B;
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.energy == ENEMY_ENERGY_MAX - 4, "contact : -4 (obtenu %d)", s.energy);
    /* Boite 8378 : |C427 - X| < 10 et Y de C425 - 27 a C425 + 22. */
    Enemies_init(&s);
    s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_2; s.enemy[0].x = 82; s.enemy[0].y = 112 + 22;
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.energy == ENEMY_ENERGY_MAX, "pas de contact a |d| = 10");
    s.enemy[0].x = 81;
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.energy == ENEMY_ENERGY_MAX - 4, "contact a |d| = 9, Y = C425 + 22");
    /* Fouet (P2, 6235) : Simon (236, 112) tourne a gauche, X du fouet 216 ; homme-poisson en
     * (208, 112) tue, en (208, 74) (phase 3 de 6207) epargne. */
    Enemies_init(&s);
    s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_2; s.enemy[0].x = 208; s.enemy[0].y = 74;
    s.enemy[1].active = 1; s.enemy[1].type = ENEMY_TYPE_SPLASH; s.enemy[1].x = 216; s.enemy[1].y = 100;
    CHECK(Enemies_whipHitType2(&s, 236, 112, 1, 0) == 0, "fouet : Y 74 hors de la boite");
    s.enemy[0].y = 112;
    CHECK(Enemies_whipHitType2(&s, 236, 112, 1, 0) == 1 && !s.enemy[0].active, "fouet : mort au premier coup");
    CHECK(s.enemy[1].active, "goutte non touchable");
    CHECK(Enemies_whipHit(&s, 0, 0, 255, 255) == 0, "Enemies_whipHit ignore la goutte");
    /* Bornes : |X fouet - X| < 17 ; debout Y de C425 - 21 a C425 + 6. */
    Enemies_init(&s);
    s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_2; s.enemy[0].x = 216 - 17; s.enemy[0].y = 112;
    s.enemy[1].active = 1; s.enemy[1].type = ENEMY_TYPE_2; s.enemy[1].x = 216 + 16; s.enemy[1].y = 112 + 6;
    s.enemy[2].active = 1; s.enemy[2].type = ENEMY_TYPE_2; s.enemy[2].x = 216; s.enemy[2].y = 112 + 7;
    CHECK(Enemies_whipHitType2(&s, 236, 112, 1, 0) == 1 && s.enemy[0].active && !s.enemy[1].active && s.enemy[2].active,
          "fouet : bornes X 17 et Y + 6");
    /* Dessin : pose 09 = 40/44 en Y-31, 50/54 en Y-15, X-8 ; pose 0C = 68/6C, 78/7C. */
    Enemies_init(&s);
    s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_2; s.enemy[0].x = 100; s.enemy[0].y = 112; s.enemy[0].pose = 0x09;
    CHECK(Enemies_sprites(&s, 0, sp) == 4 && sp[0].pattern == 0x40 && sp[1].pattern == 0x44 && sp[2].pattern == 0x50 &&
          sp[3].pattern == 0x54 && sp[0].y == 81 && sp[2].y == 97 && sp[0].x == 92 && sp[0].bank == 1, "dessin pose 09");
    s.enemy[0].pose = 0x0C;
    CHECK(Enemies_sprites(&s, 0, sp) == 4 && sp[0].pattern == 0x68 && sp[2].pattern == 0x78, "dessin pose 0C");
    s.enemy[1].active = 1; s.enemy[1].type = ENEMY_TYPE_SPLASH; s.enemy[1].x = 209; s.enemy[1].y = 195;
    CHECK(Enemies_sprites(&s, 1, sp) == 1 && sp[0].pattern == 0x90 && sp[0].x == 201 && sp[0].y == 181, "dessin goutte");
}

static void test_type2_sat_budget(void)
{
    /* P2 7598 : un homme-poisson (4 entrees SAT) et 4 gouttes (2 chacune) : 12 sur 14, le tir
     * a besoin de 4 entrees, il est perdu ; l'index CF03 avance quand meme. */
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int i;
    vars_stage2_room4(&v);
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    s.enemy[0].active = 1; s.enemy[0].type = ENEMY_TYPE_2; s.enemy[0].x = 208; s.enemy[0].y = 133; s.enemy[0].vy = -0x0800;
    for (i = 1; i <= 4; i++) {
        s.enemy[i].active = 1; s.enemy[i].type = ENEMY_TYPE_SPLASH; s.enemy[i].x = 160; s.enemy[i].y = 180;
    }
    s.type2Count = 1;                         /* le prochain appel de 9CC3 a CF10 & 3 = 0 tire */
    t.phase = 1;
    (void)Enemies_frame(&s, &t, &v);
    CHECK(s.type2Shots == 1 && count_type(&s, ENEMY_TYPE_2) == 1, "tir perdu faute d'entrees SAT");
    CHECK(count_type(&s, ENEMY_TYPE_SPLASH) == 4, "et sans gouttes");
}

/* Contact selon la posture de Simon (8290 -> 847A, C420) : bornes Y de l'objet MSX (pieds)
 * pour chaque classe de 82CB, a X egal, puis bornes X. Simon en (120, 160). Objet en (X, Y) :
 * type 1 romBox et type 4 stockes en coordonnees de sprite (X - 8 ; Y - 31 ou Y - 15). */
static int posture_touch(int type, int romBox, int state, int dx, int objY)
{
    EnemyRoomVars v;
    Enemy e;
    memset(&v, 0, sizeof v);
    memset(&e, 0, sizeof e);
    v.stage = 1;
    v.simonX = 120;
    v.simonY = 160;
    v.simonState = (EnemyU8)state;
    e.active = 1;
    e.type = (EnemyU8)type;
    e.romBox = (EnemyU8)romBox;
    e.x = (EnemyS16)(120 + dx);
    e.y = (EnemyU8)objY;
    if (romBox) { e.x -= 8; e.y = (EnemyU8)(objY - 31); }
    if (type == ENEMY_TYPE_4) { e.x -= 8; e.y = (EnemyU8)(objY - 15); }
    return Enemies_touches(&e, &v);
}

/* Vrai si l'objet touche exactement de Y = 160 + top a Y = 160 + bottom. */
static int posture_span(int type, int romBox, int state, int top, int bottom)
{
    return posture_touch(type, romBox, state, 0, 160 + top) &&
           !posture_touch(type, romBox, state, 0, 160 + top - 1) &&
           posture_touch(type, romBox, state, 0, 160 + bottom) &&
           !posture_touch(type, romBox, state, 0, 160 + bottom + 1);
}

static void test_contact_posture(void)
{
    static const struct { int type, romBox, halfW, stand, crouchTop, jumpBottom; const char *name; } k[4] = {
        { ENEMY_TYPE_A, 1, 10, 22, -19, 14, "type 1 (8378)" },
        { ENEMY_TYPE_2, 0, 10, 22, -19, 14, "type 2 (8378)" },
        { ENEMY_TYPE_4, 0, 13, 14, -19, 6, "type 4 (8390)" },
        { ENEMY_TYPE_5, 0, 17, 8, -19, 0, "type 5 (83D2)" },
    };
    int i;
    for (i = 0; i < 4; i++) {
        const int t = k[i].type, r = k[i].romBox;
        CHECK(posture_span(t, r, ENEMY_SIMON_GROUND, -27, k[i].stand), "%s debout : Y de C425 - 27 a C425 + %d", k[i].name, k[i].stand);
        CHECK(posture_span(t, r, ENEMY_SIMON_CROUCH, k[i].crouchTop, k[i].stand), "%s accroupi (C420 = 2) : haut C425 - 19", k[i].name);
        CHECK(posture_span(t, r, ENEMY_SIMON_JUMP, -27, k[i].jumpBottom), "%s saut (C420 = 1) : bas C425 + %d", k[i].name, k[i].jumpBottom);
        CHECK(posture_span(t, r, ENEMY_SIMON_STAIRS, -27, k[i].stand), "%s escalier (C420 = 3) : boite debout", k[i].name);
        CHECK(posture_span(t, r, ENEMY_SIMON_FALL, -27, k[i].stand), "%s chute (C420 = 4) : boite debout", k[i].name);
        CHECK(posture_touch(t, r, ENEMY_SIMON_CROUCH, k[i].halfW - 1, 160) &&
              !posture_touch(t, r, ENEMY_SIMON_CROUCH, k[i].halfW, 160) &&
              posture_touch(t, r, ENEMY_SIMON_JUMP, 1 - k[i].halfW, 150) &&
              !posture_touch(t, r, ENEMY_SIMON_JUMP, -k[i].halfW, 150), "%s : |dX| < %d", k[i].name, k[i].halfW);
    }

    /* Par Enemies_frame : un zombie (stage 2, salle 2, pieds en A0 comme Simon) touche Simon
     * accroupi ; Simon en saut, pieds 15 px au-dessus du sol, passe au-dessus (debout, il
     * serait touche jusqu'a 22 px). */
    {
        EnemySpawnTimer t;
        EnemyRoomVars v;
        EnemyState s;
        vars_stage2_room2(&v);
        v.simonState = ENEMY_SIMON_CROUCH;
        Enemies_init(&s);
        Enemies_spawnTimerInit(&t);
        put_stage2_type1(&s, 145 + 5, 160, ENEMY_SIDE_RIGHT);
        CHECK(Enemies_frame(&s, &t, &v) == 1 && s.energy == ENEMY_ENERGY_MAX - 2, "zombie : Simon accroupi touche");
        v.simonState = ENEMY_SIMON_JUMP;
        v.simonY = 160 - 15;
        Enemies_init(&s);
        put_stage2_type1(&s, 145 + 5, 160, ENEMY_SIDE_RIGHT);
        CHECK(Enemies_frame(&s, &t, &v) == 0 && s.energy == ENEMY_ENERGY_MAX, "zombie : saut 15 px au-dessus, pas de contact");
        v.simonState = ENEMY_SIMON_GROUND;
        Enemies_init(&s);
        put_stage2_type1(&s, 145 + 5, 160, ENEMY_SIDE_RIGHT);
        CHECK(Enemies_frame(&s, &t, &v) == 1, "zombie : meme hauteur, boite debout, contact");
    }
}

int main(void)
{
    test_rom_tables();
    test_timer_intervals();
    test_timer_reload_and_difficulty();
    test_flag_gate();
    test_guard_right_refused();
    test_guard_left_refused();
    test_side_left();
    test_room1_low_simon();
    test_e1_trajectory();
    test_e2_trajectory();
    test_spawn_limits();
    test_type4_motion();
    test_type4_timer();
    test_contact_recoil_invuln();
    test_items();
    test_spawn_pause();
    test_fall_to_floor();
    test_whip_kills_type1();
    test_type5();
    test_stage2_type1_spawn();
    test_stage2_type1_trajectory();
    test_stage2_type1_contact_whip();
    test_type2_spawn();
    test_type2_trajectory();
    test_type2_contact_whip();
    test_type2_sat_budget();
    test_contact_posture();
    if (failures == 0) {
        printf("Enemies tests: PASS\n");
        return 0;
    }
    printf("Enemies tests: %d FAIL\n", failures);
    return 1;
}
