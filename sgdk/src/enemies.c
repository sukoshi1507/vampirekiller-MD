/*
 * Logique pure des ennemis du hall (voir enemies.h et research/enemy-spawn.md, enemy-behavior.md).
 *
 * Traduction des routines Z80 :
 *   - 9CE6 (banque 2) : canal type 1, appele depuis 5EF6 si bit0 du drapeau ROM.
 *   - 9CC3 (banque 2) : minuterie partagee CF10 + compte a rebours CF00.
 *   - 9CFC (banque 2) : position de spawn (E = Y, D = X de spawn).
 *   - 9E16 (banque 2) : garde de spawn selon Simon (C427) et D.
 *   - 9D97 (banque 2) : canal type 4 (salle 4). Garde non verifiee, cadence mesuree.
 *   - 5F1D (banque 0) : creation de l'entite dans le premier bloc libre.
 *
 * Niveau de confiance :
 *   - Tables kEnemy*                                       verifie (ROM)
 *   - Minuterie type 1 (4 appels par decrement, reload = periode - 2*D012 + 1)   verifie
 *   - Garde 9E16                                           verifie code + 6 cas de trace
 *   - Cote = type (A droite, B gauche)                     enemy-behavior 6.3
 *   - Mouvement, animation, sortie du type 1               observe (trace frames.txt)
 *   - Type 4 : cadence 168 images, 1,5 px/image, sortie    observe (4 creations)
 *   - Contact -2, recul 22 images a 2 px, invulnerabilite 180 images   observe (run 3, run 6)
 *
 * PROVISOIRE (non verifie, a confirmer en emulation) :
 *   - Cote du type 1 : alternance par tir (index CF01 & 1). La vraie regle (C42C) n'est pas relevee.
 *   - Boite de contact de Simon : 16 px centres sur X (X-8 a X+7), 32 px au-dessus de Y.
 *     La boite de contact MSX n'est pas mesuree ; l'ennemi touche observe a X ~ 122 pour Simon en 120.
 *   - Recul : 2 px par image dans le sens de l'ennemi, sans blocage des commandes ni arc vertical (C420, C425).
 *   - Type 4 : phase du premier tir (168 images apres l'entree) et absence de garde 9E16 non verifiees.
 *   - Type 5 (salles 2, 3, 7) : non porte (graphisme et declencheur inconnus).
 *   - extraPhase (1 pour A, 15 pour B) : observe, cause inconnue.
 *   - Salle 1 avec Simon haut (E = 60) : condition C425 < 0x88 (module), pas une regle verifiee.
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

/* Tables ROM salle de stage 1 (D000 = 1).
 * kEnemyStage1RoomFlags : ROM 0x85CF-0x85D6 (pointeur 85A8 = 85CF pour D000 = 1).
 *   D001 = 0 -> 01, 1 -> 01, 2 -> 00, 3 -> 00, 4 -> 08 (type 4), 5 -> 01, 6 -> 01, 7 -> 00.
 * kEnemySpawnPeriod : ROM 0x9D43-0x9D4A (banque 2). */
const EnemyU8 kEnemyStage1RoomFlags[8] = { 0x01, 0x01, 0x00, 0x00, 0x08, 0x01, 0x01, 0x00 };
const EnemyU8 kEnemySpawnPeriod[8] = { 0x0C, 0x12, 0x0C, 0x0C, 0x0C, 0x12, 0x0C, 0x0C };

#define ENEMY_FLAG_TYPE1 0x01
#define ENEMY_FLAG_TYPE4 0x08

/* Motifs : [type][pose][entree] ; entrees 0,1 = rang haut ; 2,3 = rang bas. */
static const EnemyU8 kPatterns[2][2][4] = {
    { { 0x40, 0x44, 0x48, 0x4C },   /* type A, pose A */
      { 0x40, 0x44, 0x50, 0x54 } }, /* type A, pose B */
    { { 0x58, 0x5C, 0x60, 0x64 },   /* type B, pose A */
      { 0x58, 0x5C, 0x68, 0x6C } }  /* type B, pose B */
};

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
}

