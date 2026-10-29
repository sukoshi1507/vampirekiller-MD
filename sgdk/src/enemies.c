/*
 * Logique pure des ennemis du hall (voir enemies.h et research/enemy-spawn.md, enemy-behavior.md).
 *
 * Traduction des routines Z80 :
 *   - 9CE6 (banque 2) : canal type 1, appele depuis 5EF6 si bit0 du drapeau ROM.
 *   - 9CC3 (banque 2) : minuterie partagee CF10 + compte a rebours CF00.
 *   - 9CFC (banque 2) : position de spawn (E = Y, D = X de spawn).
 *   - 9E16 (banque 2) : garde de spawn selon Simon (C427) et D.
 *   - 9D97 (banque 2) : canal type 4 (drapeau bit 3) : 9CC3 sur CF06/CF07, table 9DBB, garde 9E16.
 *   - 5F1D (banque 0) : creation de l'entite dans le premier bloc libre.
 *   - 9D4B, A2E0, A310 : type 2, l'homme-poisson des salles 4 et 5 du stage 2 ; 9C66/9CA3 :
 *     ses gouttes (objet 0x20). Voir la section « Type 2 » de enemy-behavior.md.
 *
 * Niveau de confiance :
 *   - Tables kEnemy*                                       verifie (ROM)
 *   - Minuterie type 1 (4 appels par decrement, reload = periode - 2*D012 + 1)   verifie
 *   - Garde 9E16                                           verifie code + 6 cas de trace
 *   - Cote = type (A droite, B gauche)                     enemy-behavior 6.3
 *   - Mouvement, animation, sortie du type 1               observe (trace frames.txt)
 *   - Type 4 : canal 9D97 (premier tir, cadence, cote, 9E16) code + trace (scratchpad/fidelity)
 *   - Type 4 : 1,5 px/image, sortie                        observe (4 creations)
 *   - Contact -2, invulnerabilite 180 images               observe (run 3, run 6) ;
 *     le recul lui-meme est dans gameplay.c (vk_game_hit)
 *   - Type 1 du stage 2 (salle 2) : spawn 9CFC (E = A0, cote = bit 1 de CF01), sortie 99E5,
 *     contact 8290 et fouet 7DAD de la classe 1 : code lu et trace (enemy-behavior.md,
 *     section « Type 1 au stage 2 »). Mouvement et motifs : ceux du hall (meme routine A964).
 *
 * PROVISOIRE (non verifie, a confirmer en emulation) :
 *   - Cote du type 1 : alternance par tir (index CF01 & 1). La vraie regle (C42C) n'est pas relevee.
 *   - Boite de contact du type 4 : celle de la ROM (8390) appliquee a la position affichee
 *     (objet = SAT + 8, + 15) ; la SAT suit l'objet une image sur deux.
 *   - extraPhase (1 pour A, 15 pour B) : observe, cause inconnue.
 *   - Type 2 : premier tir 51 images apres l'entree (2 mesures, depend de la phase de CF10) ;
 *     +1D, +12 et +15 de l'objet repris a 0 (la ROM garde ceux de l'occupant precedent du
 *     bloc) ; premiere mise a jour des gouttes (la ROM en fait une de plus, voir 9CA3) ;
 *     objet 1E, score et sons 7/8/9 non portes.
 *   - Salle 1 avec Simon haut (E = 60) : condition C425 < 0x88 (module), pas une regle verifiee.
 *
 * Contact (8290 -> 847A, Enemies_touches) : boite de la classe du type (82CB), selon C420
 * (EnemyRoomVars.simonState) : debout, accroupi, saut ; code lu (research/enemy-behavior.md,
 * section « Contact selon la posture de Simon »).
 */
#include "enemies.h"
#include "player.h"

#define ENEMY_SPAWN_X_RIGHT 0xE5   /* 229 : cote droit, observe (SAT X de 909) */
#define ENEMY_SPAWN_X_LEFT  0x0A   /* 10 : cote gauche, observe (SAT X de 1013) */
#define ENEMY_SPAWN_X_LEFT_T4 0x0B /* 11 : type 4 a gauche, observe */
#define ENEMY_EXIT_X_LEFT   0      /* dx < 0 : limite x <= 0 ; masque a l'image suivante (observe 1125 -> 1126) */
#define ENEMY_EXIT_X_RIGHT  216    /* dx > 0 : limite x >= 216 ; masque a l'image suivante (observe 1207 -> 1208) */
#define ENEMY_EXIT_X_LEFT_T4 1     /* type 4, sens gauche : sort a X 01 */
#define ENEMY_EXIT_X_RIGHT_T4 0xE6 /* type 4, sens droite : sort a X E6 */
#define ENEMY_ROW_OFFSET    16     /* rang bas = Y + 16 (observe B0 pour A0) */
#define ENEMY_SIMON_HALF_W  8      /* boite de Simon : X-8 a X+15 (sprite 24 px) */
#define ENEMY_SIMON_RIGHT   16
#define ENEMY_SIMON_H       31     /* boite de Simon : Y-31 a Y (pieds en Y, provisoire) */
#define ENEMY_TYPE4_ROW_H   16
#define ENEMY_TYPE1_ROW_H   32
/* Type 1 du stage 2 (salle 2), research/enemy-behavior.md section « Type 1 au stage 2 ». */
#define ENEMY_S2_TYPE1_E      0xA0 /* 9D1B : E = A0 (Y de l'objet = pieds, comme C425) */
#define ENEMY_S2_TYPE1_Y      (ENEMY_S2_TYPE1_E - 0x20) /* Y SAT du rang haut : 80 (trace : 128 a chaque tir) */
#define ENEMY_S2_EXIT_X_RIGHT 0xE7 /* 99E5 : objet supprime a X >= F1 ; dernier X SAT E7 (trace) */
#define ENEMY_OBJ_X 8              /* X de l'objet = X SAT + 8 (enregistrements +21 de l'objet) */
#define ENEMY_OBJ_Y 31             /* Y de l'objet = Y affiche du rang haut + 31 (une fois pose) */
#define ENEMY_TYPE4_OBJ_Y 15       /* type 4 : Y de l'objet = Y SAT + 15 (+3 de l'objet, prouve) */

/* Tables ROM salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags : ROM 0x85CF-0x85D6 (pointeur 85A8 = 85CF pour D000 = 1).
 *   D001 = 0 -> 01, 1 -> 01, 2 -> 00, 3 -> 00, 4 -> 08 (type 4), 5 -> 01, 6 -> 01, 7 -> 00.
 * kEnemySpawnPeriod : ROM 0x9D43-0x9D4A (banque 2). */
const EnemyU8 kEnemyStage1RoomFlags[8] = { 0x01, 0x01, 0x00, 0x00, 0x08, 0x01, 0x01, 0x00 };
const EnemyU8 kEnemyStage2RoomFlags[6] = { 0x08, 0x08, 0x01, 0x08, 0x02, 0x02 };
const EnemyU8 kEnemySpawnPeriod[8] = { 0x0C, 0x12, 0x0C, 0x0C, 0x0C, 0x12, 0x0C, 0x0C };
/* 9DBB-9DC2 (banque 2) : periodes du canal type 4, index CF07 & 7 avant l'increment. */
static const EnemyU8 kType4Period[8] = { 0x14, 0x14, 0x14, 0x28, 0x14, 0x14, 0x14, 0x28 };

#define ENEMY_FLAG_TYPE1 0x01
#define ENEMY_FLAG_TYPE2 0x02
#define ENEMY_FLAG_TYPE4 0x08

/* Motifs : [type][pose][entree] ; entrees 0,1 = rang haut ; 2,3 = rang bas. */
static const EnemyU8 kPatterns[2][2][4] = {
    { { 0x40, 0x44, 0x48, 0x4C },   /* type A, pose A */
      { 0x40, 0x44, 0x50, 0x54 } }, /* type A, pose B */
    { { 0x58, 0x5C, 0x60, 0x64 },   /* type B, pose A */
      { 0x58, 0x5C, 0x68, 0x6C } }  /* type B, pose B */
};

