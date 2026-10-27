/*
 * Logique pure des ennemis de la salle 1 (voir enemies.h et research/enemy-spawn.md, section 7).
 *
 * Traduction des routines Z80 du spawn type 1 :
 *   - 9CE6 (banque 2, 0x9CE6) : canal type 1, appele depuis 5EF6 si bit0 du drapeau ROM.
 *   - 9CC3 (banque 2, 0x9CC3) : minuterie partagee CF10 + compte a rebours CF00.
 *   - 9CFC (banque 2, 0x9CFC) : position de spawn (E = Y, D = X de spawn).
 *   - 9E16 (banque 2, 0x9E16) : garde de spawn selon Simon (C427) et D.
 *   - 5F1D (banque 0, 0x5F1D) : creation de l'entite dans le premier bloc libre.
 *
 * Niveau de confiance par element :
 *   - Tables kEnemy*                            verifie (lecture ROM, adresses ci-dessous)
 *   - Minuterie (4 appels par decrement, reload = periode - 2*D012 + 1)   verifie
 *       code + intervalles 104 et 152 images (2102-2206, 2206-2358)
 *   - Garde 9E16 et choix de cote (L=1)        verifie code + 6 cas de trace
 *   - Mouvement, animation, sortie             observe (trace frames.txt)
 *   - Type par bloc (C800 = A, C880 = B)       observe, 4 spawns ; TODO(non-verifie) au-dela
 *   - extraPhase (1 pour A, 15 pour B)         observe, cause inconnue : TODO(non-verifie)
 *   - Y de sprite = E - 0x20                   observe pour E=C0 seulement : TODO(non-verifie)
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

/* Tables ROM salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags : ROM 0x85CF-0x85D6 (pointeur 85A8 = 85CF pour D000 = 1).
 *   D001 = 0 -> 01 (canal type 1 actif), 1 -> 01, 2 -> 00, 3 -> 00, 4 -> 08 (bit3, non porte), 5 -> 01, 6 -> 01, 7 -> 00.
 * kEnemySpawnPeriod : ROM 0x9D43-0x9D4A (banque 2). */
const uint8_t kEnemyStage1RoomFlags[8] = { 0x01, 0x01, 0x00, 0x00, 0x08, 0x01, 0x01, 0x00 };
const uint8_t kEnemySpawnPeriod[8] = { 0x0C, 0x12, 0x0C, 0x0C, 0x0C, 0x12, 0x0C, 0x0C };

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

/* Une image pour un ennemi deja actif. */
static void advance(Enemy *e);

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

void Enemies_setRoom(EnemyState *s)
{
    uint8_t i;
    for (i = 0; i < ENEMY_MAX; i++) clearEnemy(&s->enemy[i]);
}

void Enemies_spawnTimerInit(EnemySpawnTimer *t)
{
    /* ROM 9CBB : CF00 = 04, CF01 = 00. CF10 non initialise par cette routine :
     * sa valeur de depart n'est pas prouvee ; 0 est choisi ici (TODO(non-verifie)). */
    t->phase = 0;
    t->counter = 4;
    t->index = 0;
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
    return -1; /* plein : spawn perdu (TODO(non-verifie) : comportement MSX si plein, 5F5C/5F42 ne testent que le bloc) */
}

/* Un appel de 9CC3 (minuterie) puis, si le compte a rebours expire, 9CE6 (spawn). */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    uint8_t idx, period, reload, c, dCoord, yCoord, slot, i;
    uint8_t side;
    uint8_t type;

    /* TODO(non-verifie) : tables des autres stages non extraites. */
    if (v->stage != 1 || v->room > 7) return 0;
    /* 5EE7-5EF6 : 9CE6 n'est appele que si bit0 du drapeau de la salle est a 1. */
    if ((kEnemyStage1RoomFlags[v->room] & 0x01) == 0) return 0;

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
    c = (uint8_t)(v->difficulty << 1);
    reload = (period >= c) ? (uint8_t)(period - c) : 0;
    t->counter = (uint8_t)(reload + 1);

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
    if (slot == ENEMY_MAX) return 0;           /* plein : spawn perdu (TODO(non-verifie)) */

    /* Type observe : bloc 0 (C800) = A, bloc 1 (C880) = B. Au-dela : TODO(non-verifie). */
    type = (slot & 1) ? ENEMY_TYPE_B : ENEMY_TYPE_A;
    side = (dCoord == 0xF0) ? ENEMY_SIDE_RIGHT : ENEMY_SIDE_LEFT;

    /* Y de sprite = E - 0x20 (observe pour E = C0 -> A0). */
    yCoord = (uint8_t)(yCoord - 0x20);
    return Enemies_spawn(s, side, type, yCoord,
                         (type == ENEMY_TYPE_A) ? 1 : 15) >= 0;
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
    for (i = 0; i < ENEMY_MAX; i++) {
        if (s->enemy[i].active) advance(&s->enemy[i]);
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