void Enemies_setRoom(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) clearEnemy(&s->enemy[i]);
    s->parity = 0;
    s->type4Frames = 0;
    s->type4Side = ENEMY_SIDE_RIGHT;
}

void Enemies_init(EnemyState *s)
{
    Enemies_setRoom(s);
    s->energy = ENEMY_ENERGY_MAX;
    s->invuln = 0;
    s->invulnPhase = 0;
    s->recoil = 0;
    s->recoilDir = 0;
}

void Enemies_spawnTimerInit(EnemySpawnTimer *t)
{
    /* ROM 9CBB : CF00 = 04, CF01 = 00. CF10 non initialise par cette routine :
     * sa valeur de depart n'est pas prouvee ; 0 est choisi ici (provisoire). */
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

/* Un appel de 9CC3 (minuterie) puis, si le compte a rebours expire, 9CE6 (spawn). */
int Enemies_spawnTimerTick(EnemySpawnTimer *t, const EnemyRoomVars *v, EnemyState *s)
{
    EnemyU8 idx, period, reload, c, dCoord, yCoord, slot, i;
    EnemyU8 side;
    EnemyU8 type;

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
    return Enemies_spawn(s, side, type, yCoord,
                         (type == ENEMY_TYPE_A) ? 1 : 15) >= 0;
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

/* Une image pour un ennemi deja actif. */
static void advance(Enemy *e)
{
    if (e->exiting) {          /* masque a l'image suivant la limite (observe 1126, 1208) */
        e->active = 0;
        return;
    }
    e->age++;
    if (e->type == ENEMY_TYPE_4) {
        /* 1,5 px par image : pas alternant de 2 et 1 px (observe 152 images pour 228 px). */
        e->sub ^= 1;
        e->x += (EnemyS16)(e->dx * (e->sub ? 2 : 1));
        if (e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT_T4) e->exiting = 1;
        if (e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT_T4) e->exiting = 1;
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
    if (e->dx < 0 && e->x <= ENEMY_EXIT_X_LEFT) e->exiting = 1;
    if (e->dx > 0 && e->x >= ENEMY_EXIT_X_RIGHT) e->exiting = 1;
}

void Enemies_update(EnemyState *s)
{
    EnemyU8 i;
    for (i = 0; i < ENEMY_MAX; i++) {
        if (s->enemy[i].active) advance(&s->enemy[i]);
    }
}

/* Boite de contact : Simon est dessine sur 24 x 32 en (X-8, Y-31) ; ennemi [x, x+16) x [y, y+h).
 * Les deux boites sont celles des sprites affiches (provisoire : la boite MSX n'est pas mesuree). */
static int touches(const Enemy *e, int simonX, int simonY)
{
    const int h = (e->type == ENEMY_TYPE_4) ? ENEMY_TYPE4_ROW_H : ENEMY_TYPE1_ROW_H;
    return e->x < simonX + ENEMY_SIMON_RIGHT &&
           e->x + 16 > simonX - ENEMY_SIMON_HALF_W &&
           e->y < simonY + 1 &&
           e->y + h > simonY - ENEMY_SIMON_H;
}

#define ENEMY_SCREEN_TO_GAME_X 8   /* Simon et les ennemis : sprite X = x de jeu - 8 */

int Enemies_whipHit(EnemyState *s, int left, int top, int right, int bottom)
{
    EnemyU8 i;
    int killed = 0;
    for (i = 0; i < ENEMY_MAX; i++) {
        Enemy *e = &s->enemy[i];
        int ex, ey;
        if (!e->active || e->exiting || e->type == ENEMY_TYPE_4) continue;
        ex = e->x + ENEMY_SCREEN_TO_GAME_X;
        ey = e->y;
        if (right < ex || left >= ex + 16 || bottom < ey || top >= ey + ENEMY_TYPE1_ROW_H) continue;
        e->active = 0;   /* un coup suffit pour le type 1 (observe par le joueur, zombies) */
        killed++;
    }
    return killed;
}

int Enemies_frame(EnemyState *s, EnemySpawnTimer *t, const EnemyRoomVars *v, int *pushX)
{
    EnemyRoomVars w = *v;
    EnemyU8 i;
    int hit = 0;

    /* Recul : 2 px par image dans le sens de l'ennemi touche (C427 de 136 a 92). */
    *pushX = 0;
    if (s->recoil) {
        *pushX = ENEMY_RECOIL_STEP * s->recoilDir;
        s->recoil--;
    }

    Enemies_update(s);
    if (v->map) {
        for (i = 0; i < ENEMY_MAX; i++) {
            Enemy *e = &s->enemy[i];
            if (e->active && !e->exiting) fall(e, v->map, v->stage);
        }
    }

    /* Type 1 : 9CC3 une image sur deux. Le cote alterne par tir (provisoire). */
    if (s->parity == 0) {
        w.sideFlag = (EnemyU8)(t->index & 1);
        (void)Enemies_spawnTimerTick(t, &w, s);
    }
    s->parity ^= 1;

    /* Type 4 : salle 4 (drapeau bit3). Un tir tous les 168 images, cote alterne, droite d'abord. */
    if (v->stage == 1 && v->room < 8 && (kEnemyStage1RoomFlags[v->room] & ENEMY_FLAG_TYPE4)) {
        if (++s->type4Frames >= ENEMY_TYPE4_PERIOD) {
            s->type4Frames = 0;
            (void)Enemies_spawn(s, s->type4Side, ENEMY_TYPE_4, ENEMY_TYPE4_Y, 0);
            s->type4Side ^= 1;
        }
    }

    /* Contact : desactive pendant le recul et l'invulnerabilite (C420, C42D). */
    if (s->recoil == 0 && s->invuln == 0) {
        for (i = 0; i < ENEMY_MAX; i++) {
            const Enemy *e = &s->enemy[i];
            if (!e->active || e->exiting) continue;
            if (!touches(e, v->simonX, v->simonY)) continue;
            s->energy = (s->energy > ENEMY_CONTACT_DAMAGE) ? (EnemyU8)(s->energy - ENEMY_CONTACT_DAMAGE) : 0;
            s->invuln = ENEMY_INVULN_START;
            s->invulnPhase = 0;
            s->recoil = ENEMY_RECOIL_FRAMES;
            s->recoilDir = e->dx;
            hit = 1;
            break;
        }
    }

    /* C42D ne decroit qu'une image sur deux : 0x5A x 2 = 180 images. */
    if (s->invuln) {
        s->invulnPhase ^= 1;
        if (s->invulnPhase == 0) s->invuln--;
    }
    return hit;
}

int Enemies_sprites(const EnemyState *s, EnemyU8 index, EnemySprite out[ENEMY_SPRITES])
{
    const Enemy *e;
    EnemyU8 pose, t, k;

    if (index >= ENEMY_MAX) return 0;
    e = &s->enemy[index];
    if (!e->active) return 0;

    if (e->type == ENEMY_TYPE_4) {
        /* Une cellule 16x16, deux couches (N et N+4), sans pose. Droite 48/4C, gauche 60/64. */
        const EnemyU8 pattern = (e->dx < 0) ? 0x48 : 0x60;
        out[0].y = e->y;
        out[0].x = e->x;
        out[0].pattern = pattern;
        out[1].y = e->y;
        out[1].x = e->x;
        out[1].pattern = (EnemyU8)(pattern + 4);
        return ENEMY_TYPE4_SPRITES;
    }

    pose = poseOf(e->age);
    t = (e->type == ENEMY_TYPE_B) ? 1 : 0;
    for (k = 0; k < ENEMY_SPRITES; k++) {
        out[k].y = (EnemyU8)(k < 2 ? e->y : e->y + ENEMY_ROW_OFFSET);
        out[k].x = e->x;
        out[k].pattern = kPatterns[t][k < 2 ? 0 : pose][k];
    }
    return ENEMY_SPRITES;
}