/* Type 4 : Y SAT par pas de 2 images depuis le spawn (77 pas, identique dans les deux
 * sens et a chaque tir, 8 episodes mesures en salle 4). Le pas 18 manque a la mesure
 * (clignotement du multiplexage) : 178, interpole. */
#define ENEMY_TYPE4_Y_STEPS 77
static const EnemyU8 kType4Y[ENEMY_TYPE4_Y_STEPS] = {
    170, 171, 172, 174, 175, 175, 176, 177, 178, 178, 179, 179, 179, 179, 179, 179,
    179, 179, 178, 178, 177, 177, 176, 175, 174, 173, 172, 171, 170, 168, 167, 166,
    164, 163, 162, 162, 161, 160, 160, 159, 159, 159, 158, 158, 158, 158, 159, 159,
    159, 160, 161, 161, 162, 163, 164, 165, 166, 168, 169, 170, 172, 173, 174, 175,
    175, 176, 177, 177, 178, 178, 178, 178, 178, 178, 178, 178, 178
};
/* Type 4 : battement d'ailes, 4 poses de 16 images (premiere couche ; la seconde est N+4).
 * Vers la gauche 48, 50, 48, 58 ; vers la droite 60, 68, 60, 70 (mesure). */
static const EnemyU8 kType4Patterns[2][4] = {
    { 0x48, 0x50, 0x48, 0x58 },
    { 0x60, 0x68, 0x60, 0x70 }
};

/* Type 5 : position (X, Y) de la panthere par salle du hall, 0 = aucune. Liste d'objets
 * en ROM fichier 0x1C674 (paires type, position YX par salle) : salle 2 en 80,80,
 * salle 3 en 60,40, salle 7 en C0,60. */
static const EnemyU8 kType5Spawn[8][2] = {
    { 0, 0 }, { 0, 0 }, { 0x80, 0x80 }, { 0x60, 0x40 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0xC0, 0x60 }
};
#define ENEMY_TYPE5_VX 4            /* 0x0400 par mise a jour (une image sur deux) */
#define ENEMY_TYPE5_WAKE 0x40       /* A891 : activation si |C427 - X| < 0x40 */
#define ENEMY_TYPE5_JUMP_VY (-0x0108)
#define ENEMY_TYPE5_GRAVITY 0x00A0
#define ENEMY_TYPE5_VY_MAX 0x07FF

/* Type 2 (homme-poisson). 9D4B : X du tir = kType2X[CF03 & 7] (CF03 deja incremente), Y C8 ;
 * 9D8F : compte a rebours suivant = kType2Period[CF03 & 7] (avant l'increment) - 2 x D012 + 1,
 * decremente toutes les 8 images (9CC3). A4F3 : mises a jour avant le choix de sens suivant,
 * index +1D & 3 (+19 = 1 a la creation). */
static const EnemyU8 kType2X[8] = { 0x60, 0xD0, 0x30, 0x90, 0xA0, 0x40, 0x60, 0xB0 };
static const EnemyU8 kType2Period[8] = { 0x01, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 };
static const EnemyU8 kType2Turn[4] = { 0x10, 0x20, 0x18, 0x30 };
#define ENEMY_TYPE2_Y 0xC8          /* 9D60 : sous l'eau */
#define ENEMY_TYPE2_NEAR 0x18       /* 9D6E : X decale de 32 px vers le centre si |C427 - X| < 24 */
#define ENEMY_TYPE2_JUMP_VY (-0x0B80) /* A2F1 : +7/+8 = F480 */
#define ENEMY_TYPE2_GRAVITY 0x0080  /* A32D, 9CA3 */
#define ENEMY_TYPE2_VX 0x0180       /* A3EA/A3F9 */
#define ENEMY_TYPE2_WATER 0xC0      /* A383 : sans sol sous Y C0, il replonge */
#define ENEMY_TYPE2_STEP 8          /* A414 : une pose de marche par 8 mises a jour */
#define ENEMY_FISH_KEEP_POSE 0x01   /* +18 : tombe d'un bord, garde sa pose */
#define ENEMY_FISH_RIGHT 0x02       /* +12 : tourne vers la droite */
#define ENEMY_FISH_STEP 0x04        /* +14 & 1 : seconde pose de marche */
#define ENEMY_FISH_XHALF 0x08       /* +4 = 80 : demi-pixel de X (vx = 1,5 px) */
#define ENEMY_SPLASH_VX 0x0100      /* 9C90 : goutte impaire (CF30) +1, paire -1 */
#define ENEMY_SPLASH_VY_ODD (-0x0500)
#define ENEMY_SPLASH_VY_EVEN (-0x0600)
#define ENEMY_VY_MAX 0x07FF         /* A549 */
#define ENEMY_SPLASH_PATTERN 0x90

/* Pose B : a partir de l'age 15, par blocs de 8 images (observe). */
static EnemyU8 poseOf(EnemyU16 age)
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
    e->sub = 0;
    e->age = 0;
    e->vy = 0;
    e->state = 0;
    e->anim = 0;
    e->pose = 0;
    e->romBox = 0;
    e->timer = 0;
}

void Enemies_setRoom(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) clearEnemy(&s->enemy[i]);
    s->parity = 0;
    s->type4Count = ENEMY_TYPE4_COUNT;   /* 9CA9 : CF06 = 08, CF07 = 00 */
    s->type4Side = ENEMY_SIDE_RIGHT;
    s->type5Timer = 0;
    s->type4Shots = 0;
    s->type2Shots = 0;               /* 9CA9 au chargement de la salle : CF02 = 04, CF03 = 00 */
    s->type2Frames = 0;
    s->type2Count = ENEMY_TYPE2_COUNT;
}

void Enemies_init(EnemyState *s)
{
    Enemies_setRoom(s);
    s->splashCount = 0;              /* CF30 : 0 au depart (premiere goutte du stage 2 a droite, trace) */
    s->energy = ENEMY_ENERGY_MAX;
    s->invuln = 0;
    s->invulnPhase = 0;
    s->recoil = 0;
    s->recoilDir = 0;
    Enemies_death(s);
    s->itemPhase = 0;
    s->contactScore = 0;
    s->itemSound = 0;
    s->tick = 0;
    s->silhouette = 0;
    s->hidden = 0;
    s->flash = 0;
    s->border = 0;
    s->timeStopEnd = 0;
}

void Enemies_death(EnemyState *s)
{
    s->killTimer = 0;                /* 70C7 */
    s->guardTimer = 0;               /* 70CA */
    s->timeStop = 0;                 /* 70CD */
    s->spawnPause = 0;               /* 70D0 */
    s->frozen = 0;                   /* 70D3 : D010 a 0, sans son FC */
    s->armor = 0;                    /* 70DC (C701 bit 4) et 70EE (C441) ; C701 &= 80h
                                      * (objet 0A compris) : vk_candles_death */
}

int Enemies_useTimeStop(EnemyState *s, EnemyU8 *hearts, EnemyU8 c431, EnemyU8 c701)
{
    if (!(c701 & ENEMY_TIME_STOP_ITEM)) return 0;       /* 713B : C701 bit 6 */
    if (s->timeStop) return 0;                          /* 715F : C43B != 0 */
    if (*hearts < ENEMY_TIME_STOP_HEARTS) return 0;     /* 7164 : C417 < 5 */
    *hearts = (EnemyU8)(*hearts - ENEMY_TIME_STOP_HEARTS);  /* 7170-7179, puis 4566 (HUD) */
    s->timeStop = (c431 & 0x04) ? ENEMY_TIME_STOP_LONG : ENEMY_TIME_STOP;   /* 717C-7187 */
    s->frozen = 1;                                      /* 718A : SET 0,(D010) */
    /* 710D tourne dans une passe de la routine de Simon, qui baisse C43B aussitot (75F2,
     * 5A puis 59 a la meme image dans la trace) : la passe des minuteries est calee sur
     * cette image (decalage d'une image au plus des autres minuteries d'objet). */
    s->itemPhase = 1;
    return 1;
}

