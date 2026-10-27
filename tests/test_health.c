/*
 * Tests natifs des vies, de la mort et de la fin de partie (sgdk/src/health.c),
 * du point de reapparition (vk_game_respawn, gameplay.c) et du lien avec le
 * contact ennemi (enemies.c). Valeurs : research/player-health.md 3.3 a 3.5.
 * Compilation : voir tests/run_native.sh.
 */
#include <stdio.h>
#include "health.h"
#include "enemies.h"
#include "gameplay.h"

/* Courtyard tables of intro_assets.c (SGDK types there): empty stubs, unused here. */
const VKByte vk_courtyard_maps[3][768] = {{0}};
const VKByte vk_courtyard_neighbors[3][4] = {{255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};

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

/* Avance de n images a energie constante ; retourne le premier evenement non nul
 * et l'image (1..n) ou il arrive dans *at (0 si aucun). */
static VKHealthEvent run(VKHealth *h, int energy, int n, int *at)
{
    int f;
    *at = 0;
    for (f = 1; f <= n; f++) {
        const VKHealthEvent e = vk_health_tick(h, energy);
        if (e != VK_HEALTH_NONE) { *at = f; return e; }
    }
    return VK_HEALTH_NONE;
}

/* 1. Debut de partie : C410 = 2 une fois le jeu lance, vivant, commandes libres. */
static void test_begin(void)
{
    VKHealth h;
    int at;
    vk_health_begin(&h);
    CHECK(h.lives == 2, "C410 = 2 au debut du jeu (obtenu %d)", h.lives);
    CHECK(!vk_health_controls_locked(&h), "commandes libres au debut");
    CHECK(run(&h, VK_HEALTH_ENERGY_MAX, 1000, &at) == VK_HEALTH_NONE, "rien tant que l'energie est > 0");
    CHECK(run(&h, 2, 10, &at) == VK_HEALTH_NONE, "energie 2 : toujours vivant");
}

/* 2. Mort avec vies : 252 images, refill, 27 images, C410 - 1 (run 4 : 2 -> 1). */
static void test_death_with_lives(void)
{
    VKHealth h;
    int at;
    vk_health_begin(&h);
    CHECK(vk_health_tick(&h, 0) == VK_HEALTH_DIED, "energie 0 : mort");
    CHECK(vk_health_controls_locked(&h), "commandes bloquees pendant la mort");
    CHECK(run(&h, 0, 400, &at) == VK_HEALTH_REFILL && at == VK_HEALTH_DEATH_FRAMES,
          "etat 6 (refill) 252 images apres le contact (obtenu image %d)", at);
    CHECK(h.lives == 2, "C410 inchange avant l'etat 4");
    CHECK(run(&h, VK_HEALTH_ENERGY_MAX, 100, &at) == VK_HEALTH_RESPAWN && at == VK_HEALTH_RESPAWN_FRAMES,
          "etat 4 27 images apres l'etat 6 (obtenu image %d)", at);
    CHECK(h.lives == 1, "C410 2 -> 1 (obtenu %d)", h.lives);
    CHECK(!vk_health_controls_locked(&h), "commandes libres apres la reapparition");
}

/* 3. Trois morts : la troisieme, avec C410 = 0, mene a la fin de partie puis au titre. */
static void test_game_over(void)
{
    VKHealth h;
    int at, death;
    vk_health_begin(&h);
    for (death = 0; death < 2; death++) {
        CHECK(vk_health_tick(&h, 0) == VK_HEALTH_DIED, "mort %d", death + 1);
        CHECK(run(&h, 0, 400, &at) == VK_HEALTH_REFILL, "mort %d : refill", death + 1);
        CHECK(run(&h, VK_HEALTH_ENERGY_MAX, 100, &at) == VK_HEALTH_RESPAWN, "mort %d : reapparition", death + 1);
    }
    CHECK(h.lives == 0, "C410 = 0 apres deux morts");
    CHECK(vk_health_tick(&h, 0) == VK_HEALTH_DIED, "troisieme mort");
    CHECK(run(&h, 0, 400, &at) == VK_HEALTH_GAME_OVER && at == VK_HEALTH_DEATH_FRAMES,
          "etat 6 sans vie : fin de partie a l'image 252 (obtenu %d)", at);
    CHECK(vk_health_controls_locked(&h), "commandes bloquees pendant la fin de partie");
    CHECK(run(&h, 0, 1000, &at) == VK_HEALTH_TITLE && at == VK_HEALTH_GAMEOVER_FRAMES,
          "titre 378 images apres l'etat 6 (obtenu %d)", at);
    CHECK(VK_HEALTH_DEATH_FRAMES + VK_HEALTH_GAMEOVER_FRAMES == 630,
          "contact -> titre : 40,617 s -> 51,13 s, environ 630 images");
}

/* 4. Largeur de la barre : 64 px pour 32 points, plancher et plafond. */
static void test_bar(void)
{
    CHECK(vk_health_bar_pixels(ENEMY_ENERGY_MAX) == 64, "energie pleine : 64 px");
    CHECK(vk_health_bar_pixels(30) == 60, "un contact : 60 px");
    CHECK(vk_health_bar_pixels(1) == 2, "1 point : 2 px");
    CHECK(vk_health_bar_pixels(0) == 0, "0 : vide");
    CHECK(vk_health_bar_pixels(-3) == 0 && vk_health_bar_pixels(200) == 64, "bornes");
    CHECK(VK_HEALTH_ENERGY_MAX == ENEMY_ENERGY_MAX, "meme maximum que enemies.h (C415 = 0x20)");
}

/* 5. Chaine complete : contacts ennemis jusqu'a 0, puis mort. */
static void test_contact_to_death(void)
{
    EnemyState s;
    EnemySpawnTimer t;
    EnemyRoomVars v;
    VKHealth h;
    int f, push, died = 0;
    v.stage = 1;
    v.room = 2;        /* salle sans spawn : seul l'ennemi de test existe */
    v.simonX = 120;
    v.simonY = 192;
    v.sideFlag = 0;
    v.difficulty = 0;
    v.map = 0;
    Enemies_init(&s);
    Enemies_spawnTimerInit(&t);
    vk_health_begin(&h);
    s.energy = 4;
    (void)Enemies_spawn(&s, ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1);
    for (f = 1; f <= 400 && !died; f++) {
        s.enemy[0].x = 126;
        s.enemy[0].exiting = 0;
        (void)Enemies_frame(&s, &t, &v, &push);
        if (vk_health_tick(&h, s.energy) == VK_HEALTH_DIED) died = f;
    }
    CHECK(died == 181, "deuxieme contact (image 181, apres l'invulnerabilite) : mort (obtenu %d)", died);
    CHECK(s.energy == 0, "energie a 0");
}

/* 6. Reapparition (provisoire) : entree du hall, ou debut de la cour. */
static void test_respawn_place(void)
{
    VKGame g;
    vk_game_begin(&g);
    (void)vk_game_debug_room(&g, 5);
    g.player.x = 40;
    vk_game_respawn(&g);
    CHECK(g.area == VK_AREA_INTERIOR && g.room == 0, "hall : salle 0");
    CHECK(g.player.x == 16 && g.player.y == 192, "hall : x 16, y 192 (entree)");
    vk_game_begin(&g);
    g.room = 2;
    g.player.x = 100;
    vk_game_respawn(&g);
    CHECK(g.area == VK_AREA_COURTYARD && g.room == 0 && g.player.x == 16 && g.player.y == 176,
          "cour : debut de partie");
}

int main(void)
{
    test_begin();
    test_death_with_lives();
    test_game_over();
    test_bar();
    test_contact_to_death();
    test_respawn_place();
    if (failures) {
        printf("test_health: %d echec(s)\n", failures);
        return 1;
    }
    printf("test_health: OK\n");
    return 0;
}
