/*
 * Tests natifs de la logique des ennemis (sgdk/src/enemies.c).
 * Compilation : gcc -std=c99 -Wall -Wextra -Werror -I sgdk/src \
 *   tests/test_enemies.c sgdk/src/enemies.c -o /tmp/test_enemies && /tmp/test_enemies
 *
 * Les valeurs attendues sont lues dans la trace openMSX (frames.txt, salle 1).
 * Image absolue n = 857 + image relative de la salle.
 */
#include <stdio.h>
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

/* Avance jusqu'a l'image absolue n (inclus). */
static void advance_to(EnemyState *s, int *room_frame, int n)
{
    while (*room_frame < n - ROOM_LOAD_IMAGE) {
        Enemies_update(s);
        (*room_frame)++;
    }
}

/* Premier test : trajectoire et pose de l'ennemi E1 (spawn image 908). */
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

    Enemies_setRoom(&s, kEnemyRoom1Spawns, kEnemyRoom1SpawnCount);
    /* Image 908 = spawn : pas encore de mouvement, et E1 est en slot 0. */
    advance_to(&s, &rf, 908);
    CHECK(s.enemy[0].active && s.enemy[0].age == 0, "E1 doit etre actif a l'age 0 (image 908)");
    CHECK(s.enemy[0].dx == -1, "E1 spawn a droite doit avancer vers la gauche");

    for (i = 0; i < sizeof(e1) / sizeof(e1[0]); i++) {
        advance_to(&s, &rf, e1[i].n);
        CHECK(Enemies_sprites(&s, 0, sp) == 1, "E1 inactif a l'image %d", e1[i].n);
        CHECK(sp[0].x == e1[i].x, "E1 x a l'image %d : attendu %d, obtenu %d", e1[i].n, e1[i].x, sp[0].x);
        CHECK(sp[0].y == 0xA0 && sp[1].y == 0xA0, "E1 rang haut doit etre en Y=A0 (image %d)", e1[i].n);
        CHECK(sp[2].y == 0xB0 && sp[3].y == 0xB0, "E1 rang bas doit etre en Y=B0 (image %d)", e1[i].n);
        CHECK(sp[0].pattern == 0x40 && sp[1].pattern == 0x44, "E1 motifs haut (image %d)", e1[i].n);
        CHECK(sp[2].pattern == e1[i].bottom, "E1 motif bas a l'image %d : attendu %02X, obtenu %02X",
              e1[i].n, e1[i].bottom, sp[2].pattern);
    }

    /* Image 1126 : sortie a gauche (x passerait a -2). */
    advance_to(&s, &rf, 1126);
    CHECK(!s.enemy[0].active, "E1 doit etre inactif a l'image 1126");
    CHECK(Enemies_sprites(&s, 0, sp) == 0, "E1 inactif ne doit rien produire");
}

/* Second test : E2, spawn a gauche image 1012, avance vers la droite. */
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

    Enemies_setRoom(&s, kEnemyRoom1Spawns, kEnemyRoom1SpawnCount);
    advance_to(&s, &rf, 1012);
    /* E2 est le second spawn : il est en slot 1 (E1 est deja en slot 0). */
    advance_to(&s, &rf, 1013);
    CHECK(s.enemy[1].active && s.enemy[1].dx == 1, "E2 doit etre actif, avance vers la droite");

    for (i = 0; i < sizeof(e2) / sizeof(e2[0]); i++) {
        advance_to(&s, &rf, e2[i].n);
        CHECK(Enemies_sprites(&s, 1, sp) == 1, "E2 inactif a l'image %d", e2[i].n);
        CHECK(sp[0].x == e2[i].x, "E2 x a l'image %d : attendu %d, obtenu %d", e2[i].n, e2[i].x, sp[0].x);
        CHECK(sp[0].pattern == 0x58 && sp[1].pattern == 0x5C, "E2 motifs haut (image %d)", e2[i].n);
        CHECK(sp[2].pattern == e2[i].bottom, "E2 motif bas a l'image %d : attendu %02X, obtenu %02X",
              e2[i].n, e2[i].bottom, sp[2].pattern);
    }

    advance_to(&s, &rf, 1208);
    CHECK(!s.enemy[1].active, "E2 doit etre inactif a l'image 1208");
}

/* Troisieme test : limites du spawn et de la visibilite. */
static void test_spawn_limits(void)
{
    EnemyState s;
    EnemySprite sp[ENEMY_SPRITES];
    int idx, k;

    Enemies_setRoom(&s, 0, 0);
    for (k = 0; k < ENEMY_MAX; k++) {
        idx = Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
        CHECK(idx == k, "spawn %d attendu en slot %d, obtenu %d", k, k, idx);
    }
    idx = Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    CHECK(idx == -1, "spawn sur liste pleine doit echouer (obtenu %d)", idx);

    Enemies_setRoom(&s, 0, 0);
    idx = Enemies_spawn(&s, ENEMY_SIDE_LEFT, ENEMY_TYPE_B, 0xA0, 15);
    CHECK(idx == 0 && s.enemy[0].x == 0x0A && s.enemy[0].dx == 1,
          "spawn a gauche : x=0A, dx=+1 attendu");
    CHECK(Enemies_sprites(&s, 0, sp) == 1 && sp[0].pattern == 0x58,
          "type B au spawn : motif haut 58");

    CHECK(Enemies_isVisible(&s.enemy[0]), "x=10 doit etre visible");
    s.enemy[0].x = -1;
    CHECK(!Enemies_isVisible(&s.enemy[0]), "x=-1 ne doit pas etre visible");
    s.enemy[0].x = 0xE5;
    CHECK(Enemies_isVisible(&s.enemy[0]), "x=E5 doit etre visible (spawn droit)");
}

int main(void)
{
    test_e1_trajectory();
    test_e2_trajectory();
    test_spawn_limits();
    if (failures == 0) {
        printf("Enemies tests: PASS\n");
        return 0;
    }
    printf("Enemies tests: %d FAIL\n", failures);
    return 1;
}