void Enemies_item(EnemyState *s, EnemyU8 kind, EnemyU8 c431)
{
    const EnemyU8 timer = (c431 & 0x04) ? ENEMY_ITEM_TIMER_LONG : ENEMY_ITEM_TIMER;
    EnemyU8 i;
    switch (kind) {
    case 0x03:                       /* 8D98 : C701 bit 4 (bit 5 efface), C441 = 10h */
        s->armor = ENEMY_ARMOR_HITS;
        break;
    case 0x04:                       /* 8DA1 : C701 bit 4 efface (plus d'armure, candles.c), bit 5 mis ;
                                      * C441 = 10h (8DA8) ne sert qu'aux projectiles D700 (8610), non portes */
        s->armor = ENEMY_ARMOR_HITS;
        break;
    case 0x05:                       /* 8DBB -> 7806 : tous les objets C800 et D700 detruits
                                      * (9A3A, sans points) ; 8DC5 : C43E = 18h, eclair */
        for (i = 0; i < ENEMY_MAX; i++) clearEnemy(&s->enemy[i]);
        s->flash = ENEMY_FLASH_START;
        break;
    case 0x07:                       /* 8DDE : 4605 avec B = 08 */
    case 0x16:                       /* 8E0D : 4605 avec B = 20h */
        {
            const int energy = s->energy + (kind == 0x07 ? ENEMY_ITEM_ENERGY : ENEMY_ENERGY_MAX);
            s->energy = (EnemyU8)(energy > ENEMY_ENERGY_MAX ? ENEMY_ENERGY_MAX : energy);
        }
        break;
    case 0x08: s->guardTimer = timer; break;    /* 8DCD */
    case 0x09: s->killTimer = timer; break;     /* 8DE5 */
    /* 0A (8D94 : C701 |= 40h, sans minuterie) : seulement le bit de C701 (candles.c). */
    /* 10, 11 (livres a croix, 8E1D / 8E26) : seulement C702 bit 6 / 7, lu par le seul
     * marchand (9418) ; aucun effet sur les ennemis (trace openMSX, castle-candles.md 9.8). */
    case 0x06:                       /* 8D7C */
    case 0x1A: case 0x1B: case 0x1C: case 0x1D: /* 8D70 -> 8D7C */
        s->spawnPause = timer;
        break;
    default: break;
    }
}

void Enemies_spawnTimerInit(EnemySpawnTimer *t)
{
    /* 44C6 : la RAM C405-E000 est effacee en debut de partie, CF10 compris (trace :
     * CF10 = 0 jusqu'au premier appel de 9CC3, dans la salle 0 du hall). */
    t->phase = 0;
    Enemies_spawnTimerRoom(t);
}

void Enemies_spawnTimerRoom(EnemySpawnTimer *t)
{
    /* 9CA9 (table 9CBB) : CF00 = 04, CF01 = 00. CF10 n'est ecrit que par 9CC3. */
    t->counter = 4;
    t->index = 0;
}

int Enemies_isVisible(const Enemy *e)
{
    if (!e->active) return 0;
    if (e->x < 0 || e->x > 255) return 0;
    return 1;
}

int Enemies_spawn(EnemyState *s, EnemyU8 side, EnemyU8 type, EnemyU8 y, EnemyU8 extraPhase)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (!s->enemy[i].active) {
            Enemy *e = &s->enemy[i];
            e->active = 1;
            e->exiting = 0;
            e->type = type;
            e->extraPhase = extraPhase;
            e->y = y;
            e->age = 0;
            e->sub = 0;
            e->romBox = 0;
            if (side == ENEMY_SIDE_RIGHT) {
                e->x = ENEMY_SPAWN_X_RIGHT;
                e->dx = -1;
            } else {
                e->x = (type == ENEMY_TYPE_4) ? ENEMY_SPAWN_X_LEFT_T4 : ENEMY_SPAWN_X_LEFT;
                e->dx = 1;
            }
            return (int)i;
        }
    }
    return -1; /* plein : spawn perdu (comportement MSX si plein non verifie) */
}

/* Stage 2 : un appel de 9CC3 puis 9CE6, comme au hall, avec la branche D000 = 2 de 9CFC
 * (9D1B) : E = A0, D = F0, ou 10 si le bit 1 de CF01 (deja incremente par 9CD1) est mis.
 * Trace (scratchpad/s2t1, salle 2) : cotes D, G, G, D, D, G, G, cadence 104/152 images. */
static int stage2Type1Tick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    EnemyU8 period, c, dCoord;
    EnemyU8 side;
    int k;

    if (v->room >= 6 || (kEnemyStage2RoomFlags[v->room] & ENEMY_FLAG_TYPE1) == 0) return 0;
    /* 9CC3 : CF10, puis CF00 tous les 4 appels ; reload = max(periode - 2*D012, 0) + 1. */
    t->phase--;
    if ((t->phase & 0x03) != 0) return 0;
    t->counter--;
    if (t->counter != 0) return 0;
    period = kEnemySpawnPeriod[t->index & 7];
    t->index++;
    c = (EnemyU8)(v->difficulty << 1);
    t->counter = (EnemyU8)(((period >= c) ? (EnemyU8)(period - c) : 0) + 1);

    /* 9D1D-9D27 : cote selon CF01 apres l'increment. */
    dCoord = (t->index & 0x02) ? 0x10 : 0xF0;
    /* 9E16 : garde selon X de Simon (C427) et D. */
    if (v->simonX >= 0xC0 && dCoord >= 0xC0) return 0;
    if (v->simonX < 0x40 && dCoord < 0x40) return 0;

    side = (dCoord == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;
    k = Enemies_spawn(s, side, side == ENEMY_SIDE_RIGHT ? ENEMY_TYPE_A : ENEMY_TYPE_B,
                      ENEMY_S2_TYPE1_Y, side == ENEMY_SIDE_RIGHT ? 1 : 15);
    if (k < 0) return 0;
    s->enemy[k].romBox = 1;
    return 1;
}

/* Un appel de 9CC3 (minuterie) puis, si le compte a rebours expire, 9CE6 (spawn). */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    EnemyU8 idx, period, reload, c, dCoord, yCoord, slot, i;
    EnemyU8 side;
    EnemyU8 type;

    if (v->stage == 2) return stage2Type1Tick(t, v, s);

    /* Tables des autres stages non extraites : pas de tir. */
    if (v->stage != 1 || v->room > 7) return 0;
    /* 5EE7-5EF6 : 9CE6 n'est appele que si bit0 du drapeau de la salle est a 1. */
    if ((kEnemyStage1RoomFlags[v->room] & ENEMY_FLAG_TYPE1) == 0) return 0;

    /* 9CC3 : DEC CF10 ; RET NZ si (CF10 & 3) != 0. */
    t->phase--;
    if ((t->phase & 0x03) != 0) return 0;
    /* 9CC3 : DEC (canal) ; RET NZ tant que le compte a rebours n'est pas nul. */
    t->counter--;
    if (t->counter != 0) return 0;

    /* 9CD0-9CD3 : idx = CF01 puis CF01++ ; periode = table[idx & 7]. */
    idx = t->index;
    t->index++;
    period = kEnemySpawnPeriod[idx & 7];
    /* 9CD8-9CE3 : reload = max(periode - 2*D012, 0) + 1. */
    c = (EnemyU8)(v->difficulty << 1);
    reload = (period >= c) ? (EnemyU8)(period - c) : 0;
    t->counter = (EnemyU8)(reload + 1);

    /* 9CFC : E (Y) et D (X) de spawn. Branche L = D000 = 1 (stage 1). */
    yCoord = 0xC0;
    if (v->room == 1 && v->simonY < 0x88) {
        /* 9D38 -> DE = F060 : cas sans cote (D = F0 = droite). */
        yCoord = 0x60;
        dCoord = 0xF0;
    } else {
        /* 9D2C/9D2E : D = F0 ; D = 10 si C42C != 0. */
        dCoord = (v->sideFlag != 0) ? 0x10 : 0xF0;
    }

    /* 9E16 : garde selon X de Simon (C427) et D. Refus = RET C (9CF6). */
    if (v->simonX >= 0xC0) {
        if (dCoord >= 0xC0) return 0;          /* 9E29-9E2D : D >= C0 -> carry, refus */
    } else if (v->simonX >= 0x40) {
        /* 9E21 : pas de condition */
    } else {
        if (dCoord < 0x40) return 0;           /* 9E22-9E28 : D < 40 -> carry, refus */
    }

    /* 5F1D : premier bloc libre (5F42 : premier bloc a [0] nul). */
    slot = ENEMY_MAX;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (!s->enemy[i].active) { slot = i; break; }
    }
    if (slot == ENEMY_MAX) return 0;           /* plein : spawn perdu */

    /* Le cote donne le type : droite = A (motifs 40), gauche = B (motifs 58). */
    side = (dCoord == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;
    type = (side == ENEMY_SIDE_RIGHT) ? ENEMY_TYPE_A : ENEMY_TYPE_B;

    /* Y de sprite = E - 0x20 (observe pour E = C0 -> A0). */
    yCoord = (EnemyU8)(yCoord - 0x20);
    {
        /* Meme objet que le type 1 du stage 2 (routine A964) : sortie 99E5, contact 8290 et
         * fouet 7DAD de la ROM (trace du hall : suppression a X 06.40 et F1.40, Y C0). */
        const int k = Enemies_spawn(s, side, type, yCoord, (type == ENEMY_TYPE_A) ? 1 : 15);
        if (k < 0) return 0;
        s->enemy[k].romBox = 1;
        return 1;
    }
}

