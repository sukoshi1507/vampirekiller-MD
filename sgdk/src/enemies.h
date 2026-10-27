/*
 * Logique pure des ennemis de la salle 1 (Vampire Killer MSX2 -> Mega Drive).
 *
 * Aucune dependance a SGDK, a main.c ou au materiel : module testable sur PC
 * (tests/test_enemies.c). L'integration dans la boucle de jeu est une etape
 * ulterieure.
 *
 * Provenance (voir research/enemy-spawn.md) :
 *   - Tout le comportement de mouvement, d'animation et de spawn decrit ici
 *     vient de la trace openMSX research/enemies-platforms.md (images 857-1208,
 *     frames.txt), niveau de confiance : observe.
 *   - La table de spawn ROM n'est PAS identifiee. Les SpawnDef de la salle 1
 *     sont des observations de trace, pas une table extraite de la ROM.
 *     TODO(non-verifie) : declencheur exact (minuterie, position, compteur)
 *     et cycle de reapparition (intervalles 111, 130, 152 images).
 */
#ifndef VK_ENEMIES_H
#define VK_ENEMIES_H

#include <stdint.h>

#define ENEMY_MAX 4          /* ennemis simultanes geres */
#define ENEMY_SPRITES 4      /* entrees SAT par ennemi (2 haut, 2 bas) */

/* Cote d'apparition, en repere d'ecran (observe). */
typedef enum {
    ENEMY_SIDE_RIGHT = 0,    /* apparait a droite, avance vers la gauche (dx = -1) */
    ENEMY_SIDE_LEFT = 1      /* apparait a gauche, avance vers la droite (dx = +1) */
} EnemySide;

/* Type d'ennemi : jeu de motifs. Observe sur deux types en salle 1. */
typedef enum {
    ENEMY_TYPE_A = 0,        /* motifs 40/44 (haut), 48/4C <-> 50/54 (bas) */
    ENEMY_TYPE_B = 1         /* motifs 58/5C (haut), 60/64 <-> 68/6C (bas) */
} EnemyType;

/* Un spawn programme pour une salle. */
typedef struct {
    uint16_t frame;          /* image de spawn, relative au chargement de la salle */
    uint8_t side;            /* EnemySide */
    uint8_t type;            /* EnemyType */
    uint8_t y;               /* Y sprite du rang haut (observe 0xA0) */
    uint8_t extraPhase;      /* pas supplementaire de 1 px quand age = extraPhase (mod 16), observe : 1 ou 15 */
} EnemySpawnDef;

/* Un ennemi actif. x est la coordonnee SAT (pixels, signee pour le hors-ecran). */
typedef struct {
    uint8_t active;
    uint8_t exiting;         /* limite atteinte : masque a l'image suivante (observe) */
    uint8_t type;
    uint8_t extraPhase;
    int16_t x;
    uint8_t y;
    int8_t dx;               /* -1 ou +1 */
    uint16_t age;            /* images depuis le spawn ; 0 = image de spawn */
} Enemy;

/* Une entree SAT produite pour l'affichage. */
typedef struct {
    uint8_t y;
    int16_t x;
    uint8_t pattern;
} EnemySprite;

typedef struct {
    Enemy enemy[ENEMY_MAX];
    uint16_t roomFrame;      /* images depuis le debut de la salle */
    uint8_t nextSpawn;       /* index du prochain SpawnDef a declencher */
    const EnemySpawnDef *spawns;
    uint8_t spawnCount;
} EnemyState;

/* Donnees de la salle 1 (observees dans la trace, voir enemies.c). */
extern const EnemySpawnDef kEnemyRoom1Spawns[];
extern const uint8_t kEnemyRoom1SpawnCount;

/* Remet a zero l'etat et installe la liste de spawns de la salle. */
void Enemies_setRoom(EnemyState *s, const EnemySpawnDef *spawns, uint8_t count);

/* Avance d'une image : declenche les spawns dus, puis met a jour les ennemis. */
void Enemies_update(EnemyState *s);

/* Spawn immediat (utile aux tests). Retourne l'index ou -1 si plein. */
int Enemies_spawn(EnemyState *s, uint8_t side, uint8_t type, uint8_t y, uint8_t extraPhase);

/* Ecrit les 4 entrees SAT d'un ennemi actif dans out[0..3].
 * Retourne 0 si l'ennemi est inactif (rien ecrit), 1 sinon. */
int Enemies_sprites(const EnemyState *s, uint8_t index, EnemySprite out[ENEMY_SPRITES]);

/* 1 si l'ennemi est visible a l'ecran (x dans 0..216 pour les deux sens). */
int Enemies_isVisible(const Enemy *e);

#endif /* VK_ENEMIES_H */
