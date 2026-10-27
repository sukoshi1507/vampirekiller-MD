/*
 * Logique pure des ennemis du hall, stage 1 (Vampire Killer MSX2 -> Mega Drive).
 *
 * Aucune dependance a SGDK, a main.c ou au materiel : module testable sur PC
 * (tests/test_enemies.c). main.c l'appelle une fois par image de jeu (Enemies_frame)
 * et dessine les entrees de Enemies_sprites avec les tuiles de generated/enemy_sprites_md.
 *
 * Provenance (research/enemy-spawn.md, research/enemy-behavior.md, research/player-health.md) :
 *   - Tables de drapeaux, periodes : ROM, verifie.
 *   - Minuterie type 1 et garde de spawn : code Z80 (9CC3, 9CE6, 9CFC, 9E16), verifie en trace.
 *   - Cote de spawn = type (A a droite, B a gauche) : research/enemy-behavior.md section 6.3.
 *   - Mouvement type 1 : trajectoire mesuree (research/enemy-spawn.md, frames.txt).
 *   - Type 4 : cadence 168 images et mouvement 1,5 px/image mesures (enemy-behavior 3.3).
 *   - Contact, degat -2, recul, invulnerabilite : mesures (player-health 3.1, 3.2).
 *   - Boite de contact de Simon, cote du spawn type 1 (C42C) : PROVISOIRES (voir enemies.c).
 */
#ifndef VK_ENEMIES_H
#define VK_ENEMIES_H

/* Integer types spelled out: SGDK defines s8/u8 itself and clashes with <stdint.h>. */
typedef unsigned char EnemyU8;
typedef signed char EnemyS8;
typedef unsigned short EnemyU16;
typedef short EnemyS16;

#define ENEMY_MAX 4          /* ennemis simultanes (les blocs C800/C880 de la ROM : ordre au-dela de 2 inconnu) */
#define ENEMY_SPRITES 4      /* entrees SAT d'un ennemi de type A ou B (2 cellules 16x16, 2 couches chacune) */
#define ENEMY_TYPE4_SPRITES 2 /* entrees SAT d'un ennemi de type 4 (une cellule 16x16, 2 couches) */

#define ENEMY_ENERGY_MAX 0x20       /* C415 : energie de Simon au depart (research/player-health.md) */
#define ENEMY_CONTACT_DAMAGE 2      /* -2 par contact (observe) */
#define ENEMY_INVULN_START 0x5A     /* C42D : decremente une fois sur deux images, fenetre mesuree 180 images */
#define ENEMY_RECOIL_FRAMES 22      /* C427 : 2 px par image, de 136 a 92 (44 px) */
#define ENEMY_RECOIL_STEP 2
#define ENEMY_TYPE4_PERIOD 168      /* 8 x 21 images entre deux tirs (mesure, 4 cas) */
#define ENEMY_TYPE4_Y 0xAA          /* Y SAT du type 4 (observe) */
#define ENEMY_FALL_SPEED 2          /* px par image quand le sol manque (PROVISOIRE, vitesse non mesuree) */

/* Cote d'apparition, en repere d'ecran (observe). */
typedef enum {
    ENEMY_SIDE_RIGHT = 0,    /* apparait a droite, avance vers la gauche (dx = -1) */
    ENEMY_SIDE_LEFT = 1      /* apparait a gauche, avance vers la droite (dx = +1) */
} EnemySide;

/* Type d'ennemi : jeu de motifs et mouvement. Le type 1 a deux sens : A (droite) et B (gauche). */
typedef enum {
    ENEMY_TYPE_A = 0,        /* type 1, sens droite : motifs 40/44 (haut), 48/4C <-> 50/54 (bas) */
    ENEMY_TYPE_B = 1,        /* type 1, sens gauche : motifs 58/5C (haut), 60/64 <-> 68/6C (bas) */
    ENEMY_TYPE_4 = 2         /* type 4 (salle 4) : une cellule 48/4C (droite) ou 60/64 (gauche), sans pose */
} EnemyType;

/* Un ennemi actif. x est la coordonnee SAT (pixels, signee pour le hors-ecran). */
typedef struct {
    EnemyU8 active;
    EnemyU8 exiting;         /* limite atteinte : masque a l'image suivante (observe) */
    EnemyU8 type;
    EnemyU8 extraPhase;      /* type 1 : decalage de la pose B (observe, cause inconnue) */
    EnemyS16 x;
    EnemyU8 y;
    EnemyS8 dx;               /* -1 ou +1 */
    EnemyU8 sub;             /* type 4 : demi-pas du 1,5 px par image */
    EnemyU16 age;            /* images depuis le spawn ; 0 = image de spawn */
} Enemy;

/* Une entree SAT produite pour l'affichage. */
typedef struct {
    EnemyU8 y;
    EnemyS16 x;
    EnemyU8 pattern;
} EnemySprite;

