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
 *   - Type 4 : canal 9D97 (minuterie 9CC3 partagee, CF06 = 8 a l'entree, table 9DBB, garde 9E16),
 *     trace (enemy-behavior 3.3) ; mouvement 1,5 px/image mesure.
 *   - Type 5 (panthere) : liste ROM 0x1C674, routines A85C/A87D/99B9/99E5, contact 8290
 *     et fouet 7DAD lus dans le code et suivis en trace (enemy-behavior, section Type 5).
 *   - Type 2 (homme-poisson, stage 2 salles 4 et 5) : 9D4B/9CC3, A2E0/A310, gouttes
 *     9C66/9CA3, contact et fouet de classe 1 (8378/8380) lus dans le code et suivis en
 *     trace (enemy-behavior, section Type 2).
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

#define ENEMY_MAX 7          /* ennemis simultanes : les 7 blocs C800..CB00 de la ROM (5F35, premier bloc libre) */
#define ENEMY_SPRITES 4      /* entrees SAT d'un ennemi de type A ou B (2 cellules 16x16, 2 couches chacune) */
#define ENEMY_TYPE4_SPRITES 2 /* entrees SAT d'un ennemi de type 4 (une cellule 16x16, 2 couches) */

#define ENEMY_ENERGY_MAX 0x20       /* C415 : energie de Simon au depart (research/player-health.md) */
#define ENEMY_CONTACT_DAMAGE 2      /* -2 par contact (observe) */
#define ENEMY_INVULN_START 0x5A     /* C42D : decremente une fois sur deux images, fenetre mesuree 180 images */
#define ENEMY_TYPE4_COUNT 8         /* CF06 a l'entree (9CA9, table 9CBB) : premier tir au 8e appel de
                                       9CC3 ou CF10 & 3 = 0, soit 56 a 62 images (phase de CF10) */
#define ENEMY_TYPE4_Y 0xAA          /* Y SAT du type 4 au spawn dans le hall (Simon au sol, Y C0), puis courbe kType4Y */
#define ENEMY_TYPE4_Y_FROM_SIMON 22 /* Y SAT au spawn = C425 - 22 (9D97 : E = C425 - 8 ; mesure stage 2 et hall) */
#define ENEMY_PATTERN_TYPE4 0x80    /* bit des motifs du type 4 (tuiles vk_enemy4_tiles) */
#define ENEMY_FALL_SPEED 2          /* px par image quand le sol manque (PROVISOIRE, vitesse non mesuree) */
#define ENEMY_TYPE5_DAMAGE 6        /* table 81CE : base 3, doublee (bit 4 de C701 nul) ; mesure 4 fois */
#define ENEMY_TYPE5_DELAY 1         /* creee par 61BB a la fin de 62F5 (etat 9) : premiere image de jeu de la salle
                                       (21 ou 22 images apres D001 = duree de l'etat 9 des salles 2, 3, 7) */
#define ENEMY_TYPE2_DAMAGE 4        /* table 81CE : base 2, doublee (bit 4 de C701 nul) ; mesure 8 fois */
#define ENEMY_TYPE2_COUNT 4         /* CF02 a l'entree (9CA9, table 9CBB) : premier tir au 4e appel de 9CC3
                                       ou CF10 & 3 = 0, soit 25 a 31 images de jeu (phase de CF10) ; 31 avec
                                       CF10 & 3 = 0, mesure 2 fois (51 images apres D001, etat 9 de 20 images) */
#define ENEMY_SAT_SLOTS 14          /* entrees SAT des objets (D638.., 5F5C) : une creation sans place est perdue */

/* Cote d'apparition, en repere d'ecran (observe). */
typedef enum {
    ENEMY_SIDE_RIGHT = 0,    /* apparait a droite, avance vers la gauche (dx = -1) */
    ENEMY_SIDE_LEFT = 1      /* apparait a gauche, avance vers la droite (dx = +1) */
} EnemySide;

/* Type d'ennemi : jeu de motifs et mouvement. Le type 1 a deux sens : A (droite) et B (gauche). */
typedef enum {
    ENEMY_TYPE_A = 0,        /* type 1, sens droite : motifs 40/44 (haut), 48/4C <-> 50/54 (bas) */
    ENEMY_TYPE_B = 1,        /* type 1, sens gauche : motifs 58/5C (haut), 60/64 <-> 68/6C (bas) */
    ENEMY_TYPE_4 = 2,        /* type 4 (salle 4, volant) : une cellule 16x16, 4 poses */
    ENEMY_TYPE_5 = 3,        /* type 5 (salles 2, 3, 7, panthere) : deux cellules 16x16, 8 poses */
    ENEMY_TYPE_2 = 4,        /* type 2 (stage 2 salles 4, 5, homme-poisson) : une colonne de deux cellules 16x16 */
    ENEMY_TYPE_SPLASH = 5    /* objet 0x20 : goutte d'eclaboussure (une cellule, motif 90), ni contact ni fouet */
} EnemyType;

/* Un ennemi actif. x est la coordonnee SAT (pixels, signee pour le hors-ecran).
 * Type 5 : x et y sont l'objet MSX (X comme C427, Y = bas du corps comme C425), sub la
 * fraction de Y (8.8), dx le sens de la vitesse X (+-0x0400 par mise a jour, 0 assise).
 * Type 2 et goutte : meme repere ; dx le sens de la vitesse X (+-0x0180 pour le type 2,
 * +-0x0100 pour la goutte, 0 a l'arret), extraPhase les drapeaux ENEMY_FISH_* (+18, +12,
 * +14 de l'objet, demi-pixel de X), anim le compteur +13, timer le compteur +15, age +1D. */
typedef struct {
    EnemyU8 active;
    EnemyU8 exiting;         /* limite atteinte : masque a l'image suivante (observe) */
    EnemyU8 type;
    EnemyU8 extraPhase;      /* type 1 : decalage de la pose B (observe, cause inconnue) */
    EnemyS16 x;
    EnemyU8 y;
    EnemyS8 dx;               /* -1 ou +1 */
    EnemyU8 sub;             /* types 5, 2 : fraction de Y ; type 4 : Y SAT du spawn - ENEMY_TYPE4_Y */
    EnemyU8 timer;           /* type 2 : +15, mises a jour avant le prochain choix de sens */
    EnemyU16 age;            /* images depuis le spawn ; 0 = image de spawn */
    EnemyS16 vy;             /* type 5 : vitesse Y 8.8 (+7/+8 de l'objet) */
    EnemyU8 state;           /* type 5 : 0 assise, 1 course, 2 saut ou chute */
    EnemyU8 anim;            /* type 5 : compteur d'animation +C, 1 a 12 */
    EnemyU8 pose;            /* type 5 : pose +B, 3F a 46 */
    EnemyU8 romBox;          /* type 1 (hall et stage 2) : sortie, contact et fouet de la ROM (99E5, 8290, 7DAD) */
} Enemy;

/* Une entree SAT produite pour l'affichage. */
typedef struct {
    EnemyU8 y;
    EnemyS16 x;
    EnemyU8 pattern;
    EnemyU8 bank;            /* 1 : motif du bloc ENEMY5 (vk_enemy5_tiles, vk_enemy2_tiles en salles 4-5 du stage 2), 0 sinon */
} EnemySprite;

/* Etat des ennemis et de Simon vis-a-vis d'eux (energie et invulnerabilite persistent d'une salle a l'autre). */
typedef struct {
    Enemy enemy[ENEMY_MAX];
    EnemyU8 energy;          /* C415 */
    EnemyU8 invuln;          /* C42D : compte a rebours d'invulnerabilite */
    EnemyU8 invulnPhase;     /* C42D ne decroit qu'une image sur deux */
    EnemyU8 recoil;          /* C420 = 5 : recul en cours (gameplay.c), contact coupe */
    EnemyS8 recoilDir;       /* sens du recul = sens de l'ennemi touche */
    EnemyU8 parity;          /* 9CC3 est appele une image sur deux */
    EnemyU16 type4Count;     /* CF06 : compte a rebours du canal type 4 (decremente tous les 4 appels de 9CC3) */
    EnemyU8 type4Side;       /* cote du dernier tir de type 4 apres la garde 9E16 : 0 droite, 1 gauche */
    EnemyU8 type5Timer;      /* images depuis l'entree dans la salle, jusqu'a la creation du type 5 */
    EnemyU8 type4Shots;      /* CF07 : tirs de type 4 depuis l'entree (index de la table 9DBB) */
    EnemyU8 type2Shots;      /* CF03 : tirs de type 2 depuis l'entree (index des tables 9D87 et 9D8F) */
    EnemyU16 type2Frames;    /* images depuis l'entree dans la salle (canal du type 2) */
    EnemyU16 type2Count;     /* CF02 : compte a rebours du canal type 2 (decremente tous les 4 appels de 9CC3) */
    EnemyU8 splashCount;     /* CF30 : gouttes creees (parite = sens et vitesse), garde d'une salle a l'autre */
    /* Objets ramasses (8D3E, research/castle-candles.md 9.8). Les minuteries baissent d'une
     * unite une image sur deux (75C0-75E9, routine de Simon a 30 Hz, comme C42D). */
    EnemyU8 killTimer;       /* C434 (objet 09) : un contact detruit l'ennemi, sans degat */
    EnemyU8 guardTimer;      /* C43A (objet 08) : aucun contact */
    EnemyU8 spawnPause;      /* C440 (objet 06, fouets 1A-1D) : canaux d'apparition arretes (5EB8) */
    EnemyU8 armor;           /* C441 : coups de face restants de l'armure (C701 bit 4, EnemyRoomVars.armorOn) ;
                              * 0 apres un changement de stage (63B7), le coup suivant passe a FFh (819B) */
    EnemyU8 itemPhase;       /* les minuteries ci-dessus ne baissent qu'une image sur deux */
    EnemyU8 contactScore;    /* image courante : points (centaines) des ennemis detruits par C434 (81AB) */
    EnemyU8 itemSound;       /* image courante : effet a jouer (0D destruction par C434, 17 C43A a 10h), 0 sinon */
    EnemyU8 tick;            /* C003 : +1 a chaque passe de 2 images (414D) ; court librement */
    EnemyU8 flash;           /* C43E (objet 05 ; boss : 75D4) : eclair de bordure (7604) */
    EnemyU8 border;          /* F3EB : couleur de bordure posee par 7604 (0 ou 0Eh) */
    /* Objet 0A, arret du temps (7116-718F, 75F2, 992F ; research/castle-candles.md 9.9).
     * L'objet lui-meme est C701 bit 6, tenu par candles.c (VKCandles.c701). */
    EnemyU8 timeStop;        /* C43B : baisse comme C434 (75F2), 0 = libre */
    EnemyU8 frozen;          /* D010 bit 0 : ennemis figes (992F), canaux d'apparition arretes (98E5) */
    EnemyU8 timeStopEnd;     /* image courante : C43B tombe a 0 (75FA), son FC a jouer */
    /* Clignotement de Simon fige a chaque passe, comme les tampons D400/D600 que la passe
     * suivante envoie en VRAM (trace, research/castle-candles.md 9.8). */
    EnemyU8 silhouette;      /* 790C : 0, 8 ou 0Eh, apres la baisse des minuteries de la passe */
    EnemyU8 hidden;          /* 78CC : C42D non nul apres sa baisse et bit 1 de C003 */
} EnemyState;

/* Drapeaux de salle du stage 2 (D000 = 2) : ROM 85D7-85DC (pointeur 85AA), salles 0 a 5.
 * Bit 3 (type 4) en salles 0, 1, 3 ; bit 0 (type 1) en salle 2 ; bit 1 (type 2, 9D4B) en 4, 5.
 * Types portes au stage 2 : le type 4 (memes motifs que le hall, verifie en VRAM) et le
 * type 1 de la salle 2 (zombie du hall : memes couches 40-6C et couleurs, verifie en VRAM ;
 * Y E = A0, cote selon le bit 1 de CF01, code 9CFC et trace) et le type 2 des salles 4 et 5
 * (homme-poisson, ENEMY_TYPE_2, couches du flux ROM 0x1597D, verifiees en VRAM). */
extern const EnemyU8 kEnemyStage2RoomFlags[6];

/* Tables ROM, salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags[D001] : octet de drapeaux ROM 85CF-85D6 ; bit0 = canal
 *   du type 1 (appel de 9CE6), bit3 = canal du type 4 (appel de 9D97).
 * kEnemySpawnPeriod[i] : ROM 9D43-9D4A, periode de la sequence (index = CF01 & 7). */
extern const EnemyU8 kEnemyStage1RoomFlags[8];
extern const EnemyU8 kEnemySpawnPeriod[8];

/* Nombre d'entrees SAT d'un ennemi de type 1 : ROM 6058 (table 6057 + 1), valeur 4. */
#define ENEMY_TYPE1_SAT_COUNT 4

/* Etat de la minuterie du canal type 1 (adresses MSX : CF10, CF00, CF01). CF10 est partage
 * par tous les canaux (types 1, 2 et 4) et n'est remis a 0 qu'en debut de partie (44C6). */
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
    EnemyU8 simonState;      /* C420, lu par la boite de contact 847A : ENEMY_SIMON_* */
    EnemyU8 facingLeft;      /* C42C : Simon regarde a gauche (armure de l'objet 03, 8179) */
    EnemyU8 armorOn;         /* C701 bit 4 (objet 03, tenu par candles.c) : armure portee (8179) */
} EnemyRoomVars;

/* Valeurs de C420 qui comptent pour la boite de contact 847A (banque 2). 847A ne distingue
 * que 1 et 2 (DEC A ; CP 2) : en saut, la boite part de C425 - 8 et perd 8 px ; accroupi,
 * elle perd 8 px en haut. Les autres valeurs (sol, escalier, chute depuis le sol) gardent
 * la boite debout. C420 = 5 (recul) coupe le contact avant 847A (7EF4, champ recoil). */
#define ENEMY_SIMON_GROUND 0     /* C420 = 0 : debout, marche */
#define ENEMY_SIMON_JUMP   1     /* C420 = 1 : saut (6B9B), montee et descente jusqu'au sol (6D43) */
#define ENEMY_SIMON_CROUCH 2     /* C420 = 2 : accroupi (6C20) */
#define ENEMY_SIMON_STAIRS 3     /* C420 = 3 : escalier (6BEE) */
#define ENEMY_SIMON_FALL   4     /* C420 = 4 : chute depuis le sol (6BA5) */

/* Remet a zero les ennemis et les compteurs de salle. Energie, invulnerabilite et recul sont conserves. */
void Enemies_setRoom(EnemyState *s);

/* Debut de partie : energie pleine, aucune invulnerabilite ni recul, salle vide. */
void Enemies_init(EnemyState *s);

/* Duree des minuteries d'objet : 96h, F0h si C431 bit 2 (objet 0B, 8D7C, 8DCD, 8DE5). */
#define ENEMY_ITEM_TIMER 0x96
#define ENEMY_ITEM_TIMER_LONG 0xF0
#define ENEMY_ITEM_ENERGY 8          /* objet 07 : 4605 avec B = 08 (C415, max 20h) */
#define ENEMY_ARMOR_HITS 0x10        /* objet 03 : C441 = 10h (8DA8) */
#define ENEMY_GUARD_WARNING 0x10     /* 75EA : effet 17 quand C43A tombe a 10h */
/* Effet d'un objet ramasse sur cet etat (table 8D3E) : 03, 05, 06, 07, 08, 09, 16 et
 * les fouets 1A-1D (C440). c431 = C431 (bit 2 : minuteries F0h). Les autres objets sont sans
 * effet ici. */
void Enemies_item(EnemyState *s, EnemyU8 kind, EnemyU8 c431);
#define ENEMY_FLASH_START 0x18      /* objet 05 : C43E = 18h (8DC5) */
#define ENEMY_FLASH_COLOR 0x0E       /* 7604 : F3EB = 0Eh quand le bit 1 de C43E est mis, 0 sinon */
#define ENEMY_SIMON_GUARD_COLOR 0x0E /* 7924 : Simon en couleur 0Eh avec C43A (objet 08) */
#define ENEMY_SIMON_KILL_COLOR 0x08  /* 7932 : Simon en couleur 8 avec C434 (objet 09) */
/* Couleur MSX de toutes les lignes des sprites de Simon (790C) : avec C43A puis C434 non
 * nuls et le bit 0 de C003 mis, 0Eh puis 8 (sans le bit CC 40h : silhouette d'une seule
 * couleur) ; 0 = couleurs normales (1 et 42h). Valeur figee a la derniere passe : un
 * objet ramasse (7FE2, apres 790C) ne compte qu'a la passe suivante. */
EnemyU8 Enemies_simonColor(const EnemyState *s);
/* 78CC : sprites de Simon caches (Y E0) quand C42D, apres sa baisse de la passe, est non
 * nul et le bit 1 de C003 mis ; des la passe du coup (6F9F precede 78CC). */
EnemyU8 Enemies_simonHidden(const EnemyState *s);
/* Fin d'une mort (70BF) : C434, C43A, C43B, C440, C441 et D010 a 0 (C701 &= 80h, qui
 * retire l'armure et l'objet 0A, est fait par vk_candles_death). */
void Enemies_death(EnemyState *s);

/* Objet 0A : C43B = 5Ah (96h si C431 bit 2) au declenchement (717C-7187), 5 coeurs (7170). */
#define ENEMY_TIME_STOP 0x5A
#define ENEMY_TIME_STOP_LONG 0x96
#define ENEMY_TIME_STOP_HEARTS 5
#define ENEMY_TIME_STOP_ITEM 0x40    /* C701 bit 6 : objet 0A tenu (8D94, VK_ITEM_BIT_0A) */
/* 715F-718F : avec C701 bit 6 (c701 : VKCandles.c701), C43B = 0 et au moins 5 coeurs, retire 5 coeurs (*hearts,
 * C417), C43B = 5Ah (96h si c431 bit 2) et D010 bit 0. Retourne 1 si le temps s'arrete
 * (l'appelant joue FB, 718F), 0 sinon. L'appelant verifie la touche (C006 bit 1 : BAS
 * nouvellement presse), C420 = 1 (saut, 7136) et l'absence de coup de fouet (7116). */
int Enemies_useTimeStop(EnemyState *s, EnemyU8 *hearts, EnemyU8 c431, EnemyU8 c701);

/* Debut de partie : CF10 = 0 (44C6 efface C405-E000, trace : 0 a la premiere salle du hall),
 * puis Enemies_spawnTimerRoom. */
void Enemies_spawnTimerInit(EnemySpawnTimer *t);

/* Chargement de salle (9CA9, appele en 6324) : CF00 = 04, CF01 = 00 (table 9CBB). CF10 garde sa
 * valeur d'une salle a l'autre et apres une vie perdue (trace : jamais reecrit hors de 9CC3). */
void Enemies_spawnTimerRoom(EnemySpawnTimer *t);

/* Un appel de 9CC3/9CE6 (une fois toutes les 2 images).
 * Retourne 1 si un ennemi a ete cree, 0 sinon (minuterie, drapeau, garde, plein).
 * Le cote est v->sideFlag ; le type est celui du cote (A a droite, B a gauche).
 * Stage 2 (salle 2) : cote selon le bit 1 de CF01 (9D1F), v->sideFlag ignore. */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s);

