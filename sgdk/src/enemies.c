/*
 * Logique pure des ennemis de la salle 1 (voir enemies.h et research/enemy-spawn.md).
 *
 * Toutes les valeurs ci-dessous sont des observations de la trace openMSX
 * (research/enemies-platforms.md, frames.txt), sauf mention contraire.
 * Image de reference : chargement de la salle 1 = image 857 (E000-E47F reecrit
 * en 857-858). Le premier spawn est a l'image 908, soit frame = 51.
 *
 * Confiance par comportement :
 *   - X initial : E5 (cote droit) ou 0A (cote gauche)            observe
 *   - pas : +-2 px toutes les 2 images a partir de l'age 3        observe
 *   - pas supplementaire : +-1 px tous les 16 images (phase)       observe, cause inconnue
 *   - pose : bas A 8 images / bas B 8 images, 1re pose B a l'age 15 observe
 *   - sortie : limite atteinte (x <= 0 si dx < 0 ; x >= 216 si dx > 0), masque a l'image suivante
 *                                                                  observe (2 cas), TODO(non-verifie) pour la regle generale
 *   - declencheur et frame des spawns : observes seulement pour la salle 1
 *     TODO(non-verifie) : table ROM et declencheur (voir research/enemy-spawn.md)
 *
 * Age : 0 = image de spawn (sprite encore hors position, Y=E1 dans la trace),
 *       1 = premiere image visible.
 */
#include "enemies.h"

#define ENEMY_SPAWN_X_RIGHT 0xE5   /* 229 : cote droit, observe (SAT X de 909) */
#define ENEMY_SPAWN_X_LEFT  0x0A   /* 10 : cote gauche, observe (SAT X de 1013) */
#define ENEMY_EXIT_X_LEFT   0      /* dx < 0 : limite x <= 0 ; masque a l'image suivante (observe 1125 -> 1126) */
#define ENEMY_EXIT_X_RIGHT  216    /* dx > 0 : limite x >= 216 ; masque a l'image suivante (observe 1207 -> 1208) */
#define ENEMY_ROW_OFFSET    16     /* rang bas = Y + 16 (observe B0 pour A0) */

/* Salle 1 : deux spawns observes dans le premier cycle d'ecran.
 * Cycle ulterieur (apres changement d'ecran C000=9) : non inclus, relation au
 * declencheur inconnue. Observes (frame absolue -> cote, Y, type) :
 *   1256 gauche (X=0 a l'apparition), 1360 droite (X=D8), 1585 gauche (X=0). */
const EnemySpawnDef kEnemyRoom1Spawns[] = {
    /* frame, side, type, y, extraPhase */
    { 51,  ENEMY_SIDE_RIGHT, ENEMY_TYPE_A, 0xA0, 1 },   /* 908 : E1 (4 entrees SAT s14-s17) */
    { 155, ENEMY_SIDE_LEFT,  ENEMY_TYPE_B, 0xA0, 15 }   /* 1012 : E2 (s18-s21) */
};
const uint8_t kEnemyRoom1SpawnCount = (uint8_t)(sizeof(kEnemyRoom1Spawns) / sizeof(kEnemyRoom1Spawns[0]));

/* Motifs : [type][pose][entree] ; entrees 0,1 = rang haut ; 2,3 = rang bas. */
static const uint8_t kPatterns[2][2][4] = {
    { { 0x40, 0x44, 0x48, 0x4C },   /* type A, pose A */
      { 0x40, 0x44, 0x50, 0x54 } }, /* type A, pose B */
    { { 0x58, 0x5C, 0x60, 0x64 },   /* type B, pose A */
      { 0x58, 0x5C, 0x68, 0x6C } }  /* type B, pose B */
};

/* Pose B : a partir de l'age 15, par blocs de 8 images (observe). */
static uint8_t poseOf(uint16_t age)
{
    if (age >= 15 && ((age - 15) % 16) < 8) return 1;
    return 0;
}

static void clearEnemy(Enemy *e)
{
    e->active = 0;
    e->exiting = 0;
    e->type = 0;
    e->extraPhase = 0;
    e->x = 0;
    e->y = 0;
    e->dx = 0;
    e->age = 0;
}

void Enemies_setRoom(EnemyState *s, const EnemySpawnDef *spawns, uint8_t count)
{
    uint8_t i;
    for (i = 0; i < ENEMY_MAX; i++) clearEnemy(&s->enemy[i]);
    s->roomFrame = 0;
    s->nextSpawn = 0;
    s->spawns = spawns;
    s->spawnCount = count;
}

int Enemies_isVisible(const Enemy *e)
{
    if (!e->active) return 0;
    if (e->x < 0 || e->x > 255) return 0;
    return 1;
}

int Enemies_spawn(EnemyState *s, uint8_t side, uint8_t type, uint8_t y, uint8_t extraPhase)
{
    uint8_t i;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (!s->enemy[i].active) {
            Enemy *e = &s->enemy[i];
            e->active = 1;
            e->exiting = 0;
            e->type = type;
            e->extraPhase = extraPhase;
            e->y = y;
            e->age = 0;
            if (side == ENEMY_SIDE_RIGHT) {
                e->x = ENEMY_SPAWN_X_RIGHT;
                e->dx = -1;
            } else {
                e->x = ENEMY_SPAWN_X_LEFT;
                e->dx = 1;
            }
            return (int)i;
        }
    }
    return -1; /* plein : spawn perdu (TODO(non-verifie) : comportement MSX si plein) */
}

/* Une image pour un ennemi deja actif. */
static void advance(Enemy *e)
{
    if (e->exiting) {          /* masque a l'image suivant la limite (observe 1126, 1208) */
        e->active = 0;
        return;
    }
    e->age++;
    /* Pas de base : 2 px toutes les 2 images, a partir de l'age 3 (observe). */
    if (e->age >= 3 && (e->age & 1)) {
        e->x += 2 * e->dx;
    }
    /* Pas supplementaire : 1 px tous les 16 images, phase extraPhase (observe). */
    if (e->age >= 3 && ((e->age - e->extraPhase) % 16) == 0) {
        e->x += e->dx;
    }
    if (e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) e->exiting = 1;
    if (e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT) e->exiting = 1;
}

void Enemies_update(EnemyState *s)
{
    uint8_t i;

    /* 1. avancer les ennemis existants */
    for (i = 0; i < ENEMY_MAX; i++) {
        if (s->enemy[i].active) advance(&s->enemy[i]);
    }

    /* 2. declencher les spawns dus a cette image */
    s->roomFrame++;
    while (s->spawns != 0 && s->nextSpawn < s->spawnCount &&
           s->spawns[s->nextSpawn].frame == s->roomFrame) {
        const EnemySpawnDef *d = &s->spawns[s->nextSpawn];
        (void)Enemies_spawn(s, d->side, d->type, d->y, d->extraPhase);
        s->nextSpawn++;
    }
}

int Enemies_sprites(const EnemyState *s, uint8_t index, EnemySprite out[ENEMY_SPRITES])
{
    const Enemy *e;
    uint8_t pose, t, k;

    if (index >= ENEMY_MAX) return 0;
    e = &s->enemy[index];
    if (!e->active) return 0;

    pose = poseOf(e->age);
    t = e->type ? 1 : 0;
    for (k = 0; k < ENEMY_SPRITES; k++) {
        out[k].y = (uint8_t)(k < 2 ? e->y : e->y + ENEMY_ROW_OFFSET);
        out[k].x = e->x;
        out[k].pattern = kPatterns[t][k < 2 ? 0 : pose][k];
    }
    return 1;
}