/* Chute : un ennemi sans sol sous lui tombe (ENEMY_FALL_SPEED px par image). Le sol se teste
 * comme celui de Simon (vk_grounded) : centre du sprite en X (sprite X + 8 en repere de jeu),
 * pieds en Y + 31 (bas du sprite de 32 px). Vitesse provisoire. */
static void fall(Enemy *e, const EnemyU8 *map, EnemyU8 stage)
{
    int step;
    for (step = 0; step < ENEMY_FALL_SPEED; step++) {
        if (vk_grounded(map, (VKByte)(e->x + 8), (VKByte)(e->y + 31), stage, 0)) break;
        e->y++;
    }
}

/* Type 5 : sol au point (X, Y) ou (X - 10, Y) (7B98 = 7D2F + 7C5E). Sans carte, sol partout. */
static int pantherGround(const Enemy *e, const EnemyU8 *map, EnemyU8 stage)
{
    if (!map) return 1;
    return vk_tile_solid(vk_sample_tile(map, (VKByte)e->x, e->y), stage, 0) ||
           vk_tile_solid(vk_sample_tile(map, (VKByte)(e->x - 10), e->y), stage, 0);
}

/* Type 5 : vitesse X vers Simon (C427 >= X : droite). */
static void pantherToward(Enemy *e, EnemyU8 simonX)
{
    e->dx = (EnemyS8)(simonX >= e->x ? 1 : -1);
}

/* Type 5 : une mise a jour (une image sur deux), dans l'ordre de la boucle 98EC :
 * routine A87D, deplacement 99B9 (Y += vy, X += vx en 8.8), bornes 99E5. */
static void pantherStep(Enemy *e, const EnemyRoomVars *v)
{
    int y;
    if (e->x < 0x18) {         /* sortie a gauche, avant la routine */
        e->active = 0;
        return;
    }
    if (e->state == 0) {       /* A891 : assise, attend Simon a moins de 64 px (sans condition en Y) */
        const int d = (int)v->simonX - e->x;
        if ((d < 0 ? -d : d) < ENEMY_TYPE5_WAKE) {
            pantherToward(e, v->simonX);
            e->state = 1;
        }
    } else if (e->state == 1) { /* A8AC : course, animation A909, saut au bord du sol */
        const EnemyU8 base = e->dx < 0 ? 0x41 : 0x45;
        if (++e->anim > 12) e->anim = 1;
        e->pose = (EnemyU8)(e->anim <= 3 ? base : e->anim <= 9 ? base + 1 : base - 1);
        if (!pantherGround(e, v->map, v->stage)) {
            e->vy = ENEMY_TYPE5_JUMP_VY;
            e->pose = e->dx < 0 ? 0x42 : 0x46;
            e->state = 2;
        }
    } else {                   /* A8D2 : saut ou chute, pose figee */
        e->vy = (EnemyS16)(e->vy + ENEMY_TYPE5_GRAVITY);
        if (e->vy > ENEMY_TYPE5_VY_MAX) e->vy = ENEMY_TYPE5_VY_MAX;
        if (pantherGround(e, v->map, v->stage)) {
            pantherToward(e, v->simonX);
            e->anim = 0;
            e->state = 1;
            e->y &= 0xF8;
            e->sub = 0;
            e->vy = 0;
        }
    }
    y = (int)((e->y << 8) | e->sub) + e->vy;
    e->y = (EnemyU8)(y >> 8);
    e->sub = (EnemyU8)y;
    e->x += (EnemyS16)(ENEMY_TYPE5_VX * e->dx);
    if (e->y >= 0xE4 || e->x >= 0xF1 || e->x < 0x07) e->active = 0;
}

/* Type 5 : creation (61BB, 5F1F, A85C). Pose assise tournee vers Simon, etat 0. */
static void pantherSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y, EnemyU8 simonX)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        if (e->active) continue;
        clearEnemy(e);
        e->active = 1;
        e->type = ENEMY_TYPE_5;
        e->x = x;
        e->y = y;
        e->pose = simonX >= x ? 0x43 : 0x3F;
        return;
    }
}

/* Octet de drapeaux de la salle (85A6 + 2 x D000), 0 hors des stages 1 et 2. */
static EnemyU8 roomFlags(const EnemyRoomVars *v)
{
    if (v->stage == 1 && v->room < 8) return kEnemyStage1RoomFlags[v->room];
    if (v->stage == 2 && v->room < 6) return kEnemyStage2RoomFlags[v->room];
    return 0;
}

/* Une image pour un ennemi deja actif. */
static void advance(Enemy *e, EnemyU8 frozen)
{
    if (e->type == ENEMY_TYPE_5 || e->type == ENEMY_TYPE_2 || e->type == ENEMY_TYPE_SPLASH)
        return;                            /* mis a jour par Enemies_frame (carte, Simon) */
    if (e->exiting) {          /* masque a l'image suivant la limite (observe 1126, 1208) */
        e->active = 0;
        return;
    }
    /* 98F9 -> 992F : D010 != 0 et +7E = 1 (objet cree par 5F35) : ni comportement (993B,
     * pose comprise) ni vitesse (99B9) ; l'objet reste dessine a sa place. */
    if (frozen) return;
    e->age++;
    if (e->type == ENEMY_TYPE_4) {
        /* La SAT suit l'objet une image sur deux : 3 px en X et une marche de la
         * courbe Y a chaque age pair (mesure image par image, scratchpad/type4/gfx). */
        if ((e->age & 1) == 0) e->x += (EnemyS16)(3 * e->dx);
        /* La courbe est relative a la Y du spawn (sub = Y du spawn - ENEMY_TYPE4_Y). */
        e->y = (EnemyU8)(kType4Y[(e->age / 2) < ENEMY_TYPE4_Y_STEPS ? e->age / 2 : ENEMY_TYPE4_Y_STEPS - 1] + e->sub);
        /* Derniere position tenue deux images : X 01 (gauche, ages 152-153), E6 (droite, 146-147). */
        if ((e->age & 1) && e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT_T4) e->exiting = 1;
        if ((e->age & 1) && e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT_T4) e->exiting = 1;
        return;
    }
    /* Type 1 : pas de base de 2 px toutes les 2 images, a partir de l'age 3, et 1 px
     * supplementaire tous les 16 images (phase extraPhase). Somme : 229 px en ~218 images. */
    if (e->age >= 3 && (e->age & 1)) {
        e->x += 2 * e->dx;
    }
    if (e->age >= 3 && ((e->age - e->extraPhase) % 16) == 0) {
        e->x += e->dx;
    }
    if (e->romBox) {
        /* 99E5 (X < 07 ou X >= F1) : la derniere position, X SAT 00 ou E7, reste deux
         * images (ages 217-218 et 209-210 en salle 2 du stage 2), puis l'objet disparait. */
        if ((e->age & 1) == 0 && ((e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) ||
                                  (e->dx > 0 && e->x >= ENEMY_S2_EXIT_X_RIGHT))) e->exiting = 1;
        return;
    }
    if (e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) e->exiting = 1;
    if (e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT) e->exiting = 1;
}