/* Etat des ennemis et de Simon vis-a-vis d'eux (energie et invulnerabilite persistent d'une salle a l'autre). */
typedef struct {
    Enemy enemy[ENEMY_MAX];
    EnemyU8 energy;          /* C415 */
    EnemyU8 invuln;          /* C42D : compte a rebours d'invulnerabilite */
    EnemyU8 invulnPhase;     /* C42D ne decroit qu'une image sur deux */
    EnemyU8 recoil;          /* images restantes de recul */
    EnemyS8 recoilDir;       /* sens du recul = sens de l'ennemi touche */
    EnemyU8 parity;          /* 9CC3 est appele une image sur deux */
    EnemyU8 type4Frames;     /* images depuis le dernier tir de type 4 */
    EnemyU8 type4Side;       /* prochain cote de type 4 : 0 droite, 1 gauche */
} EnemyState;

/* Tables ROM, salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags[D001] : octet de drapeaux ROM 85CF-85D6 ; bit0 = canal
 *   du type 1 (appel de 9CE6), bit3 = canal du type 4 (appel de 9D97).
 * kEnemySpawnPeriod[i] : ROM 9D43-9D4A, periode de la sequence (index = CF01 & 7). */
extern const EnemyU8 kEnemyStage1RoomFlags[8];
extern const EnemyU8 kEnemySpawnPeriod[8];

/* Nombre d'entrees SAT d'un ennemi de type 1 : ROM 6058 (table 6057 + 1), valeur 4. */
#define ENEMY_TYPE1_SAT_COUNT 4

/* Etat de la minuterie du canal type 1 (adresses MSX : CF10, CF00, CF01). */
typedef struct {
    EnemyU8 phase;           /* CF10 : compteur partage, decremente a chaque appel de 9CC3 */
    EnemyU8 counter;         /* CF00 : compte a rebours, decremente tous les 4 appels */
    EnemyU8 index;           /* CF01 : index dans kEnemySpawnPeriod (incremente a chaque tir) */
} EnemySpawnTimer;

/* Variables MSX lues par la minuterie, la garde de spawn et le contact. */
typedef struct {
    EnemyU8 stage;           /* D000 */
    EnemyU8 room;            /* D001 */
    EnemyU8 simonX;          /* C427 */
    EnemyU8 simonY;          /* C425 */
    EnemyU8 sideFlag;        /* C42C : 0 = spawn a droite, autre = a gauche (salle D001 != 1) */
    EnemyU8 difficulty;      /* D012 : soustrait 2*D012 a la periode (valeur 0 observee) */
    const EnemyU8 *map;      /* carte de collision de la salle (sol pour la chute) ; NULL = pas de chute */
} EnemyRoomVars;

/* Remet a zero les ennemis et les compteurs de salle. Energie, invulnerabilite et recul sont conserves. */
void Enemies_setRoom(EnemyState *s);

/* Debut de partie : energie pleine, aucune invulnerabilite ni recul, salle vide. */
void Enemies_init(EnemyState *s);

/* Etat initial de la minuterie : CF00=04, CF01=00 (ROM 9CBB, ecrit par 9CA9 appele en 6324). */
void Enemies_spawnTimerInit(EnemySpawnTimer *t);

/* Un appel de 9CC3/9CE6 (une fois toutes les 2 images).
 * Retourne 1 si un ennemi a ete cree, 0 sinon (minuterie, drapeau, garde, plein).
 * Le cote est v->sideFlag ; le type est celui du cote (A a droite, B a gauche). */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s);

/* Une image de jeu complete : recul de Simon, mouvement, spawns (type 1 une image sur deux,
 * type 4 tous les ENEMY_TYPE4_PERIOD images en salle 4), contact, invulnerabilite.
 * *pushX reçoit le deplacement de Simon de cette image (px, signe) ; 0 hors recul.
 * Retourne 1 si un contact a ete compte cette image (energie -2), 0 sinon. */
int Enemies_frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v, int *pushX);

/* Coup de fouet : tue les ennemis (types 1, A, B et 4) dont la boite touche la boite du fouet.
 * La boite du fouet est en coordonnees de jeu, inclusive (vk_whip_bounds). Les ennemis sont
 * en coordonnees d'ecran : x de jeu = x d'ecran + 8.
 * Retourne le nombre d'ennemis tues. */
int Enemies_whipHit(EnemyState *s, int left, int top, int right, int bottom);

/* Avance d'une image les ennemis actifs (mouvement, animation, sortie). */
void Enemies_update(EnemyState *s);

/* Spawn direct (utile aux tests). Retourne l'index ou -1 si plein.
 * Le type 4 part a x=E5 (droite) ou 0B (gauche) ; les types A/B a E5 ou 0A. */
int Enemies_spawn(EnemyState *s, EnemyU8 side, EnemyU8 type, EnemyU8 y, EnemyU8 extraPhase);

/* Ecrit les entrees SAT d'un ennemi actif dans out[] (4 pour A/B, 2 pour le type 4).
 * Retourne le nombre d'entrees ecrites, 0 si l'ennemi est inactif. */
int Enemies_sprites(const EnemyState *s, EnemyU8 index, EnemySprite out[ENEMY_SPRITES]);

/* 1 si l'ennemi est visible a l'ecran (x dans 0..255 : limites de sortie gerees par update). */
int Enemies_isVisible(const Enemy *e);

#endif /* VK_ENEMIES_H */