/* Une image de jeu complete : recul de Simon, mouvement, spawns (type 1 une image sur deux,
 * type 4 par le canal 9D97 (meme minuterie, CF06/CF07), type 5 une fois par visite des
 * salles 2, 3 et 7), contact, invulnerabilite.
 * *pushX reçoit le deplacement de Simon de cette image (px, signe) ; 0 hors recul.
 * Retourne 1 si un contact a ete compte cette image (energie -2, -6 pour le type 5), 0 sinon. */
int Enemies_frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v);

/* Test de contact de Enemies_frame (8290 -> 847A) pour un ennemi actif, selon la classe de son
 * type (82CB) et l'etat de Simon v->simonState (C420). Ne tient compte ni du recul ni de
 * l'invulnerabilite. Classes, en Y de l'objet MSX (pieds) :
 *   types 1, 2 (8378) : |dX| < 10 ; debout Y de C425 - 27 a C425 + 22, accroupi C425 - 19
 *     a C425 + 22, saut C425 - 27 a C425 + 14 ;
 *   type 4 (8390) : |dX| < 13 ; debout C425 - 27 a C425 + 14, accroupi C425 - 19 a
 *     C425 + 14, saut C425 - 27 a C425 + 6 ;
 *   type 5 (83D2) : |dX| < 17 ; debout C425 - 27 a C425 + 8, accroupi C425 - 19 a C425 + 8,
 *     saut C425 - 27 a C425.
 * Escalier et chute depuis le sol : boite debout. */