void Enemies_update(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (s->enemy[i].active) advance(&s->enemy[i], s->frozen);
    }
}

/* Boite de contact 847A (banque 2), appelee par 8290 avec les parametres de la classe de
 * l'ennemi (table 82CB, puis 8378 / 8390 / 83D2 : HL = H L, D = X, E = Y de l'objet) :
 *   |C427 - X| < L + 5 et (C425' - 2 - Y + H) & FF < H + 1A,
 * ou H + 12 si C420 vaut 1 (saut) ou 2 (accroupi), et C425' = C425 - 8 si C420 = 1.
 * Debout, l'ennemi touche de Y = C425 + H - 2 - (H + 19) a C425 + H - 2 ; accroupi, le haut
 * descend de 8 px ; en saut, le bas monte de 8 px. Escalier (3) et chute (4) : boite debout. */
static int romContact(const EnemyRoomVars *v, int objX, int objY, int l, int h)
{
    const int d = (int)v->simonX - objX;
    const int crouchOrJump = v->simonState == ENEMY_SIMON_JUMP || v->simonState == ENEMY_SIMON_CROUCH;
    const int tall = h + (crouchOrJump ? 0x12 : 0x1A);
    const int y = v->simonState == ENEMY_SIMON_JUMP ? (int)v->simonY - 8 : (int)v->simonY;
    l += 5;
    return d > -l && d < l && (EnemyU8)(y - 2 - objY + h) < tall;
}

/* Classes 82CB : types 1, 2, 3 -> 8378 (L = 05, H = 18) ; type 4 -> 8390 (L = 08, H = 10) ;
 * type 5 -> 83D2 (L = 0C, H = 0A). X, Y de l'objet MSX : romBox et type 4 sont stockes en
 * coordonnees de sprite (objet = SAT + 8 en X ; + 31 en Y pour le rang haut du type 1,
 * + 15 pour le type 4), les types 2 et 5 en coordonnees d'objet. */
int Enemies_touches(const Enemy *e, const EnemyRoomVars *v)
{
    if (e->type == ENEMY_TYPE_5) return romContact(v, e->x, e->y, 0x0C, 0x0A);
    if (e->type == ENEMY_TYPE_2) return romContact(v, e->x, e->y, 0x05, 0x18);
    if (e->type == ENEMY_TYPE_4)
        return romContact(v, e->x + ENEMY_OBJ_X, e->y + ENEMY_TYPE4_OBJ_Y, 0x08, 0x10);
    if (e->romBox) return romContact(v, e->x + ENEMY_OBJ_X, e->y + ENEMY_OBJ_Y, 0x05, 0x18);
    /* Type 1 sans romBox (tests seulement : tous les tirs du port sont romBox) : boites
     * des sprites, Simon 24 x 32 en (X-8, Y-31), ennemi [x, x+16) x [y, y+32). */
    return e->x < v->simonX + ENEMY_SIMON_RIGHT &&
           e->x + 16 > v->simonX - ENEMY_SIMON_HALF_W &&
           e->y < v->simonY + 1 &&
           e->y + ENEMY_TYPE1_ROW_H > v->simonY - ENEMY_SIMON_H;
}

#define ENEMY_SCREEN_TO_GAME_X 8   /* Simon et les ennemis : sprite X = x de jeu - 8 */

int Enemies_whipHit(EnemyState *s, int left, int top, int right, int bottom)
{
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        int ex, ey, eh;
        if (!e->active || e->exiting || e->type == ENEMY_TYPE_5 || e->romBox ||
            e->type == ENEMY_TYPE_2 || e->type == ENEMY_TYPE_SPLASH) continue;
        ex = e->x + ENEMY_SCREEN_TO_GAME_X;
        ey = e->y;
        eh = (e->type == ENEMY_TYPE_4) ? ENEMY_TYPE4_ROW_H : ENEMY_TYPE1_ROW_H;
        if (right < ex || left >= ex + 16 || bottom < ey || top >= ey + eh) continue;
        /* Un coup suffit pour les types 1 (zombies, observe par le joueur) et 4 (trace :
         * 12 morts sur la premiere image de fouet utile, scratchpad/type4/whip-kill-type4.md).
         * Boite du type 4 : 16 x 16 provisoire, bornes a +-1-2 px non fixees. */
        e->active = 0;
        killed++;
    }
    return killed;
}

int Enemies_whipHitType5(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched)
{
    const int whipX = facingLeft ? simonX - 20 : simonX + 20;
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        const int d = whipX - e->x;
        if (!e->active || e->type != ENEMY_TYPE_5) continue;
        if (d <= -24 || d >= 24 || e->y < simonY - b - 3 || e->y > simonY - b + 10) continue;
        e->active = 0;         /* points de vie 1 : mort au premier coup (objet 1E non porte) */
        killed++;
    }
    return killed;
}

int Enemies_whipHitType1Rom(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched)
{
    /* 84D9 : X du fouet sur 8 bits ; une retenue (bord d'ecran) annule le test (84B6). */
    const int whipX = facingLeft ? simonX - 20 : simonX + 20;
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    if (whipX < 0 || whipX > 255) return 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        int d, h;
        if (!e->active || e->exiting || !e->romBox) continue;
        d = whipX - (e->x + ENEMY_OBJ_X);
        h = simonY - b - (e->y + ENEMY_OBJ_Y) + 24;
        if (d <= -17 || d >= 17 || h < 0 || h >= 28) continue;
        /* Points de vie 1 (+D = 01) : mort au premier coup (trace : 6 morts en phase 3,
         * |X fouet - X| de 0 a 15). L'objet 1E et le drop ne sont pas portes. */
        e->active = 0;
        killed++;
    }
    return killed;
}

int Enemies_whipHitType2(EnemyState *s, int simonX, int simonY, int facingLeft, int crouched)
{
    /* 84D9 : X du fouet sur 8 bits ; une retenue (bord d'ecran) annule le test (84B6). */
    const int whipX = facingLeft ? simonX - 20 : simonX + 20;
    const int b = crouched ? 10 : 18;
    EnemyU8 i;
    int killed = 0;
    if (whipX < 0 || whipX > 255) return 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        const int d = whipX - e->x;
        const int h = simonY - b - e->y + 24;
        if (!e->active || e->type != ENEMY_TYPE_2) continue;
        if (d <= -17 || d >= 17 || h < 0 || h >= 28) continue;
        e->active = 0;         /* points de vie 1 : mort au premier coup (objet 1E non porte) */
        killed++;
    }
    return killed;
}

/* Entrees SAT d'un objet (table 6057 + type) : 4 pour les types 1, 2 et 5, 2 pour le type 4 et la goutte. */
static EnemyU8 satCount(EnemyU8 type)
{
    return (EnemyU8)((type == ENEMY_TYPE_4 || type == ENEMY_TYPE_SPLASH) ? 2 : 4);
}

/* 5F1D : premier bloc libre, puis assez d'entrees SAT libres parmi les 14 ; sinon rien. */
static Enemy *newObject(EnemyState *s, EnemyU8 type)
{
    EnemyU8 i, used = 0;
    Enemy *slot = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (s->enemy[i].active) used = (EnemyU8)(used + satCount(s->enemy[i].type));
        else if (!slot) slot = &s->enemy[i];
    }
    if (!slot || used + satCount(type) > ENEMY_SAT_SLOTS) return 0;
    clearEnemy(slot);
    slot->active = 1;
    slot->type = type;
    return slot;
}

