/*
 * Logique pure des ennemis de la salle 1 (Vampire Killer MSX2 -> Mega Drive).
 *
 * Aucune dependance a SGDK, a main.c ou au materiel : module testable sur PC
 * (tests/test_enemies.c). L'integration dans la boucle de jeu est une etape
 * ulterieure.
 *
 * Provenance (voir research/enemy-spawn.md, section 7) :
 *   - Tables : extraites de la ROM (valeurs, adresses CPU indiquees), verifie.
 *   - Minuterie, garde de position et cote de spawn : lus dans le code Z80
 *     (routines 9CC3, 9CE6, 9CFC, 9E16, 5F1D), verifie par trace openMSX
 *     (spawns 908, 1012, 1256, 1360 et refus a 1164 et 1512).
 *   - Mouvement, animation et sortie : observes dans la trace (frames.txt).
 *   - Type <-> bloc d'entite, extraPhase, Y = E - 0x20 : observes, cause
 *     non prouvee (voir TODO dans enemies.c).
 */
#ifndef VK_ENEMIES_H
#define VK_ENEMIES_H

#include <stdint.h>

#define ENEMY_MAX 4          /* ennemis simultanes geres (blocs C800/C880/... dans la ROM : ordre inconnu au-dela de 2) */
#define ENEMY_SPRITES 4      /* entrees SAT par ennemi (2 haut, 2 bas) */

/* Cote d'apparition, en repere d'ecran (observe). */
typedef enum {
    ENEMY_SIDE_RIGHT = 0,    /* apparait a droite, avance vers la gauche (dx = -1) */
    ENEMY_SIDE_LEFT = 1      /* apparait a gauche, avance vers la droite (dx = +1) */
} EnemySide;

/* Type d'ennemi : jeu de motifs. */
typedef enum {
    ENEMY_TYPE_A = 0,        /* motifs 40/44 (haut), 48/4C <-> 50/54 (bas) */
    ENEMY_TYPE_B = 1         /* motifs 58/5C (haut), 60/64 <-> 68/6C (bas) */
} EnemyType;

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
} EnemyState;

/* Tables ROM, salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags[D001] : octet de drapeaux ROM 85CF-85D6 ; bit0 = canal
 *   du type 1 actif (appel de 9CE6 depuis 5EE7-5EF6). Autres bits : canaux non portes.
 * kEnemySpawnPeriod[i] : ROM 9D43-9D4A, periode de la sequence (index = CF01 & 7). */
extern const uint8_t kEnemyStage1RoomFlags[8];
extern const uint8_t kEnemySpawnPeriod[8];

/* Nombre d'entrees SAT d'un ennemi de type 1 : ROM 6058 (table 6057 + 1), valeur 4. */
#define ENEMY_TYPE1_SAT_COUNT 4

/* Etat de la minuterie du canal type 1 (adresses MSX : CF10, CF00, CF01). */
typedef struct {
    uint8_t phase;           /* CF10 : compteur partage, decremente a chaque appel de 9CC3 */
    uint8_t counter;         /* CF00 : compte a rebours, decremente tous les 4 appels */
    uint8_t index;           /* CF01 : index dans kEnemySpawnPeriod (incremente a chaque tir) */
} EnemySpawnTimer;

/* Variables MSX lues par la minuterie et la garde de spawn. */
typedef struct {
    uint8_t stage;           /* D000 */
    uint8_t room;            /* D001 */
    uint8_t simonX;          /* C427 */
    uint8_t simonY;          /* C425 */
    uint8_t sideFlag;        /* C42C : 0 = spawn a droite, autre = a gauche (salle D001 != 1) */
    uint8_t difficulty;      /* D012 : soustrait 2*D012 a la periode (valeur 0 observee) */
} EnemyRoomVars;

/* Remet a zero les ennemis (pas de liste de spawns : le spawn vient de la minuterie). */
void Enemies_setRoom(EnemyState *s);

/* Etat initial de la minuterie : CF00=04, CF01=00 (ROM 9CBB, ecrit par 9CA9 appele en 6324). */
void Enemies_spawnTimerInit(EnemySpawnTimer *t);

/* Un appel de 9CC3/9CE6 (observe : une fois toutes les 2 images).
 * Retourne 1 si un ennemi a ete cree, 0 sinon (minuterie, drapeau, garde, plein).
 * Les etats hors stage 1 ne sont pas portes : TODO(non-verifie), retour 0. */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s);

/* Avance d'une image les ennemis actifs (mouvement, animation, sortie). */
void Enemies_update(EnemyState *s);

/* Spawn immediat (utile aux tests). Retourne l'index ou -1 si plein. */
int Enemies_spawn(EnemyState *s, uint8_t side, uint8_t type, uint8_t y, uint8_t extraPhase);

/* Ecrit les 4 entrees SAT d'un ennemi actif dans out[0..3].
 * Retourne 0 si l'ennemi est inactif (rien ecrit), 1 sinon. */
int Enemies_sprites(const EnemyState *s, uint8_t index, EnemySprite out[ENEMY_SPRITES]);

/* 1 si l'ennemi est visible a l'ecran (x dans 0..255 : limites de sortie gerees par update). */
int Enemies_isVisible(const Enemy *e);

#endif /* VK_ENEMIES_H */