int Enemies_touches(const Enemy *e, const EnemyRoomVars *v);

/* Coup de fouet : tue les ennemis de type 1 sans romBox (tests) dont la boite touche la boite du fouet.
 * La boite du fouet est en coordonnees de jeu, inclusive (vk_whip_bounds). Les ennemis sont
 * en coordonnees d'ecran : x de jeu = x d'ecran + 8.
 * Retourne le nombre d'ennemis tues. Le type 5 a son propre test (Enemies_whipHitType5).
 */
int Enemies_whipHit(EnemyState *s, int left, int top, int right, int bottom);

/* Fouet contre le type 5 (7DAD, boite 84A7), a appeler pendant la phase 3 du fouet :
 * X du fouet = simonX +- 20 selon le regard, touche si |X fouet - X| < 24 et
 * simonY - B - 3 <= Y <= simonY - B + 10, B = 18 debout, 10 accroupi. Fouet de depart
 * (C416 = 0) ; chain = 1 (fouet a chaine, C416 = 1) : X du fouet +- 24, |dX| < 28 (84A7).
 * Un coup tue (points de vie 1). Retourne le nombre de pantheres tuees. */
int Enemies_whipHitType5(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched,
                         int chain);

/* Fouet contre le type 1 du stage 2 (romBox), a appeler pendant la phase 3 du fouet :
 * 7DAD, classe 1 (82CB), boite 84A7 avec HL = 1805 (8380). X du fouet = simonX +- 20,
 * touche si |X fouet - X| < 17 et simonY - B - 3 <= Y <= simonY - B + 24 (X, Y : objet
 * MSX, Y = pieds), B = 18 debout, 10 accroupi. Fouet de depart (C416 = 0) ; chain = 1
 * (fouet a chaine, C416 = 1) : X du fouet +- 24, |dX| < 21 (84A7). Points de vie 1.
 * Ces ennemis sont ignores par Enemies_whipHit. Retourne le nombre d'ennemis tues. */