/* Goutte (9C66) : CF30 + 1 ; impaire, vers la droite a -5 px, paire, vers la gauche a -6 px.
 * age 0 : pas de mise a jour dans l'image de sa creation. */
static void splashSpawn(EnemyState *s, EnemyU8 x, EnemyU8 y)
{
    Enemy *e = newObject(s, ENEMY_TYPE_SPLASH);
    if (!e) return;
    e->x = x;
    e->y = y;
    s->splashCount++;
    e->dx = (EnemyS8)((s->splashCount & 1) ? 1 : -1);
    e->vy = (s->splashCount & 1) ? ENEMY_SPLASH_VY_ODD : ENEMY_SPLASH_VY_EVEN;
}

/* 99B9 puis 99E5 : Y += vy, X += vx en 8.8 (X n'a que des demi-pixels), suppression si
 * Y >= E4, X >= F1 ou X < 07. */
static void moveObject(Enemy *e, int vx)
{
    const int y = (int)((e->y << 8) | e->sub) + e->vy;
    const int x = (int)e->x * 256 + ((e->extraPhase & ENEMY_FISH_XHALF) ? 0x80 : 0) + vx;
    e->y = (EnemyU8)(y >> 8);
    e->sub = (EnemyU8)y;
    e->x = (EnemyS16)(x >> 8);
    if (x & 0x80) e->extraPhase |= ENEMY_FISH_XHALF;
    else e->extraPhase &= (EnemyU8)~ENEMY_FISH_XHALF;
    if (e->y >= 0xE4 || e->x >= 0xF1 || e->x < 0x07) e->active = 0;
}

/* A549 : vy += 0x80, plafonnee a 0x07FF quand l'octet fort atteint 8. */
static void fallSpeed(Enemy *e)
{
    e->vy = (EnemyS16)(e->vy + ENEMY_TYPE2_GRAVITY);
    if (e->vy > ENEMY_VY_MAX) e->vy = ENEMY_VY_MAX;
}

/* Type 2 : sol au point (X + dx, Y) ou (X + dx - 10, Y) (7B98). Sans carte, sol partout. */
static int fishGround(const Enemy *e, const EnemyRoomVars *v, int dx)
{
    if (!v->map) return 1;
    return vk_tile_solid(vk_sample_tile(v->map, (VKByte)(e->x + dx), e->y), v->stage, 0) ||
           vk_tile_solid(vk_sample_tile(v->map, (VKByte)(e->x + dx - 10), e->y), v->stage, 0);
}

/* A4B3 : pose tournee vers Simon, 0B (droite) si C427 >= X, sinon 08. */
static void fishFace(Enemy *e, EnemyU8 simonX)
{
    e->pose = (EnemyU8)(simonX >= e->x ? 0x0B : 0x08);
}

/* Type 2 : une mise a jour (une image sur deux), routine A310 puis 99B9 et 99E5. */
static void fishStep(EnemyState *s, Enemy *e, const EnemyRoomVars *v)
{
    e->age++;                                  /* +1D */
    if (e->state == 0) {                       /* A31E : saut hors de l'eau ou chute */
        if (!(e->extraPhase & ENEMY_FISH_KEEP_POSE)) fishFace(e, v->simonX);
        e->dx = 0;
        fallSpeed(e);
        if (e->vy >= 0) {                      /* A333 : pas de test pendant la montee */
            if (fishGround(e, v, (e->extraPhase & ENEMY_FISH_RIGHT) ? 8 : 0)) {
                e->y &= 0xF8;                  /* A352 : la fraction de Y est gardee */
                e->timer = 1;
                e->anim = ENEMY_TYPE2_STEP;
                e->extraPhase &= (EnemyU8)~ENEMY_FISH_STEP;
                e->state = 1;
                e->vy = 0;
            } else if (e->y >= ENEMY_TYPE2_WATER) {
                const EnemyU8 x = (EnemyU8)e->x, y = e->y;
                e->active = 0;                 /* A386 : replonge, deux gouttes */
                splashSpawn(s, x, y);
                splashSpawn(s, x, y);
                return;
            }
        }
    } else {                                   /* A3DE : marche */
        if (--e->timer == 0) {                 /* A4CF : sens vers Simon */
            e->timer = kType2Turn[e->age & 3];
            if (v->simonX >= e->x) {
                e->extraPhase |= ENEMY_FISH_RIGHT;
                e->dx = 1;
            } else {
                e->extraPhase &= (EnemyU8)~ENEMY_FISH_RIGHT;
                e->dx = -1;
            }
            e->anim = 1;
        }
        if (--e->anim == 0) {                  /* A414 : pose suivante (tables A452/A456) */
            e->anim = ENEMY_TYPE2_STEP;
            e->extraPhase ^= ENEMY_FISH_STEP;
            e->pose = (EnemyU8)(((e->extraPhase & ENEMY_FISH_RIGHT) ? 0x0B : 0x08) +
                                ((e->extraPhase & ENEMY_FISH_STEP) ? 1 : 0));
        }
        if (!fishGround(e, v, e->dx > 0 ? 8 : 0)) {   /* A43E : bord du sol, chute */
            e->vy = 0;
            e->state = 0;
            e->extraPhase |= ENEMY_FISH_KEEP_POSE;
        }
    }
    moveObject(e, ENEMY_TYPE2_VX * e->dx);
}

/* Type 2 : creation (5F1D, A2E0) en Y C8, pose vers Simon, vy -0x0B80, deux gouttes. */
static void fishSpawn(EnemyState *s, EnemyU8 x, const EnemyRoomVars *v)
{
    Enemy *e = newObject(s, ENEMY_TYPE_2);
    if (!e) return;
    e->x = x;
    e->y = ENEMY_TYPE2_Y;
    fishFace(e, v->simonX);
    e->vy = ENEMY_TYPE2_JUMP_VY;
    splashSpawn(s, x, ENEMY_TYPE2_Y);
    splashSpawn(s, x, ENEMY_TYPE2_Y);
}

/* Canal 9D4B (bit 1 du drapeau), un appel par passe de l'etat 5 (une image sur deux, a partir
 * de la premiere) : 9CC3 sur CF02/CF03 comme le canal 9D97 (DEC CF10 partage, puis CF02 tous
 * les 4 appels ; recharge table 9D8F + 1). X de la table 9D87 a CF03 apres l'increment, garde
 * 9D6E. Un tir sans place est perdu, l'index avance. */
static void type2Channel(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 period, c, x;
    if (!(++s->type2Frames & 1)) return;
    if (s->spawnPause) return;                      /* 5EB8 : C440 != 0, boucle 5ED8 sautee */
    if (s->frozen) return;                          /* 98E5 : D010 != 0, 5EB8 n'est pas appele */
    t->phase--;                                     /* 9CC3 : DEC (CF10) */
    if ((t->phase & 0x03) != 0) return;
    if (--s->type2Count != 0) return;               /* DEC (CF02) */
    period = kType2Period[s->type2Shots & 7];       /* 9CD0 : CF03 avant l'increment */
    s->type2Shots++;
    c = (EnemyU8)(v->difficulty << 1);
    s->type2Count = (EnemyU16)((period >= c ? period - c : 0) + 1);
    x = kType2X[s->type2Shots & 7];
    if ((EnemyU8)(v->simonX - x + ENEMY_TYPE2_NEAR) < 2 * ENEMY_TYPE2_NEAR)
        x = (EnemyU8)(x < 0x80 ? x + 0x20 : x - 0x20);
    fishSpawn(s, x, v);
}

/* Types 2 et gouttes : tir puis mises a jour, une image sur deux (valeurs impaires de
 * type2Frames, celles des tirs : le type 2 bouge des l'image de sa creation, la goutte a
 * partir de la mise a jour suivante). */
