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

/* Driver de test : une image = Enemies_update, puis spawns observes (908 et 1012). */
static void frame_step(EnemyState *s, int *rf)
{
    Enemies_update(s);
    (*rf)++;
    if (*rf == SPAWN_E1_REL) (void)Enemies_spawn(s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    if (*rf == SPAWN_E2_REL) (void)Enemies_spawn(s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
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
        { 1125, 0,  0x48 },  /* 00 : derniere position visible */
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

    advance_to(&s, &rf, 1126);
    CHECK(!s.enemy[0].active, "E1 doit etre inactif a l'image 1126");
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
        { 1207, 216, 0x68 }, /* D8 : derniere position visible */
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

    advance_to(&s, &rf, 1208);
    CHECK(!s.enemy[1].active, "E2 doit etre inactif a l'image 1208");
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

/* 6. Type 4 par Enemies_frame : un tir tous les 168 images, cote droit puis gauche. */
static void test_type4_timer(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int f, push;

    v.stage = 1;
    v.room = 4;
    v.map = 0;
    v.simonX = 0x7A;
    v.simonY = 0x40;   /* hors de la zone des ennemis : pas de contact dans ce test */
    v.sideFlag = 0;
    v.difficulty = 0;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    for (f = 1; f <= 167; f++) (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(!s.enemy[0].active, "type 4 : aucun tir avant 168 images");
    (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(s.enemy[0].active && s.enemy[0].type == ENEMY_TYPE_4 && s.enemy[0].dx == -1 && s.enemy[0].x == 0xE5,
          "type 4 : premier tir a 168 images, cote droit");
    for (f = 169; f <= 335; f++) (void)Enemies_frame(&s, &t, &v, &push);
    (void)Enemies_frame(&s, &t, &v, &push);
    /* Le premier tir est sorti vers 320 images : son slot (0) est reutilise. */
    CHECK(s.enemy[0].active && s.enemy[0].type == ENEMY_TYPE_4 && s.enemy[0].dx == 1 && s.enemy[0].x == 0x0B,
          "type 4 : second tir a 336 images, cote gauche (slot libere reutilise)");
    CHECK(!s.enemy[1].active, "type 4 : un seul ennemi actif apres le second tir");
    CHECK(s.energy == ENEMY_ENERGY_MAX, "pas de contact dans ce test");

    /* Salle 2 (drapeau 0) : ni type 1 ni type 4. */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    v.room = 2;
    for (f = 1; f <= 400; f++) (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(!s.enemy[0].active && !s.enemy[1].active, "salle 2 : aucun ennemi de type 1 ni 4");
}

/* 7. Contact : -2, recul de 22 images a 2 px, invulnerabilite de 180 images. Ennemi maintenu sur Simon. */
static void test_contact_recoil_invuln(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int f, push, sum = 0, contacts[8], n = 0;

    v.stage = 1;
    v.room = 2;        /* salle sans spawn : seul l'ennemi de test existe */
    v.simonX = 120;
    v.simonY = 192;
    v.sideFlag = 0;
    v.difficulty = 0;
    v.map = 0;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    CHECK(s.energy == 0x20, "energie de depart 0x20");
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    for (f = 1; f <= 400; f++) {
        s.enemy[0].x = 126;            /* maintenu en contact avec Simon */
        s.enemy[0].exiting = 0;
        if (Enemies_frame(&s, &t, &v, &push) && n < 8) contacts[n++] = f;
        if (f >= 2 && f <= 23) sum += push;
        if (f == 1) CHECK(push == 0 && s.invuln == ENEMY_INVULN_START, "contact image 1 : invulnerabilite 0x5A");
        if (f == 24) CHECK(push == 0, "recul termine a l'image 23 (attendu push 0 a 24)");
    }
    CHECK(sum == -44, "recul : 22 images a -2 px (attendu -44, obtenu %d)", sum);
    CHECK(n == 3 && contacts[0] == 1 && contacts[1] == 181 && contacts[2] == 361,
          "contacts aux images 1, 181, 361 (obtenu n=%d)", n);
    CHECK(s.energy == 0x20 - 3 * ENEMY_CONTACT_DAMAGE, "energie : 0x20 - 3 contacts x 2");
}

/* 8. Chute : sans sol sous lui un ennemi descend ; posé sur le sol de la salle (ligne 22 :
 * pieds en Y 192), il s'arrete. Carte synthetique : une seule rangee de tuiles pleines. */
static void test_fall_to_floor(void)
{
    static uint8_t map[768];
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    int push;
    int i;

    memset(map, 0, sizeof map);
    v.stage = 1;
    v.room = 2;
    v.simonX = 0xF0;      /* loin de l'ennemi : pas de contact dans ce test */
    v.simonY = 0x40;
    v.sideFlag = 0;
    v.difficulty = 0;
    v.map = map;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    (void)Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    /* Pas de sol : chute de ENEMY_FALL_SPEED par image. */
    (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(s.enemy[0].y == 0xA0 + ENEMY_FALL_SPEED, "chute sans sol : 2 px par image (obtenu %d)", s.enemy[0].y);
    /* Sol en rangee 22 de la carte (ligne de 32 tuiles, pieds a Y 192). Le spawn a 0xA0
     * (pieds 191) tombe d'un pixel et s'arrete. */
    memset(map, 0, sizeof map);
    for (i = 0; i < 32; i++) map[22 * 32 + i] = 1;
    Enemies_init(&s);
    (void)Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(s.enemy[0].y == 0xA1, "chute sur sol : s'arrete a Y 0xA1 (pieds 192), obtenu %d", s.enemy[0].y);
    (void)Enemies_frame(&s, &t, &v, &push);
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
    int push = 0, f, k;
    memset(map, 0, sizeof map);
    for (k = 0; k < 32; k++) map[18 * 32 + k] = 1;   /* sol en Y A0..A7 */
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    memset(&v, 0, sizeof v);
    v.stage = 1; v.room = 2; v.simonX = 10; v.simonY = 192; v.map = map;
    for (f = 1; f < ENEMY_TYPE5_DELAY; f++) (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(!e->active, "type 5 : pas encore cree a l'image %d", f - 1);
    (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(e->active && e->type == ENEMY_TYPE_5 && e->x == 0x80 && e->y == 0x80 && e->pose == 0x3F,
          "type 5 : cree en 80,80 assis a gauche");
    CHECK(Enemies_sprites(&s, 0, sp) == 4 && sp[0].pattern == 0x40 && sp[1].pattern == 0x44 &&
          sp[2].pattern == 0x48 && sp[3].pattern == 0x4C && sp[0].x == 0x80 - 9 && sp[2].x == 0x80 + 7 &&
          sp[0].y == 0x80 - 15 && sp[0].bank == 1, "type 5 : pose 3F en X-9 / X+7");
    for (k = 0; k < 4; k++) (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(e->state == 0 && e->x == 0x80, "type 5 : Simon a 118 px, reste assise");
    v.simonX = 0x80 + 62;
    (void)Enemies_frame(&s, &t, &v, &push);
    if (e->state == 0) (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(e->state == 1 && e->dx == 1 && e->x == 0x84 && e->pose == 0x3F,
          "type 5 : activation a 62 px, 4 px vers Simon, pose assise une mise a jour");
    v.simonX = 0;   /* pas de demi-tour en course ni en saut */
    for (k = 0; k < 13; k++) {
        (void)Enemies_frame(&s, &t, &v, &push);
        (void)Enemies_frame(&s, &t, &v, &push);
        CHECK(e->y == jumpY[k], "type 5 : saut pas %d, Y %02X attendu %02X", k, jumpY[k], e->y);
        CHECK(e->state == 2 && e->pose == 0x46, "type 5 : saut pas %d, pose 46", k);
    }
    CHECK(e->x == 0x84 + 4 * 13, "type 5 : 4 px par mise a jour en saut (X %d)", e->x);
    (void)Enemies_frame(&s, &t, &v, &push);
    (void)Enemies_frame(&s, &t, &v, &push);
    CHECK(e->state == 1 && e->y == 0xA0 && e->sub == 0 && e->vy == 0 && e->dx == -1,
          "type 5 : posee en A0, repart vers Simon");
    (void)Enemies_frame(&s, &t, &v, &push);
    (void)Enemies_frame(&s, &t, &v, &push);
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
    CHECK(Enemies_frame(&s, &t, &v, &push) == 1 && s.energy == 26 && s.recoilDir == 1,
          "type 5 : contact -6, recul a l'oppose");
    /* La panthere s'est reveillee : Simon 28 px plus bas qu'elle est hors de la boite. */
    s.invuln = 0; s.recoil = 0; s.energy = 32; v.simonX = (EnemyU8)e->x; v.simonY = 0xA0 + 28;
    CHECK(Enemies_frame(&s, &t, &v, &push) == 0 && s.energy == 32, "type 5 : Y = C425 - 28, pas de contact");

    /* Fouet : debout, il passe au-dessus d'une panthere au niveau de Simon ; accroupi, il la tue. */
    e->x = 100; e->y = 0xA0;
    CHECK(Enemies_whipHitType5(&s, 120, 0xA0, 1, 0) == 0 && e->active, "type 5 : fouet debout au-dessus");
    CHECK(Enemies_whipHitType5(&s, 120, 0xA0, 0, 1) == 0 && e->active, "type 5 : fouet accroupi du mauvais cote");
    CHECK(Enemies_whipHitType5(&s, 120, 0xA0, 1, 1) == 1 && !e->active, "type 5 : fouet accroupi, mort");
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
    test_fall_to_floor();
    test_whip_kills_type1();
    test_type5();
    if (failures == 0) {
        printf("Enemies tests: PASS\n");
        return 0;
    }
    printf("Enemies tests: %d FAIL\n", failures);
    return 1;
}