int Enemies_whipHitType1Rom(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched,
                         int chain);

/* Fouet contre le type 2 (7DAD, classe 1, boite 84A7 avec HL = 1805), a appeler pendant la
 * phase 3 du fouet : X du fouet = simonX +- 20 selon le regard, touche si |X fouet - X| < 17
 * et simonY - B - 3 <= Y <= simonY - B + 24 (X, Y : objet MSX), B = 18 debout, 10 accroupi.
 * Fouet de depart (C416 = 0) ; chain = 1 (fouet a chaine, C416 = 1) : X du fouet +- 24,
 * |dX| < 21 (84A7). Un coup tue (points de vie 1, table 60E1). Les gouttes ne sont pas
 * touchables (+E = 0). Retourne le nombre d'hommes-poissons tues. */
int Enemies_whipHitType2(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched,
                         int chain);

/* Fouet contre le type 4 (7DAD, classe 2 de 82CB, 8398 : boite 84A7 avec HL = 1008), a
 * appeler pendant la phase 3 du fouet : X du fouet = simonX + 20 (regard a droite, C42C = 0)
 * ou simonX - 20 (84D9), une retenue annule le coup (84B6) ; touche si |X fouet - X| < 20
 * (L = 08 + 12) et simonY - B - 3 <= Y <= simonY - B + 16 ((C425 - B - Y + 10h) & FF < 14h),
 * X, Y : objet MSX (SAT + 8, SAT + 15), B = 18 debout, 10 accroupi. Avec le fouet
 * a chaine (chain = 1, C416 = 1) : X du fouet a +- 24 et |X fouet - X| < 24. Un coup tue.
 * Traces openMSX : stage 2 salle 0, 76 tests, 13 morts ; hall salle 4, 138 tests (debout,
 * accroupi, deux regards, cuir et chaine), 6 morts ; tous conformes. */