static void type2Frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 i;
    if (!(roomFlags(v) & ENEMY_FLAG_TYPE2)) return;
    type2Channel(s, t, v);
    if (s->type2Frames & 1) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            if (!e->active) continue;
            /* Type 2 fige par D010 ; la goutte (+7E = 0, 9C84) tombe toujours. */
            if (e->type == ENEMY_TYPE_2) { if (!s->frozen) fishStep(s, e, v); }
            else if (e->type == ENEMY_TYPE_SPLASH && e->age) {   /* 9CA3 : gravite seule */
                fallSpeed(e);
                moveObject(e, ENEMY_SPLASH_VX * e->dx);
            }
        }
    }
    for (i = 0; i < ENEMY_MAX; i++)
        if (s->enemy[i].active && s->enemy[i].type == ENEMY_TYPE_SPLASH) s->enemy[i].age = 1;
}

/* Canal 9D97 (bit 3 du drapeau), un appel par passe de l'etat 5 (une image sur deux) :
 * 9CC3 sur CF06/CF07 (CF06 = 8 a l'entree, recharge table 9DBB + 1), cote D = F0 si le bit 0
 * de CF07 (deja incremente) vaut 1, sinon 10 ; 9E16 deplace D de l'autre cote si Simon est a
 * moins de 64 px de ce bord (pas de refus : 9D97 ignore la retenue) ; E = C425 - 8 (9DB2). */
static void type4Channel(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyU8 period, c, d;
    int k;
    t->phase--;                                     /* 9CC3 : DEC (CF10) */
    if ((t->phase & 0x03) != 0) return;
    if (--s->type4Count != 0) return;               /* DEC (CF06) */
    period = kType4Period[s->type4Shots & 7];       /* 9CD0 : CF07 avant l'increment */
    s->type4Shots++;
    c = (EnemyU8)(v->difficulty << 1);
    s->type4Count = (EnemyU16)(((period >= c) ? period - c : 0) + 1);
    d = (s->type4Shots & 1) ? 0xF0 : 0x10;          /* 9DA5-9DAD */
    if (v->simonX >= 0xC0) { if (d >= 0xC0) d = 0x10; }   /* 9E29-9E2E */
    else if (v->simonX < 0x40) { if (d < 0x40) d = 0xF0; } /* 9E22-9E26 */
    s->type4Side = (d == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;
    k = Enemies_spawn(s, s->type4Side, ENEMY_TYPE_4, (EnemyU8)(v->simonY - ENEMY_TYPE4_Y_FROM_SIMON), 0);
    if (k >= 0) s->enemy[k].sub = (EnemyU8)(s->enemy[k].y - ENEMY_TYPE4_Y);
}

int Enemies_frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v)
{
    EnemyRoomVars w = *v;
    EnemyU8 i;
    int hit = 0;

    /* 75F2-7601 (6B2D) : C43B baisse a chaque passe de la routine de Simon (les images
     * ou itemPhase repasse a 0 plus bas), avant la boucle des objets de la meme passe
     * (trace : les ennemis repartent dans la passe ou D010 s'efface). A 0, D010 bit 0
     * efface et son FC. */
    s->timeStopEnd = 0;
    if (s->timeStop && s->itemPhase && --s->timeStop == 0) {
        s->frozen = 0;
        s->timeStopEnd = 1;
    }

    Enemies_update(s);
    if (v->map) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            /* Le type 4 vole : sa hauteur suit kType4Y, sans chute. */
            if (e->active && !e->exiting && !s->frozen && e->type != ENEMY_TYPE_4 &&
                e->type != ENEMY_TYPE_5 && e->type != ENEMY_TYPE_2 && e->type != ENEMY_TYPE_SPLASH)
                fall(e, v->map, v->stage);
        }
    }
    /* Type 5 : une mise a jour une image sur deux, a partir de l'image qui suit la creation. */
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        if (!e->active || e->type != ENEMY_TYPE_5) continue;
        /* D010 : la panthere (creee par 61BB -> 5F35, +7E = 1) est figee ; la phase de ses
         * mises a jour (passe de 30 Hz) continue. */
        if ((++e->age & 1) && !s->frozen) pantherStep(e, v);
    }

    /* Une passe de l'etat 5 sur deux images, a partir de la premiere image de jeu de la
     * salle : canal type 1 (9CE6, cote alterne par tir, provisoire) puis canal type 4 (9D97,
     * hall salle 4, stage 2 salles 0, 1, 3), dans l'ordre des bits de la boucle 5ED8. */
    /* 98E5 : D010 != 0 saute tout l'appel de 5EB8 (CF10 compris). */
    if (s->parity == 0 && !s->spawnPause && !s->frozen) {   /* 5EB8 : rien tant que C440 != 0 */
        w.sideFlag = (EnemyU8)(t->index & 1);
        (void)Enemies_spawnTimerTick(t, &w, s);
        if (roomFlags(v) & ENEMY_FLAG_TYPE4) type4Channel(s, t, v);
    }
    s->parity ^= 1;

    /* Type 5 : une panthere par visite des salles 2, 3 et 7, a la premiere image de jeu apres
     * l'etat 9 (62F5 recopie la liste a chaque chargement de salle et la cree en 61BB). */
    if (v->stage == 1 && v->room < 8 && kType5Spawn[v->room][0] && s->type5Timer < ENEMY_TYPE5_DELAY) {
        if (++s->type5Timer == ENEMY_TYPE5_DELAY)
            pantherSpawn(s, kType5Spawn[v->room][0], kType5Spawn[v->room][1], v->simonX);
    }

    /* Type 2 (stage 2, salles 4 et 5) et ses gouttes. */
    type2Frame(s, t, v);

    /* Contact (7E67) : desactive pendant le recul, l'invulnerabilite et C43A (objet 08) ;
     * avec C434 (objet 09) ces gardes sont sautees et le contact detruit l'ennemi.
     * D010 (objet 0A) ne coupe ni le contact ni le fouet (seuls 98F9 et 9E3E le lisent). */
    s->contactScore = 0;
    s->itemSound = 0;
    if (s->killTimer || (s->recoil == 0 && s->invuln == 0 && s->guardTimer == 0)) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            if (!e->active || e->exiting) continue;
            if (e->type == ENEMY_TYPE_SPLASH) continue;   /* goutte : +E = 0, ni contact ni fouet */
            EnemyU8 damage = e->type == ENEMY_TYPE_5 ? ENEMY_TYPE5_DAMAGE :
                             e->type == ENEMY_TYPE_2 ? ENEMY_TYPE2_DAMAGE : ENEMY_CONTACT_DAMAGE;
            if (!Enemies_touches(e, v)) continue;
            if (s->killTimer) {
                /* 7EB0 : 81AB (points de la table 81CE, comme au fouet), 9A3E, effet 0D ;
                 * un seul ennemi par passe (JP 509F). */
                s->contactScore = (EnemyU8)(e->type == ENEMY_TYPE_2 ? 2 : 1);
                s->itemSound = 0x0D;
                clearEnemy(e);
                break;
            }
            /* 8179 : avec C701 bit 4 (objet 03) et Simon tourne vers l'ennemi
             * (C427 - X et C42C), le degat n'est pas double et C441 baisse. X de
             * l'ennemi : centre de sa boite (PROVISOIRE pour les types 1 et 4). Seul le
             * bit 4 compte : C441 a 0 (stage change, 63B7) passe a FFh au coup suivant
             * (819B DEC, 819C JR NZ), l'armure tient alors 256 coups (trace, castle-candles.md 9.8). */
            if (v->armorOn) {
                const int ex = (e->type == ENEMY_TYPE_5 || e->type == ENEMY_TYPE_2) ? e->x :
                               e->romBox ? e->x + ENEMY_OBJ_X : e->x + 8;
                if ((v->simonX < ex) != (v->facingLeft != 0)) {
                    damage = (EnemyU8)(damage / 2);
                    s->armor--;              /* 819B ; a 0, 819E efface C701 bit 4 (actors.c) */
                }
            }
            s->energy = (s->energy > damage) ? (EnemyU8)(s->energy - damage) : 0;
            s->invuln = ENEMY_INVULN_START;
            s->invulnPhase = 0;
            /* 78CC suit 6F9F dans la passe du coup : hors image de passe, Simon est cache
             * des maintenant si le bit 1 de C003 est mis (sinon voir plus bas). */
            if (s->itemPhase == 0) s->hidden = (EnemyU8)(s->tick & 2);
            s->recoil = 1;    /* C420 := 5 : l'appelant l'efface a la fin du recul */
            /* Panthere assise (dx 0) : recul a l'oppose d'elle (PROVISOIRE, non mesure). */
            s->recoilDir = e->dx ? e->dx : (EnemyS8)(v->simonX >= e->x ? 1 : -1);
            hit = 1;
            break;
        }
    }

    /* 75C0-75E9 : C440, C434 et C43A baissent a chaque passe de la routine de Simon,
     * comme C42D (une image sur deux). C43A tombe a 10h : effet 17. */
    s->itemPhase ^= 1;
    if (s->itemPhase == 0) {
        s->tick++;                   /* C003 (414D), au debut de la passe */
        /* 7604 : tant que C43E != 0, F3EB = 0Eh si le bit 1 de C43E (avant la baisse) est
         * mis, 0 sinon, puis CHGCLR : 24 passes, 4 images de chaque couleur, fin a 0.
         * Second declencheur, non porte : 75D4 (6B30) met C43E = 18h quand C445 tombe a 0,
         * 20 passes apres la defaite d'un boss (7E17 : jauge ENEMY C418 a 0) ; aucun boss
         * dans les stages 0-2 (7777[D000] = 0). */
        if (s->flash) {
            s->border = (s->flash & 2) ? ENEMY_FLASH_COLOR : 0;
            s->flash--;
        }
        if (s->spawnPause) s->spawnPause--;
        if (s->killTimer) s->killTimer--;
        if (s->guardTimer && --s->guardTimer == ENEMY_GUARD_WARNING) s->itemSound = 0x17;
        /* 790C et 78CC remplissent D400/D600 apres ces baisses ; la passe suivante les
         * envoie en VRAM (trace openMSX, research/castle-candles.md 9.8). L'objet ramasse
         * plus loin (step_candles, comme 7FE2) attend donc la passe suivante. */
        s->silhouette = !(s->tick & 1) ? 0 : s->guardTimer ? ENEMY_SIMON_GUARD_COLOR :
                        s->killTimer ? ENEMY_SIMON_KILL_COLOR : 0;
        /* C42D du MSX baisse dans la passe du coup ; ici invuln baisse a l'image suivante :
         * C42D apres la baisse de la passe = invuln - 1 - invulnPhase. */
        s->hidden = (EnemyU8)((s->tick & 2) && s->invuln > 1 + s->invulnPhase);
    }

    /* C42D ne decroit qu'une image sur deux : 0x5A x 2 = 180 images. */
    if (s->invuln) {
        s->invulnPhase ^= 1;
        if (s->invulnPhase == 0) s->invuln--;
    }
    return hit;
}