int Enemies_whipHitType4(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched,
                         int chain);
/* Armes lancees (801E -> 82A4 -> 84F0/8500, research/secondary-weapons.md) : pour chaque
 * ennemi actif dans l'ordre des blocs C800, puis pour l'emplacement C450 (k = 0) et C460
 * (k = 1), touch(ctx, k, X, Y, H, L) dit si le projectile k touche la boite de la classe
 * de l'ennemi (X, Y : objet MSX). Le premier contact tue l'ennemi (points de vie 1) et
 * termine la passe : *slot = k, *score = points en centaines. Retourne 1 sur un contact. */
typedef int (*ShotTouch)(const void *ctx, int slot, int x, int y, int h, int l);
int Enemies_shotHit(EnemyState *s, ShotTouch touch, const void *ctx, int *slot, int *score);

/* Avance d'une image les ennemis actifs (mouvement, animation, sortie). */
void Enemies_update(EnemyState *s);

/* Spawn direct (utile aux tests). Retourne l'index ou -1 si plein.
 * Le type 4 part a x=E5 (droite) ou 0B (gauche) ; les types A/B a E5 ou 0A. */
int Enemies_spawn(EnemyState *s, EnemyU8 side, EnemyU8 type, EnemyU8 y, EnemyU8 extraPhase);

/* Ecrit les entrees SAT d'un ennemi actif dans out[] (4 pour A/B, 5 et 2, 2 pour le type 4,
 * 1 pour la goutte).
 * Retourne le nombre d'entrees ecrites, 0 si l'ennemi est inactif. */
int Enemies_sprites(const EnemyState *s, EnemyU8 index, EnemySprite out[ENEMY_SPRITES]);

/* 1 si l'ennemi est visible a l'ecran (x dans 0..255 : limites de sortie gerees par update). */
int Enemies_isVisible(const Enemy *e);

#endif /* VK_ENEMIES_H */