EnemyU8 Enemies_simonColor(const EnemyState *s)
{
    /* 7924 / 7932 figes a la passe (RRA de C003) ; une fin de mort (70BF) les coupe. */
    return (s->guardTimer || s->killTimer) ? s->silhouette : 0;
}

EnemyU8 Enemies_simonHidden(const EnemyState *s)
{
    /* 78D2 : bit 1 de C003 ; une mort (7064) remet C42D a 0. */
    return (EnemyU8)(s->invuln && s->hidden);
}

int Enemies_sprites(const EnemyState *s, EnemyU8 index, EnemySprite out[ENEMY_SPRITES])
{
    const Enemy *e;
    EnemyU8 pose, t, k;

    if (index >= ENEMY_MAX) return 0;
    e = &s->enemy[index];
    if (!e->active) return 0;

    if (e->type == ENEMY_TYPE_5) {
        /* Deux cellules 16x16 a deux couches (N, N+4), N = 40 + 16 x (pose - 3F) a gauche,
         * 80 + 16 x (pose - 43) a droite (miroir). Decalages mesures sur 981 images ; lignes
         * Y-15 a Y. Le retard d'affichage MSX d'une mise a jour n'est pas reproduit. */
        const int right = e->pose >= 0x43;
        const int sitting = e->pose == 0x3F || e->pose == 0x43;
        const EnemyU8 n = (EnemyU8)(right ? 0x80 + 16 * (e->pose - 0x43) : 0x40 + 16 * (e->pose - 0x3F));
        const EnemyU8 left = (EnemyU8)(right ? n + 8 : n);
        const EnemyU8 rightCell = (EnemyU8)(right ? n : n + 8);
        const EnemyS16 xl = (EnemyS16)(e->x + (sitting ? (right ? -23 : -9) : -16));
        const EnemyS16 xr = (EnemyS16)(e->x + (sitting ? (right ? -7 : 7) : 0));
        for (k = 0; k < ENEMY_SPRITES; k++) {
            out[k].y = (EnemyU8)(e->y - 15);
            out[k].x = k < 2 ? xl : xr;
            out[k].pattern = (EnemyU8)((k < 2 ? left : rightCell) + (k & 1 ? 4 : 0));
            out[k].bank = 1;
        }
        return ENEMY_SPRITES;
    }

    if (e->type == ENEMY_TYPE_2) {
        /* Une colonne de deux cellules 16x16 a deux couches (N, N+4), lignes Y-31 a Y, X-8 :
         * haut 40 (gauche) ou 68 (droite, miroir) ; bas 48/50 (poses 08/09) ou 70/78 (0B/0C).
         * Mesure sur 5395 images ; la SAT MSX montre l'objet d'une mise a jour plus tot
         * (retard non reproduit, comme pour le type 5). */
        const int right = e->pose >= 0x0B;
        const EnemyU8 top = (EnemyU8)(right ? 0x68 : 0x40);
        const EnemyU8 bottom = (EnemyU8)((right ? 0x70 : 0x48) + ((e->pose == 0x09 || e->pose == 0x0C) ? 8 : 0));
        for (k = 0; k < ENEMY_SPRITES; k++) {
            out[k].y = (EnemyU8)(e->y - (k < 2 ? 31 : 15));
            out[k].x = (EnemyS16)(e->x - 8);
            out[k].pattern = (EnemyU8)((k < 2 ? top : bottom) + (k & 1 ? 4 : 0));
            out[k].bank = 1;
        }
        return ENEMY_SPRITES;
    }

    if (e->type == ENEMY_TYPE_SPLASH) {
        /* Goutte : motif 90 seul (couleur 6), SAT (X-8, Y-15), lignes Y-14 a Y+1 (4004 images). */
        out[0].y = (EnemyU8)(e->y - 14);
        out[0].x = (EnemyS16)(e->x - 8);
        out[0].pattern = ENEMY_SPLASH_PATTERN;
        out[0].bank = 1;
        return 1;
    }

    if (e->type == ENEMY_TYPE_4) {
        /* Une cellule 16x16, deux couches (N et N+4), 4 poses de 16 images. Les motifs
         * portent ENEMY_PATTERN_TYPE4 : leurs numeros recouvrent ceux des types A et B. */
        const EnemyU8 pattern = kType4Patterns[e->dx < 0 ? 0 : 1][(e->age / 16) & 3];
        out[0].y = e->y;
        out[0].x = e->x;
        out[0].pattern = (EnemyU8)(ENEMY_PATTERN_TYPE4 | pattern);
        out[1].y = e->y;
        out[1].x = e->x;
        out[1].pattern = (EnemyU8)(ENEMY_PATTERN_TYPE4 | (pattern + 4));
        out[0].bank = out[1].bank = 0;
        return ENEMY_TYPE4_SPRITES;
    }

    pose = poseOf(e->age);
    t = (e->type == ENEMY_TYPE_B) ? 1 : 0;
    for (k = 0; k < ENEMY_SPRITES; k++) {
        out[k].y = (EnemyU8)(k < 2 ? e->y : e->y + ENEMY_ROW_OFFSET);
        out[k].x = e->x;
        out[k].pattern = kPatterns[t][k < 2 ? 0 : pose][k];
        out[k].bank = 0;
    }
    return ENEMY_SPRITES;
}
